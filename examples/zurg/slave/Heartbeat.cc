#include <examples/zurg/slave/Heartbeat.h>

#include <examples/zurg/slave/SlaveConfig.h>

#include <examples/zurg/proto/master.pb.h>
#include <muduo/protorpc2/rpc2.pb.h>

#include <muduo/base/Logging.h>
#include <muduo/base/ProcessInfo.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/EventLoop.h>

namespace zurg
{
void ignoreCallback(const EmptyPtr&)
{
}
}

using namespace zurg;
using namespace muduo;
using namespace muduo::net;

Heartbeat::Heartbeat(muduo::net::EventLoop* loop,
                     const SlaveConfig& config,
                     MasterService_Stub* stub)
  : loop_(loop),
    name_(config.name_),
    stub_(stub),
    beating_(false)
{
  loop_->runEvery(config.heartbeatInterval_, boost::bind(&Heartbeat::onTimer, this));
}

void Heartbeat::start()
{
  beating_ = true;
  beat(true);
}

void Heartbeat::stop()
{
  beating_ = false;
}

void Heartbeat::onTimer()
{
  if (beating_)
  {
    beat(false);
  }
}

void Heartbeat::beat(bool firstTime)
{
  LOG_DEBUG << firstTime;
  SlaveHeartbeat hb;
  hb.set_slave_name(name_);
  if (firstTime)
  {
    hb.set_host_name(ProcessInfo::hostname().c_str());

    hb.set_slave_pid(ProcessInfo::pid());
    hb.set_start_time_us(ProcessInfo::startTime().microSecondsSinceEpoch());
  }
  hb.set_send_time_us(Timestamp::now().microSecondsSinceEpoch());

  stub_->slaveHeartbeat(hb, ignoreCallback);
}
