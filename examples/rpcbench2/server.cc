#include <examples/rpcbench2/echo.pb.h>

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/protorpc2/RpcServer.h>

using namespace muduo;
using namespace muduo::net;

namespace echo
{

class EchoServiceImpl : public EchoService
{
 public:
  virtual void Echo(const ::echo::EchoRequestPtr& request,
                    const ::echo::EchoResponse* responsePrototype,
                    const RpcDoneCallback& done)
  {
    //LOG_INFO << "EchoServiceImpl::Solve";
    EchoResponse response;
    response.set_payload(request->payload());
    done(&response);
  }
};

}

int main(int argc, char* argv[])
{
  int nThreads =  argc > 1 ? atoi(argv[1]) : 1;
  LOG_INFO << "pid = " << getpid() << " threads = " << nThreads;
  EventLoop loop;
  InetAddress listenAddr(8888);
  echo::EchoServiceImpl impl;
  RpcServer server(&loop, listenAddr);
  server.setThreadNum(nThreads);
  server.registerService(&impl);
  server.start();
  loop.loop();
}

