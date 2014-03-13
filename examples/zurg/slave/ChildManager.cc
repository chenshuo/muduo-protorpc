#include <examples/zurg/slave/ChildManager.h>

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <assert.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/signalfd.h>
#include <sys/wait.h>

using namespace muduo::net;
using namespace zurg;
using std::placeholders::_1;

namespace zurg
{
sigset_t sigmask;
sigset_t oldSigmask;

int initSigMask()
{
  ::sigemptyset(&sigmask);
  // ::sigaddset(&sigmask, SIGINT);
  ::sigaddset(&sigmask, SIGCHLD);
  ::sigprocmask(SIG_BLOCK, &sigmask, &oldSigmask);
  return 0;
}
}

static int dummy = initSigMask();

ChildManager::ChildManager(muduo::net::EventLoop* loop, int zombieInterval)
  : loop_(loop),
    zombieInterval_(zombieInterval),
    signalFd_(::signalfd(-1, &sigmask, SFD_NONBLOCK | SFD_CLOEXEC)),
    channel_(loop_, signalFd_)
{
  LOG_DEBUG << "ChildManager - signalFd_ = " << signalFd_
            << " dummy = " << dummy;
}

ChildManager::~ChildManager()
{
  channel_.disableAll();
  channel_.remove();
  ::close(signalFd_);
}

void ChildManager::start()
{
  loop_->runEvery(zombieInterval_, std::bind(&ChildManager::onTimer, this));
  channel_.setReadCallback(std::bind(&ChildManager::onRead, this, _1));
  channel_.enableReading();
}

void ChildManager::runAtExit(pid_t pid, const Callback& cb)
{
  assert(callbacks_.find(pid) == callbacks_.end());
  callbacks_[pid] = cb;
}

void ChildManager::onRead(muduo::Timestamp t)
{
  LOG_TRACE << "ChildManager::onRead - " << t.toString();
  struct signalfd_siginfo siginfo;
  ssize_t n = ::read(signalFd_, &siginfo, sizeof(siginfo));
  if (n == sizeof(siginfo))
  {
    LOG_DEBUG << " ssi_signo = " << siginfo.ssi_signo
              << " ssi_code = " << siginfo.ssi_code
              << " ssi_pid = " << siginfo.ssi_pid
              << " ssi_status = " << siginfo.ssi_status
              << " ssi_utime = " << siginfo.ssi_utime
              << " ssi_stime = " << siginfo.ssi_stime;

    if (siginfo.ssi_signo == SIGCHLD)
    {
      pid_t pid = siginfo.ssi_pid;
      LOG_INFO << "ChildManager::onRead - SIGCHLD of " << pid;
      int status = 0;
      struct rusage resourceUsage;
      bzero(&resourceUsage, sizeof(resourceUsage));
      pid_t result = ::wait4(pid, &status, WNOHANG, &resourceUsage);
      if (result == pid)
      {
        onExit(pid, status, resourceUsage);
      }
      else if (result < 0)
      {
        LOG_SYSERR << "ChildManager::onRead - wait4 ";
      }
    }
    else
    {
      LOG_ERROR << "ChildManager::onRead - unknown signal " << siginfo.ssi_signo;
    }
  }
  else
  {
    LOG_SYSERR << "ChildManager::onRead - " << n;
  }
}

void ChildManager::onTimer()
{
  int status = 0;
  struct rusage resourceUsage;
  bzero(&resourceUsage, sizeof(resourceUsage));
  pid_t pid = 0;
  while ( (pid = ::wait4(-1, &status, WNOHANG, &resourceUsage)) > 0)
  {
    LOG_INFO << "ChildManager::onTimer - child process " << pid << " exited.";
    onExit(pid, status, resourceUsage);
  }
}

void ChildManager::onExit(pid_t pid, int status, const struct rusage& resourceUsage)
{
  std::map<pid_t, Callback>::iterator it = callbacks_.find(pid);
  if (it != callbacks_.end())
  {
    // defer
    loop_->queueInLoop(std::bind(it->second, status, resourceUsage));
    // it->second(status, resourceUsage);
    callbacks_.erase(it);
  }
  else
  {
    // could be failed run commands.
    // LOG_ERROR << "ChildManager::onExit - unknown pid " << pid;
  }
}
