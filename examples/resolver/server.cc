#include <examples/resolver/resolver.pb.h>

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/cdns/Resolver.h>
#include <muduo/protorpc2/RpcServer.h>

using namespace muduo;
using namespace muduo::net;

namespace resolver
{

class ResolverServiceImpl : public ResolverService
{
 public:
  ResolverServiceImpl(EventLoop* loop)
    : resolver_(loop, cdns::Resolver::kDNSonly)
  {
  }

  virtual void Resolve(const ::resolver::ResolveRequestPtr& request,
                       const ::resolver::ResolveResponse* responsePrototype,
                       const RpcDoneCallback& done)
  {
    LOG_INFO << "ResolverServiceImpl::Resolve " << request->address();

    bool succeed = resolver_.resolve(request->address().c_str(),
                                     std::bind(&ResolverServiceImpl::doneCallback,
                                                 this,
                                                 request->address(),
                                                 _1,
                                                 done));
    if (!succeed)
    {
      ResolveResponse response;
      response.set_resolved(false);
      done(&response);
    }
  }

 private:

  void doneCallback(const std::string& host,
                    const muduo::net::InetAddress& address,
                    const RpcDoneCallback& done)
  {
    LOG_INFO << "ResolverServiceImpl::doneCallback " << host;
    int32_t ip = address.ipNetEndian();
    ResolveResponse response;
    if (ip)
    {
      response.set_resolved(true);
      response.add_ip(ip);
      response.add_port(address.portNetEndian());
    }
    else
    {
      response.set_resolved(false);
    }
    done(&response);
  }

  cdns::Resolver resolver_;
};

}

int main()
{
  LOG_INFO << "pid = " << getpid();
  EventLoop loop;
  // loop.runAfter(30, std::bind(&EventLoop::quit, &loop));
  InetAddress listenAddr(2053);
  resolver::ResolverServiceImpl impl(&loop);
  RpcServer server(&loop, listenAddr);
  server.registerService(&impl);
  server.start();
  loop.loop();
  google::protobuf::ShutdownProtobufLibrary();
}

