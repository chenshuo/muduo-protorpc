#include <examples/zurg/slave/Process.h>

#include <examples/zurg/slave/Sink.h>

#include <muduo/base/Logging.h>
#include <muduo/base/ProcessInfo.h>
#include <muduo/base/FileUtil.h>
#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>

#include <boost/weak_ptr.hpp>

#include <stdexcept>

#include <assert.h>
#include <errno.h>
#include <fcntl.h> // O_CLOEXEC
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/resource.h>
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

void setNonBlockAndCloseOnExec(int fd)
{
  // non-block
  int flags = ::fcntl(fd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  int ret = ::fcntl(fd, F_SETFL, flags);
  // FIXME check

  // close-on-exec
  flags = ::fcntl(fd, F_GETFD, 0);
  flags |= FD_CLOEXEC;
  ret = ::fcntl(fd, F_SETFD, flags);
  // FIXME check

  (void)ret;
}

class Pipe : boost::noncopyable
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

  int readFd() const { return pipefd_[0]; }
  int writeFd() const { return pipefd_[1]; }

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
    ::close(pipefd_[0]);
    pipefd_[0] = -1;
  }

  void closeWrite()
  {
    ::close(pipefd_[1]);
    pipefd_[1] = -1;
  }

 private:
  int pipefd_[2];
};

struct ProcStatFile
{
 public:
  explicit ProcStatFile(int pid)
    : valid(false),
      ppid(0),
      startTime(0)
  {
    char filename[64];
    ::snprintf(filename, sizeof filename, "/proc/%d/stat", pid);
    muduo::FileUtil::SmallFile file(filename);
    if (file.readToBuffer(NULL) == 0)
    {
      valid = true;
      parse(file.buffer());
    }
  }

  bool valid;
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
using namespace muduo::net;

Process::Process(muduo::net::EventLoop* loop,
                 const RunCommandRequestPtr& request,
                 const muduo::net::RpcDoneCallback& done)
  : loop_(loop),
    request_(request),
    doneCallback_(done),
    pid_(0),
    startTimeInJiffies_(0)
{
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
  assert(muduo::ProcessInfo::numThreads() == 1);
  int availabltFds = muduo::ProcessInfo::maxOpenFiles() - muduo::ProcessInfo::openedFiles();
  if (availabltFds < 20)
  {
    // to fork() and capture stdout/stderr, we need new file descriptors.
    return EMFILE;
  }

  // The self-pipe trick
  // http://cr.yp.to/docs/selfpipe.html
  Pipe execError, stdOutput, stdError;

  const pid_t result = ::fork();
  if (result == 0)
  {
    // child process
    execChild(execError, stdOutput, stdError); // never return
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

void Process::execChild(Pipe& execError, Pipe& stdOutput, Pipe& stdError)
{
  if (kSleepAfterExec > 0)
  {
    fprintf(stderr, "Child pid = %d", ::getpid());
    ::sleep(kSleepAfterExec);
  }

  ProcStatFile stat(muduo::ProcessInfo::pid());
  execError.write(stat.startTime);

  std::vector<const char*> argv;
  argv.reserve(request_->args_size() + 2);
  argv.push_back(request_->command().c_str());
  for (int i = 0; i < request_->args_size(); ++i)
  {
    argv.push_back(request_->args(i).c_str());
  }
  argv.push_back(NULL);

  ::sigprocmask(SIG_SETMASK, &oldSigmask, NULL);
  // FIXME: max_memory_mb
  // FIXME: environ
  int stdInput = ::open("/dev/null", O_RDONLY);
  ::dup2(stdInput, STDIN_FILENO);
  ::close(stdInput);
  ::dup2(stdOutput.writeFd(), STDOUT_FILENO);
  ::dup2(stdError.writeFd(), STDERR_FILENO);
  if (::chdir(request_->cwd().c_str()) == 0)
  {
    const char* cmd = request_->command().c_str();
    ::execvp(cmd, const_cast<char**>(&*argv.begin()));
  }
  // should not reach here
  execError.write(errno);
  // FIXME: in child process, logging fd is closed.
  // LOG_ERROR << "CHILD " << muduo::strerror_tl(err) << " (errno=" << err << ")";
  ::exit(1);
}

int Process::afterFork(Pipe& execError, Pipe& stdOutput, Pipe& stdError)
{
  LOG_DEBUG << "PARENT child pid " << pid_;
  startTime_ = muduo::Timestamp::now();
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
    // read nothing, child process exec successfully.
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

void Process::onTimeoutWeak(const boost::weak_ptr<Process>& wkPtr)
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

void Process::onExit(int status, const struct rusage& ru)
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

  LOG_INFO << "Process[" << pid_ << "] onExit " << buf;

  assert(!loop_->eventHandling());
  // FIXME: defer 100ms to capture all outputs.
  stdoutSink_->stop(pid_);
  stderrSink_->stop(pid_);

  response.set_error_code(0);
  response.set_pid(pid_);
  response.set_status(status);
  response.set_std_output(stdoutSink_->bufferAsStdString());
  response.set_std_error(stderrSink_->bufferAsStdString());
  response.set_start_time_us(startTime_.microSecondsSinceEpoch());
  response.set_finish_time_us(muduo::Timestamp::now().microSecondsSinceEpoch());
  response.set_user_time(getSeconds(ru.ru_utime));
  response.set_system_time(getSeconds(ru.ru_stime));
  response.set_memory_maxrss_kb(ru.ru_maxrss);

  doneCallback_(&response);
}
