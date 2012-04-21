#ifndef MUDUO_PROTORPC_ZURG_PROCESS_H
#define MUDUO_PROTORPC_ZURG_PROCESS_H

#include <examples/zurg/proto/slave.pb.h>

#include <muduo/net/TimerId.h>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

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

class Process : boost::noncopyable
{
 public:
  Process(muduo::net::EventLoop* loop,
          const RunCommandRequestPtr& request,
          const muduo::net::RpcDoneCallback& done)
    : loop_(loop),
      runCommandRequest(request),
      doneCallback(done),
      pid_(0),
      timerId_(NULL, 0)
  {
  }

  ~Process();

  int start();

  pid_t pid() const
  {
    return pid_;
  }

  void setTimerId(const muduo::net::TimerId& timerId)
  {
    timerId_ = timerId;
  }

  void onExit(int status, const struct rusage&);
  void onTimeout();

  static void onTimeoutWeak(const boost::weak_ptr<Process>& wkPtr);

 private:
  muduo::net::EventLoop* loop_;
  RunCommandRequestPtr runCommandRequest;
  muduo::net::RpcDoneCallback doneCallback;
  pid_t pid_;
  muduo::Timestamp startTime_;
  muduo::net::TimerId timerId_;
};
typedef boost::shared_ptr<Process> ProcessPtr;

}
#endif  // MUDUO_PROTORPC_ZURG_PROCESS_H
