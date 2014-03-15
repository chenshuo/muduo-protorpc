#include <examples/collect/collect.pb.h>

#include <muduo/net/inspect/Inspector.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/protorpc2/RpcServer.h>

namespace collect
{
class CollectServiceImpl : public CollectService
{
 public:
  virtual void getSnapshot(const ::collect::SnapshotRequestPtr& request,
                           const ::collect::SystemInfo* responsePrototype,
                           const ::muduo::net::RpcDoneCallback& done)
  {
    SystemInfo response;
    // fill in
    done(&response);
  }
};
}

using namespace muduo::net;

int main(int argc, char* argv[])
{
  EventLoop loop;
  Inspector ins(&loop, InetAddress(12345), "test");
  ins.remove("pprof", "profile"); // remove 30s blocking
  collect::CollectServiceImpl impl;
  RpcServer server(&loop, InetAddress(54321));
  server.registerService(&impl);
  server.start();
  loop.loop();
}
