#include <examples/zurg/slave/RpcClient.h>

#include <examples/zurg/slave/Heartbeat.h>
#include <examples/zurg/slave/SlaveConfig.h>

#include <muduo/base/Logging.h>
#include <muduo/net/Endian.h>

#include <netdb.h>

using namespace zurg;
using namespace muduo;
using namespace muduo::net;

namespace zurg
{

InetAddress syncResolve(const string& hostport)
{
  sockaddr_in addr;
  bzero(&addr, sizeof addr);
  addr.sin_family = AF_INET;
  size_t pos = hostport.find(':');
  if (pos != string::npos)
  {
    string host(hostport.begin(), hostport.begin() + pos);
    struct hostent* hent = gethostbyname(host.c_str());
    if (hent)
    {
      addr.sin_addr = *reinterpret_cast<in_addr*>(hent->h_addr);
    }

    const char* port = hostport.c_str()+pos+1;
    char* end = NULL;
    long portHostEndian = strtol(port, &end, 10);
    if (end == &*hostport.end())
    {
      addr.sin_port = sockets::hostToNetwork16(static_cast<uint16_t>(portHostEndian));
    }
    else
    {
      struct servent* sent = getservbyname(port, "tcp");
      if (sent)
      {
        addr.sin_port = static_cast<in_port_t>(sent->s_port);
      }
    }
  }
  return InetAddress(addr);
}

}

RpcClient::RpcClient(EventLoop* loop, const SlaveConfig& config)
  : loop_(loop),
    channel_(new RpcChannel),
    stub_(get_pointer(channel_)),
    heartbeat_(new Heartbeat(loop, config, &stub_))
{
  const string& masterAddr = config.masterAddress_;
  LOG_DEBUG << "syncResolve " << masterAddr;
  InetAddress addr = syncResolve(masterAddr);
  LOG_DEBUG << masterAddr << " resolved to " << addr.toIpPort();
  if (addr.ipNetEndian() > 0 && addr.portNetEndian() > 0)
  {
    client_.reset(new TcpClient(loop, addr, "RpcClient"));
  }
  else
  {
    LOG_FATAL << "Unable to resolve master address " << masterAddr;
  }

  client_->setConnectionCallback(
      std::bind(&RpcClient::onConnection, this, _1));
  client_->setMessageCallback(
      std::bind(&RpcChannel::onMessage, get_pointer(channel_), _1, _2, _3));
  client_->enableRetry();
}

RpcClient::~RpcClient()
{
}

void RpcClient::onConnection(const muduo::net::TcpConnectionPtr& conn)
{
  if (conn->connected())
  {
    channel_->setConnection(conn);
    heartbeat_->start();
  }
  else
  {
    heartbeat_->stop();
    channel_->onDisconnect();
  }
}

