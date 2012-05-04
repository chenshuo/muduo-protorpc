#ifndef MUDUO_PROTORPC_ZURG_SLAVEAPP_H
#define MUDUO_PROTORPC_ZURG_SLAVEAPP_H

#include <muduo/net/EventLoop.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{
namespace net
{
class RpcServer;
}
}

namespace zurg
{

class RpcClient;
class SlaveConfig;
class SlaveServiceImpl;

class SlaveApp : boost::noncopyable
{
 public:
  explicit SlaveApp(const SlaveConfig&);
  ~SlaveApp();

  void run();

  const std::string& name() const;
  const std::string& prefix() const;
  static SlaveApp& instance() { return *instance_; }

 private:
  muduo::net::EventLoop loop_;
  const SlaveConfig& config_;
  boost::scoped_ptr<RpcClient> rpcClient_;
  boost::scoped_ptr<SlaveServiceImpl> service_;

  // optional
  boost::scoped_ptr<muduo::net::RpcServer> server_;

  static SlaveApp* instance_;
};

}
#endif  // MUDUO_PROTORPC_ZURG_SLAVEAPP_H
