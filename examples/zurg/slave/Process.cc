#include <examples/zurg/slave/Process.h>

#include <examples/zurg/slave/Sink.h>
#include <examples/zurg/slave/SlaveApp.h>

#include <examples/zurg/common/Util.h>

#include <muduo/base/Logging.h>
#include <muduo/base/ProcessInfo.h>
#include <muduo/base/FileUtil.h>
#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>

#include <stdexcept>

#include <assert.h>
#include <errno.h>
#include <fcntl.h> // O_CLOEXEC
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>

namespace zurg
{
extern sigset_t oldSigmask;

float getSeconds(const struct timeval& tv)
{
  int64_t microSeconds = tv.tv_sec*1000000 + tv.tv_usec;
  double seconds = static_cast<double>(microSeconds)/1000000.0;
  return static_cast<float>(seconds);
}

int redirect(bool toFile, const std::string& prefix, const char* postfix)
{
  int fd = -1;
  if (toFile)
  {
    char buf[256];
    ::snprintf(buf, sizeof buf, "%s.%d.%s",
               prefix.c_str(), muduo::ProcessInfo::pid(), postfix);
    fd = ::open(buf, O_WRONLY | O_CREAT | O_CLOEXEC, 0644);
  }
  else
  {
    fd = ::open("/dev/null", O_WRONLY | O_CREAT | O_CLOEXEC, 0644);
  }
  return fd;
}

class Pipe : muduo::noncopyable
{
 public:
  Pipe()
  {
    if (::pipe2(pipefd_, O_CLOEXEC))
    {
      throw std::runtime_error(muduo::strerror_tl(errno));
    }
  }

  ~Pipe()
  {
    if (readFd() >= 0)
      closeRead();
    if (writeFd() >= 0)
      closeWrite();
  }

  int readFd() const { return pipefd_[kRead]; }
  int writeFd() const { return pipefd_[kWrite]; }

  ssize_t read(int64_t* x)
  {
    return ::read(readFd(), x, sizeof(*x));
  }

  void write(int64_t x)
  {
    ssize_t n = ::write(writeFd(), &x, sizeof(x));
    assert(n == sizeof(x)); (void)n;
  }

  void closeRead()
  {
    ::close(readFd());
    pipefd_[kRead] = -1;
  }

  void closeWrite()
  {
    ::close(writeFd());
    pipefd_[kWrite] = -1;
  }

 private:
  int pipefd_[2];
  static const int kRead = 0, kWrite = 1;
};

struct ProcStatFile
{
 public:
  explicit ProcStatFile(int pid)
    : valid(false),
      error(0),
      ppid(0),
      startTime(0)
  {
    char filename[64];
    ::snprintf(filename, sizeof filename, "/proc/%d/stat", pid);
    muduo::FileUtil::ReadSmallFile file(filename);
    if ( (error = file.readToBuffer(NULL)) == 0)
    {
      valid = true;
      parse(file.buffer());
    }
  }

  bool valid;
  int error;
  int ppid;
  int64_t startTime;

 private:
  void parse(const char* buffer)
  {
    // do_task_stat() in fs/proc/array.c
    // pid (cmd) S ppid pgid sid
    const char* p = buffer;
    p = strchr(p, ')');
    for (int i = 0; i < 20 && (p); ++i)
    {
      p = strchr(p, ' ');
      if (p)
      {
        if (i == 1)
        {
          ppid = atoi(p);
        }
        ++p;
      }
    }
    if (p)
    {
      startTime = atoll(p);
    }
  }

};

const int kSleepAfterExec = 0; // for strace child
}

using namespace zurg;
using namespace muduo;
using namespace muduo::net;

// ========================================================

Process::Process(EventLoop* loop,
                 const RunCommandRequestPtr& request,
                 const RpcDoneCallback& done)
  : loop_(loop),
    request_(request),
    doneCallback_(done),
    pid_(0),
    startTimeInJiffies_(0),
    redirectStdout_(false),
    redirectStderr_(false),
    runCommand_(true)
{
}

