#include <examples/zurg/slave/SlaveApp.h>

#include <examples/zurg/slave/SlaveConfig.h>
#include <examples/zurg/slave/SlaveService.h>

#include <muduo/protorpc2/RpcServer.h>

#include <muduo/base/Logging.h>

#include <assert.h>

using namespace zurg;
using namespace muduo::net;

SlaveApp::SlaveApp(const SlaveConfig& config)
  : loop_(),
    service_(new SlaveServiceImpl(&loop_))
{
  assert(config.succeed_);

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
  if (server_)
  {
    server_->start();
  }
  loop_.loop();
}
