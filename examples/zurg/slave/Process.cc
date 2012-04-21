#include <examples/zurg/slave/Process.h>

#include <muduo/base/Logging.h>
#include <muduo/base/ProcessInfo.h>

#include <boost/weak_ptr.hpp>

#include <assert.h>
#include <errno.h>
#include <fcntl.h> // O_CLOEXEC
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>

namespace zurg
{
extern sigset_t oldSigmask;

float getSeconds(const struct timeval& tv)
{
  int64_t microSeconds = tv.tv_sec*1000000 + tv.tv_usec;
  double seconds = static_cast<double>(microSeconds)/1000000.0;
  return static_cast<float>(seconds);
}

}

using namespace zurg;

Process::~Process()
{
  LOG_DEBUG << "Process[" << pid_ << "] dtor";
}

int Process::start()
{
  // The self-pipe trick
  // http://cr.yp.to/docs/selfpipe.html
  int pipefd[2];  // unfortunately, RAII doesn't work when forking
  if (::pipe2(pipefd, O_CLOEXEC))
  {
    return errno;
  }

  assert(muduo::ProcessInfo::threads().size() == 1);

  const pid_t result = ::fork();
  if (result == 0)
  {
    // child
    ::sigprocmask(SIG_SETMASK, &oldSigmask, NULL);
    ::close(pipefd[0]); // close read
    const char* cmd = runCommandRequest->command().c_str();
    ::execlp(cmd, cmd, static_cast<const char*>(NULL));
    // should not reach here
    int32_t err = errno;
    // FIXME: in child process, logging fd is closed.
    LOG_ERROR << "CHILD " << muduo::strerror_tl(err) << " (errno=" << err << ")";
    ::write(pipefd[1], &err, sizeof(err));
    ::close(pipefd[1]);
    ::exit(1);
  }
  else if (result > 0)
  {
    // parent
    startTime_ = muduo::Timestamp::now();
    pid_ = result;
    ::close(pipefd[1]); // close write
    int32_t childErrno = 0;
    ssize_t n = ::read(pipefd[0], &childErrno, sizeof(childErrno));
    ::close(pipefd[0]); // close read
    LOG_DEBUG << "PARENT read " << n;
    if (n == 0)
    {
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
      LOG_ERROR << "PARENT " << muduo::strerror_tl(err) << " (errno=" << err << ")";
      return err;
    }
    else
    {
      LOG_ERROR << "PARENT read " << n;
      return EINVAL;
    }
  }
  else
  {
    // failed
    return errno;
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
  // FIXME:
}

void Process::onExit(int status, const struct rusage& ru)
{
  LOG_INFO << "Process[" << pid_ << "] onExit";
  RunCommandResponse response;
  response.set_error_code(0);
  response.set_pid(pid_);
  response.set_status(status);
  // stdout stderr
  double realSeconds = muduo::timeDifference(muduo::Timestamp::now(), startTime_);
  response.set_real_time(static_cast<float>(realSeconds));
  response.set_user_time(getSeconds(ru.ru_utime));
  response.set_system_time(getSeconds(ru.ru_stime));
  response.set_memory_maxrss_kb(ru.ru_maxrss);

  doneCallback(&response);
}

