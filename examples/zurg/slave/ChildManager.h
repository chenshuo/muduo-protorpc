#ifndef MUDUO_PROTORPC_ZURG_CHILDMANAGER_H
#define MUDUO_PROTORPC_ZURG_CHILDMANAGER_H

#include <muduo/net/Channel.h>

#include <map>

namespace muduo
{
namespace net
{
class EventLoop;
}
}

struct rusage;

namespace zurg
{

class ChildManager : muduo::noncopyable
{
 public:
  typedef std::function<void(int status, const struct rusage&)> Callback;

  ChildManager(muduo::net::EventLoop* loop, int zombieInterval);
  ~ChildManager();

  void start();

  void runAtExit(pid_t pid, const Callback&);

 private:

  void onRead(muduo::Timestamp t);
  void onTimer();
  void onExit(pid_t pid, int status, const struct rusage&);

  muduo::net::EventLoop* loop_;
  int zombieInterval_;
  int signalFd_;
  muduo::net::Channel channel_;
  std::map<pid_t, Callback> callbacks_;
};

}

#endif  // MUDUO_PROTORPC_ZURG_CHILDMANAGER_H
