#ifndef MUDUO_PROTORPC_ZURG_HEARTBEAT_H
#define MUDUO_PROTORPC_ZURG_HEARTBEAT_H

#include <boost/noncopyable.hpp>

#include <muduo/base/Types.h>
#include <string>

namespace muduo
{
namespace net
{
class EventLoop;
}
}

namespace zurg
{
class MasterService_Stub;
class SlaveConfig;

class Heartbeat : boost::noncopyable
{
 public:
  Heartbeat(muduo::net::EventLoop* loop,
            const SlaveConfig& config,
            MasterService_Stub* stub);
  void start();
  void stop();

 private:
  void onTimer();

  void beat(bool firstTime);

  muduo::net::EventLoop* loop_;
  std::string name_;
  MasterService_Stub* stub_;
  bool beating_;
};

}
#endif  // MUDUO_PROTORPC_ZURG_HEARTBEAT_H