Process::Process(const AddApplicationRequestPtr& appRequest)
  : loop_(NULL),
    request_(new RunCommandRequest),
    doneCallback_(),
    pid_(0),
    name_(appRequest->name()),
    startTimeInJiffies_(0),
    redirectStdout_(appRequest->redirect_stdout()),
    redirectStderr_(appRequest->redirect_stderr()),
    runCommand_(false)
{
  request_->set_command(appRequest->binary());

  char dir[256];
  snprintf(dir, sizeof dir, "%s/%s/%s",
           SlaveApp::instance().prefix().c_str(),
           SlaveApp::instance().name().c_str(),
           appRequest->name().c_str());
  request_->set_cwd(dir);

  request_->mutable_args()->CopyFrom(appRequest->args());
  request_->mutable_envs()->CopyFrom(appRequest->envs());
  request_->set_envs_only(appRequest->envs_only());
  request_->set_max_stdout(0);
  request_->set_max_stderr(0);
  request_->set_timeout(-1);
  request_->set_max_memory_mb(appRequest->max_memory_mb());
}

Process::~Process()
{
  LOG_DEBUG << "Process[" << pid_ << "] dtor";
  if (stdoutSink_ || stderrSink_)
  {
    assert(!loop_->eventHandling());
  }
}

int Process::start()
{
  assert(ProcessInfo::numThreads() == 1);
  int availabltFds = ProcessInfo::maxOpenFiles() - ProcessInfo::openedFiles();
  if (availabltFds < 20)
  {
    // to fork() and capture stdout/stderr, we need new file descriptors.
    return EMFILE;
  }

  // The self-pipe trick
  // http://cr.yp.to/docs/selfpipe.html
  Pipe execError;

  Pipe stdOutput;
  Pipe stdError;

  startTime_ = Timestamp::now();
  const pid_t result = ::fork(); // FORKED HERE
  if (result == 0)
  {
    // child process
    ::mkdir(request_->cwd().c_str(), 0755); // FIXME: check return value

    int stdoutFd = -1;
    int stderrFd = -1;
    if (runCommand_)
    {
      stdoutFd = stdOutput.writeFd();
      stderrFd = stdError.writeFd();
    }
    else
    {
      string startTimeStr = startTime_.toString();
      // FIXME: capture errno
      stdoutFd = redirect(redirectStdout_, request_->cwd()+"/stdout", startTimeStr.c_str());
      stderrFd = redirect(redirectStderr_, request_->cwd()+"/stderr", startTimeStr.c_str());
    }
    execChild(execError, stdoutFd, stderrFd); // never return
  }
  else if (result > 0)
  {
    // parent
    pid_ = result;
    return afterFork(execError, stdOutput, stdError);
  }
  else
  {
    // failed
    return errno;
  }
}

void Process::execChild(Pipe& execError, int stdOutput, int stdError)
{
  if (kSleepAfterExec > 0)
  {
    ::fprintf(stderr, "Child pid = %d", ::getpid());
    ::sleep(kSleepAfterExec);
  }

  try
  {
    ProcStatFile stat(ProcessInfo::pid());
    if (!stat.valid) throw stat.error;

    execError.write(stat.startTime);

    std::vector<const char*> argv;
    argv.reserve(request_->args_size() + 2);
    argv.push_back(request_->command().c_str());
    for (int i = 0; i < request_->args_size(); ++i)
    {
      argv.push_back(request_->args(i).c_str());
    }
    argv.push_back(NULL);

    // FIXME: new process group, new session
    ::sigprocmask(SIG_SETMASK, &oldSigmask, NULL);
    if (::chdir(request_->cwd().c_str()) < 0)
      throw static_cast<int>(errno);

    // FIXME: max_memory_mb
    // FIXME: environ with execvpe
    int stdInput = ::open("/dev/null", O_RDONLY);
    if (stdInput < 0) throw static_cast<int>(errno);

    if (stdOutput < 0 || stdError < 0)
    {
      if (stdOutput >= 0) ::close(stdOutput);
      if (stdError >= 0) ::close(stdError);
      throw static_cast<int>(EACCES);
    }

    if (::dup2(stdInput, STDIN_FILENO) < 0)
      throw static_cast<int>(errno);
    ::close(stdInput);
    if (::dup2(stdOutput, STDOUT_FILENO) < 0)
      throw static_cast<int>(errno);
    if (::dup2(stdError, STDERR_FILENO) < 0)
      throw static_cast<int>(errno);

    const char* cmd = request_->command().c_str();
    ::execvp(cmd, const_cast<char**>(&*argv.begin()));
    // should not reach here
    throw static_cast<int>(errno);
  }
  catch(int error)
  {
    execError.write(error);
    fprintf(stderr, "CHILD %s (errno=%d)\n", muduo::strerror_tl(error), error);
  }
  catch(...)
  {
    int error = EINVAL;
    execError.write(error);
  }
  ::exit(1);
}

