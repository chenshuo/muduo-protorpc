#ifndef MUDUO_PROTORPC_ZURG_PROCESS_H
#define MUDUO_PROTORPC_ZURG_PROCESS_H

#include <examples/zurg/proto/slave.pb.h>

#include <muduo/net/Buffer.h>
#include <muduo/net/TimerId.h>

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

class Pipe;
class Sink;

class Process : public std::enable_shared_from_this<Process>,
                muduo::noncopyable
{
 public:
  Process(muduo::net::EventLoop* loop,
          const RunCommandRequestPtr& request,
          const muduo::net::RpcDoneCallback& done);

  Process(const AddApplicationRequestPtr& request);

  ~Process();

  int start(); // may throw

  pid_t pid() const { return pid_; }
  const std::string& name() const { return name_; }

  void setTimerId(const muduo::net::TimerId& timerId)
  {
    timerId_ = timerId;
  }

  void onCommandExit(int status, const struct ::rusage&);
  void onTimeout();

  static void onTimeoutWeak(const std::weak_ptr<Process>& wkPtr);

 private:
  void execChild(Pipe& execError, int stdOutput, int stdError)
       __attribute__ ((__noreturn__));;
  int afterFork(Pipe& execError, Pipe& stdOutput, Pipe& stdError);

  muduo::net::EventLoop* loop_;
  RunCommandRequestPtr request_;
  muduo::net::RpcDoneCallback doneCallback_;
  pid_t pid_;
  std::string name_;
  std::string exe_file_;
  muduo::Timestamp startTime_;
  int64_t startTimeInJiffies_;
  muduo::net::TimerId timerId_;
  std::shared_ptr<Sink> stdoutSink_;
  std::shared_ptr<Sink> stderrSink_;
  bool redirectStdout_;
  bool redirectStderr_;
  bool runCommand_;
};
typedef std::shared_ptr<Process> ProcessPtr;

}
#endif  // MUDUO_PROTORPC_ZURG_PROCESS_H
