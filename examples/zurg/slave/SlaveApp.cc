#include <examples/zurg/slave/SlaveApp.h>

#include <examples/zurg/slave/Heartbeat.h>
#include <examples/zurg/slave/SlaveConfig.h>
#include <examples/zurg/slave/SlaveService.h>

#include <muduo/protorpc2/RpcServer.h>

#include <muduo/base/Logging.h>

#include <assert.h>

namespace zurg
{

class RpcClient : boost::noncopyable
{
 public:
  RpcClient(muduo::net::EventLoop* loop, const muduo::string& masterAddr)
  {
  }
};

}

using namespace zurg;
using namespace muduo::net;

SlaveApp::SlaveApp(const SlaveConfig& config)
  : loop_(),
    config_(config),
    rpcClient_(new RpcClient(&loop_, config.masterAddress_)),
    service_(new SlaveServiceImpl(&loop_, config.zombieInterval_)),
    heartbeat_(new Heartbeat)
{
  assert(config.valid());

  LOG_INFO << "Start zurg_slave";
  if (config.listenPort_ > 0)
  {
    InetAddress listenAddress(static_cast<uint16_t>(config.listenPort_));
    server_.reset(new RpcServer(&loop_, listenAddress));
    server_->registerService(get_pointer(service_));
    LOG_INFO << "Listen on port " << listenAddress.toIpPort();
  }

  // FIXME: connect to master
}

SlaveApp::~SlaveApp()
{
}

void SlaveApp::run()
{
  service_->start();
  heartbeat_->start();
  if (server_)
  {
    server_->start();
  }
  loop_.loop();
}
