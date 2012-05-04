#include <examples/zurg/slave/SlaveApp.h>

#include <examples/zurg/slave/RpcClient.h>
#include <examples/zurg/slave/SlaveConfig.h>
#include <examples/zurg/slave/SlaveService.h>
#include <examples/zurg/common/Util.h>

#include <muduo/protorpc2/RpcServer.h>

#include <muduo/base/Logging.h>

#include <assert.h>

using namespace zurg;
using namespace muduo::net;

SlaveApp* SlaveApp::instance_ = NULL;

SlaveApp::SlaveApp(const SlaveConfig& config)
  : loop_(),
    config_(config),
    rpcClient_(new RpcClient(&loop_, config)), // FIXME: share two timers
    service_(new SlaveServiceImpl(&loop_, config.zombieInterval_))
{
  assert(config.valid());
  assert(instance_ == NULL);
  instance_ = this;

  LOG_INFO << "Start zurg_slave";

  std::string cwd = prefix()+'/'+name();
  setupWorkingDir(cwd);

  if (config.listenPort_ > 0)
  {
    InetAddress listenAddress(static_cast<uint16_t>(config.listenPort_));
    server_.reset(new RpcServer(&loop_, listenAddress));
    server_->registerService(get_pointer(service_));
    LOG_INFO << "Listen on port " << listenAddress.toIpPort();
  }
}

SlaveApp::~SlaveApp()
{
}

const std::string& SlaveApp::name() const
{
  return config_.name_;
}

const std::string& SlaveApp::prefix() const
{
  return config_.prefix_;
}

void SlaveApp::run()
{
  rpcClient_->connect();
  service_->start();
  if (server_)
  {
    server_->start();
  }
  loop_.loop();
}

