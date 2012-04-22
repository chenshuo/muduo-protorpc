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

  virtual void getFileContent(const GetFileContentRequestPtr& request,
                              const GetFileContentResponse* responsePrototype,
                              const RpcDoneCallback& done);

  virtual void runCommand(const RunCommandRequestPtr& request,
                          const RunCommandResponse* responsePrototype,
                          const RpcDoneCallback& done);

 private:
  muduo::net::EventLoop* loop_;
  boost::scoped_ptr<ChildManager> children_;
};

}
#endif  // MUDUO_PROTORPC_ZURG_SLAVESERVICE_H
