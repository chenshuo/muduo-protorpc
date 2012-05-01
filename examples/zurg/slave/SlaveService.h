#ifndef MUDUO_PROTORPC_ZURG_SLAVESERVICE_H
#define MUDUO_PROTORPC_ZURG_SLAVESERVICE_H

#include <examples/zurg/proto/slave.pb.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{
namespace net
{
class EventLoop;
}
}

namespace zurg
{

class ChildManager;

class SlaveServiceImpl : public SlaveService
{
 public:
  SlaveServiceImpl(muduo::net::EventLoop* loop, int zombieInterval);
  ~SlaveServiceImpl();

  void start();

  virtual void getHardware(const GetHardwareRequestPtr& request,
                           const GetHardwareResponse* responsePrototype,
                           const RpcDoneCallback& done);

  virtual void getFileContent(const GetFileContentRequestPtr& request,
                              const GetFileContentResponse* responsePrototype,
                              const RpcDoneCallback& done);

  virtual void runCommand(const RunCommandRequestPtr& request,
                          const RunCommandResponse* responsePrototype,
                          const RpcDoneCallback& done);

  virtual void runScript(const RunScriptRequestPtr& request,
                         const RunCommandResponse* responsePrototype,
                         const RpcDoneCallback& done);

  virtual void addApplication(const AddApplicationRequestPtr& request,
                              const AddApplicationResponse* responsePrototype,
                              const RpcDoneCallback& done);

  virtual void startApplication(const StartApplicationRequestPtr& request,
                                const StartApplicationResponse* responsePrototype,
                                const RpcDoneCallback& done);

  virtual void stopApplication(const StopApplicationRequestPtr& request,
                               const StopApplicationResponse* responsePrototype,
                               const RpcDoneCallback& done);

 private:
  muduo::net::EventLoop* loop_;
  boost::scoped_ptr<ChildManager> children_;
};

}
#endif  // MUDUO_PROTORPC_ZURG_SLAVESERVICE_H
