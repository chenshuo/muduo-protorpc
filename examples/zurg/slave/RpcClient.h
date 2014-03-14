#ifndef MUDUO_PROTORPC_ZURG_RPCCLIENT_H
#define MUDUO_PROTORPC_ZURG_RPCCLIENT_H

#include <muduo/net/TcpClient.h>

#include <examples/zurg/proto/master.pb.h>

namespace muduo
{
namespace net
{
class EventLoop;
}
}

namespace zurg
{

muduo::net::InetAddress syncResolve(const muduo::string& hostport);
class Heartbeat;
class SlaveConfig;

class RpcClient : muduo::noncopyable
{
 public:
  RpcClient(muduo::net::EventLoop* loop,
            const SlaveConfig& config);
  ~RpcClient();

  void connect()
  {
    client_->connect();
  }

 private:
  void onConnection(const muduo::net::TcpConnectionPtr& conn);

  muduo::net::EventLoop* loop_;
  std::unique_ptr<muduo::net::TcpClient> client_;
  muduo::net::RpcChannelPtr channel_;
  MasterService::Stub stub_;
  std::unique_ptr<Heartbeat> heartbeat_;
};

}

#endif  // MUDUO_PROTORPC_ZURG_RPCCLIENT_H