int Process::afterFork(Pipe& execError, Pipe& stdOutput, Pipe& stdError)
{
  LOG_DEBUG << "PARENT child pid " << pid_;
  int64_t childStartTime = 0;
  int64_t childErrno = 0;
  execError.closeWrite();
  ssize_t n = execError.read(&childStartTime);
  assert(n == sizeof(childStartTime));
  startTimeInJiffies_ = childStartTime;
  LOG_DEBUG << "PARENT child start time " << childStartTime;
  n = execError.read(&childErrno);
  if (n == 0)
  {
    LOG_INFO << "started child pid " << pid_;

    char filename[64];
    ::snprintf(filename, sizeof filename, "/proc/%d/exe", pid_);
    char buf[1024];
    ssize_t len = ::readlink(filename, buf, sizeof buf);
    if (len >= 0)
    {
      exe_file_.assign(buf, len);
      LOG_INFO << filename << " -> " << exe_file_;
    }
    else
    {
      LOG_SYSERR << "Fail to read link " << filename;
    }

    // read nothing, child process exec successfully.
    if (runCommand_)
    {
      int stdoutFd = ::dup(stdOutput.readFd());
      int stderrFd = ::dup(stdError.readFd());
      if (stdoutFd < 0 || stderrFd < 0)
      {
        if (stdoutFd >= 0) ::close(stdoutFd);
        if (stderrFd >= 0) ::close(stderrFd);
        return errno;
      }
      setNonBlockAndCloseOnExec(stdoutFd);
      setNonBlockAndCloseOnExec(stderrFd);
      stdoutSink_.reset(new Sink(loop_, stdoutFd, request_->max_stdout(), "stdout"));
      stderrSink_.reset(new Sink(loop_, stderrFd, request_->max_stderr(), "stderr"));
      stdoutSink_->start();
      stderrSink_->start();
    }
    else
    {
      // FIXME: runApp
    }
    return 0;
  }
  else if (n == sizeof(childErrno))
  {
    int err = static_cast<int>(childErrno);
    LOG_ERROR << "PARENT child error " << muduo::strerror_tl(err)
              << " (errno=" << childErrno << ")";
    return err;
  }
  else if (n < 0)
  {
    int err = errno;
    LOG_SYSERR << "PARENT";
    return err;
  }
  else
  {
    LOG_ERROR << "PARENT read " << n;
    return EINVAL;
  }
}

void Process::onTimeoutWeak(const std::weak_ptr<Process>& wkPtr)
{
  ProcessPtr ptr(wkPtr.lock());
  if (ptr)
  {
    ptr->onTimeout();
  }
}

void Process::onTimeout()
{
  // kill process
  LOG_INFO << "Process[" << pid_ << "] onTimeout";

  const ProcStatFile stat(pid_);
  if (stat.valid
      && stat.ppid == muduo::ProcessInfo::pid()
      && stat.startTime == startTimeInJiffies_)
  {
    ::kill(pid_, SIGINT);
  }
}

#pragma GCC diagnostic ignored "-Wold-style-cast"
// FIXME: dup AppManager::onProcessExit
void Process::onCommandExit(const int status, const struct rusage& ru)
{
  RunCommandResponse response;

  char buf[256];
  if (WIFEXITED(status))
  {
    snprintf(buf, sizeof buf, "exit status %d", WEXITSTATUS(status));
    response.set_exit_status(WEXITSTATUS(status));
  }
  else if (WIFSIGNALED(status))
  {
    snprintf(buf, sizeof buf, "signaled %d%s",
             WTERMSIG(status), WCOREDUMP(status) ? " (core dump)" : "");
    response.set_signaled(WTERMSIG(status));
    response.set_coredump(WCOREDUMP(status));
  }

  LOG_INFO << "Process[" << pid_ << "] onCommandExit " << buf;

  assert(!loop_->eventHandling());
  // FIXME: defer 100ms or blocking read to capture all outputs.
  stdoutSink_->stop(pid_);
  stderrSink_->stop(pid_);

  response.set_error_code(0);
  response.set_pid(pid_);
  response.set_status(status);
  response.set_std_output(stdoutSink_->bufferAsStdString());
  response.set_std_error(stderrSink_->bufferAsStdString());
  response.set_executable_file(exe_file_);
  response.set_start_time_us(startTime_.microSecondsSinceEpoch());
  response.set_finish_time_us(muduo::Timestamp::now().microSecondsSinceEpoch());
  response.set_user_time(getSeconds(ru.ru_utime));
  response.set_system_time(getSeconds(ru.ru_stime));
  response.set_memory_maxrss_kb(ru.ru_maxrss);

  doneCallback_(&response);
}
