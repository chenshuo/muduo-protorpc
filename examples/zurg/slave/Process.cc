#include <examples/zurg/slave/Process.h>

#include <muduo/base/Logging.h>
#include <muduo/base/ProcessInfo.h>
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

}

using namespace zurg;
using namespace muduo::net;

Process::~Process()
{
  LOG_DEBUG << "Process[" << pid_ << "] dtor";

  if (stdoutFd_ >= 0)
  {
    assert(stdoutChannel_);
    loop_->removeChannel(get_pointer(stdoutChannel_));
    ::close(stdoutFd_);
  }
  else
  {
    assert(!stdoutChannel_);
  }

  if (stderrFd_ >= 0)
  {
    assert(stderrChannel_);
    loop_->removeChannel(get_pointer(stderrChannel_));
    ::close(stderrFd_);
  }
  else
  {
    assert(!stderrChannel_);
  }
}

int Process::start()
{
  assert(muduo::ProcessInfo::threads().size() == 1);

  // The self-pipe trick
  // http://cr.yp.to/docs/selfpipe.html
  Pipe execError, stdOutput, stdError;

  const pid_t result = ::fork();
  if (result == 0)
  {
    // child process
    ::sigprocmask(SIG_SETMASK, &oldSigmask, NULL);
    // FIXME: max_memory_mb
    int stdInput = ::open("/dev/null", O_RDONLY);
    ::dup2(stdInput, STDIN_FILENO);
    ::close(stdInput);
    ::dup2(stdOutput.writeFd(), STDOUT_FILENO);
    ::dup2(stdError.writeFd(), STDERR_FILENO);
    const char* cmd = runCommandRequest->command().c_str();
    ::execlp(cmd, cmd, static_cast<const char*>(NULL));
    // should not reach here
    int32_t err = errno;
    // FIXME: in child process, logging fd is closed.
    LOG_ERROR << "CHILD " << muduo::strerror_tl(err) << " (errno=" << err << ")";
    ::write(execError.writeFd(), &err, sizeof(err));
    ::exit(1);
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

int Process::afterFork(Pipe& execError, Pipe& stdOutput, Pipe& stdError)
{
  LOG_DEBUG << "PARENT child pid " << pid_;
  startTime_ = muduo::Timestamp::now();
  int32_t childErrno = 0;
  execError.closeWrite();
  ssize_t n = ::read(execError.readFd(), &childErrno, sizeof(childErrno));
  if (n == 0)
  {
    LOG_INFO << "started child pid " << pid_;
    // read nothing, child process exec successfully.
    stdoutFd_ = ::dup(stdOutput.readFd());
    stderrFd_ = ::dup(stdError.readFd());
    if (stdoutFd_ < 0 || stderrFd_ < 0)
    {
      return errno;
    }
    stdoutChannel_.reset(new Channel(loop_, stdoutFd_));
    stderrChannel_.reset(new Channel(loop_, stderrFd_));
    stdoutChannel_->setReadCallback(boost::bind(&Process::onReadStdout, this, _1));
    stderrChannel_->setReadCallback(boost::bind(&Process::onReadStderr, this, _1));
    stdoutChannel_->setCloseCallback(boost::bind(&Process::onCloseStdout, this));
    stderrChannel_->setCloseCallback(boost::bind(&Process::onCloseStderr, this));
    stdoutChannel_->doNotLogHup();
    stderrChannel_->doNotLogHup();
    stdoutChannel_->enableReading();
    stderrChannel_->enableReading();
    return 0;
  }
  else if (n == sizeof(childErrno))
  {
    LOG_ERROR << "PARENT child error " << muduo::strerror_tl(childErrno)
      << " (errno=" << childErrno << ")";
    return childErrno;
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
  ::kill(pid_, SIGINT);
}

#pragma GCC diagnostic ignored "-Wold-style-cast"

void Process::onExit(int status, const struct rusage& ru)
{
  char buf[256];

  if (WIFEXITED(status))
  {
    snprintf(buf, sizeof buf, "exit status %d", WEXITSTATUS(status));
  }
  else if (WIFSIGNALED(status))
  {
    snprintf(buf, sizeof buf, "signaled %d%s",
             WTERMSIG(status), WCOREDUMP(status) ? " (core dump)" : "");
  }

  LOG_INFO << "Process[" << pid_ << "] onExit " << buf;
  assert(!loop_->eventHandling());

  // FIXME: defer 100ms to capture all outputs.
  if (!stdoutChannel_->isNoneEvent())
  {
    LOG_WARN << pid_ << " stdoutChannel_ disableAll before child closing";
    stdoutChannel_->disableAll();
  }
  if (!stderrChannel_->isNoneEvent())
  {
    LOG_WARN << pid_ << " stderrChannel_ disableAll before child closing";
    stderrChannel_->disableAll();
  }

  RunCommandResponse response;
  response.set_error_code(0);
  response.set_pid(pid_);
  response.set_status(status);
  response.set_std_output(stdoutBuffer_.peek(), stdoutBuffer_.readableBytes());
  response.set_start_time_us(startTime_.microSecondsSinceEpoch());
  response.set_finish_time_us(muduo::Timestamp::now().microSecondsSinceEpoch());
  response.set_user_time(getSeconds(ru.ru_utime));
  response.set_system_time(getSeconds(ru.ru_stime));
  response.set_memory_maxrss_kb(ru.ru_maxrss);

  doneCallback(&response);
}

void Process::onReadStdout(muduo::Timestamp t)
{
  LOG_DEBUG << stdoutFd_;
  int savedErrno = 0;
  ssize_t n = stdoutBuffer_.readFd(stdoutFd_, &savedErrno);
  if (n == 0)
  {
    LOG_DEBUG << "disableAll";
    stdoutChannel_->disableAll();
  }
  // FIXME: max_stdout
}

void Process::onReadStderr(muduo::Timestamp t)
{
  LOG_DEBUG << stderrFd_;
  int savedErrno = 0;
  ssize_t n = stderrBuffer_.readFd(stderrFd_, &savedErrno);
  if (n == 0)
  {
    LOG_DEBUG << "disableAll";
    stderrChannel_->disableAll();
  }
  // FIXME: max_stderr
}

void Process::onCloseStdout()
{
  LOG_DEBUG << "disableAll";
  stdoutChannel_->disableAll();
}

void Process::onCloseStderr()
{
  LOG_DEBUG << "disableAll";
  stderrChannel_->disableAll();
}
