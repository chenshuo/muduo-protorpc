#ifndef MUDUO_PROTORPC_ZURG_SLAVEAPP_H
#define MUDUO_PROTORPC_ZURG_SLAVEAPP_H

#include <muduo/net/EventLoop.h>
#include <muduo/protorpc2/RpcServer.h>

#include <boost/noncopyable.hpp>

namespace zurg
{

class SlaveConfig;
class SlaveServiceImpl;

class SlaveApp : boost::noncopyable
{
 public:
  explicit SlaveApp(const SlaveConfig&);
  ~SlaveApp();

  void run();

 private:
  muduo::net::EventLoop loop_;
  boost::scoped_ptr<SlaveServiceImpl> service_;
  boost::scoped_ptr<muduo::net::RpcServer> server_;
};

}
#endif  // MUDUO_PROTORPC_ZURG_SLAVEAPP_H
