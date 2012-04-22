#ifndef MUDUO_PROTORPC_ZURG_PROCESS_H
#define MUDUO_PROTORPC_ZURG_PROCESS_H

#include <examples/zurg/proto/slave.pb.h>

#include <muduo/net/Buffer.h>
#include <muduo/net/TimerId.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace muduo
{
namespace net
{
class Channel;
class EventLoop;
}
}

struct rusage;

namespace zurg
{

class Pipe;

class Process : boost::noncopyable
{
 public:
  Process(muduo::net::EventLoop* loop,
          const RunCommandRequestPtr& request,
          const muduo::net::RpcDoneCallback& done)
    : loop_(loop),
      request_(request),
      doneCallback_(done),
      pid_(0),
      stdoutFd_(-1),
      stderrFd_(-1)
  {
  }

  ~Process();

  int start(); // may throw

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
  void execChild(Pipe& execError, Pipe& stdOutput, Pipe& stdError)
       __attribute__ ((__noreturn__));;
  int afterFork(Pipe& execError, Pipe& stdOutput, Pipe& stdError);

  void onReadStdout(muduo::Timestamp t);
  void onReadStderr(muduo::Timestamp t);
  void onCloseStdout();
  void onCloseStderr();

  muduo::net::EventLoop* loop_;
  RunCommandRequestPtr request_;
  muduo::net::RpcDoneCallback doneCallback_;
  pid_t pid_;
  muduo::Timestamp startTime_;
  muduo::net::TimerId timerId_;

  int stdoutFd_;
  int stderrFd_;
  muduo::net::Buffer stdoutBuffer_;
  muduo::net::Buffer stderrBuffer_;
  boost::scoped_ptr<muduo::net::Channel> stdoutChannel_;
  boost::scoped_ptr<muduo::net::Channel> stderrChannel_;
};
typedef boost::shared_ptr<Process> ProcessPtr;

}
#endif  // MUDUO_PROTORPC_ZURG_PROCESS_H
