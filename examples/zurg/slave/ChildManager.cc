#include <examples/zurg/slave/ChildManager.h>

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <boost/bind.hpp>

#include <assert.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/signalfd.h>
#include <sys/wait.h>

using namespace muduo::net;
using namespace zurg;

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

ChildManager::ChildManager(muduo::net::EventLoop* loop)
  : loop_(loop),
    signalFd_(::signalfd(-1, &sigmask, SFD_NONBLOCK | SFD_CLOEXEC)),
    channel_(loop_, signalFd_)
{
  LOG_DEBUG << "ChildManager - signalFd_ = " << signalFd_
            << " dummy = " << dummy;
}

ChildManager::~ChildManager()
{
  channel_.disableAll();
  loop_->removeChannel(&channel_);
  ::close(signalFd_);
}

void ChildManager::start()
{
  channel_.setReadCallback(boost::bind(&ChildManager::onRead, this, _1));
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

    pid_t pid = siginfo.ssi_pid;
    LOG_INFO << "ChildManager::onRead - child process " << pid << " exited.";
    int status = 0;
    struct rusage resourceUsage;
    bzero(&resourceUsage, sizeof(resourceUsage));
    pid_t result = ::wait4(pid, &status, WNOHANG, &resourceUsage);
    assert(result == pid);
    // LOG_DEBUG << "========== " << status << " " << siginfo.ssi_status;
    std::map<pid_t, Callback>::iterator it = callbacks_.find(pid);
    if (it != callbacks_.end())
    {
      it->second(status, resourceUsage);
      callbacks_.erase(it);
    }
  }
  else
  {
    LOG_ERROR << "ChildManager::onRead - " << n;
  }
}

