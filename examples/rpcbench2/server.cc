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
  virtual void Echo(const boost::shared_ptr<const EchoRequest>& request,
                    const ::echo::EchoResponse* responsePrototype,
                    const DoneCallback& done)
  {
    //LOG_INFO << "EchoServiceImpl::Solve";
    EchoResponse response;
    response.set_payload(request->payload());
    done(&response);
  }
};

}

int main()
{
  LOG_INFO << "pid = " << getpid();
  EventLoop loop;
  InetAddress listenAddr(8888);
  echo::EchoServiceImpl impl;
  RpcServer server(&loop, listenAddr);
  server.setThreadNum(2);
  server.registerService(&impl);
  server.start();
  loop.loop();
}

