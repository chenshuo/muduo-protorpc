#ifndef MUDUO_PROTORPC_ZURG_HEARTBEAT_H
#define MUDUO_PROTORPC_ZURG_HEARTBEAT_H

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

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
class ProcFs;

class Heartbeat : boost::noncopyable
{
 public:
  Heartbeat(muduo::net::EventLoop* loop,
            const SlaveConfig& config,
            MasterService_Stub* stub);
  ~Heartbeat();
  void start();
  void stop();

 private:
  void onTimer();

  void beat(bool showStatic);

  muduo::net::EventLoop* loop_;
  const std::string name_;
  const int port_;
  MasterService_Stub* stub_;
  boost::scoped_ptr<ProcFs> procFs_;
  bool beating_;
};

}
#endif  // MUDUO_PROTORPC_ZURG_HEARTBEAT_H
