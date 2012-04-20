#ifndef MUDUO_PROTORPC_ZURG_SLAVESERVICE_H
#define MUDUO_PROTORPC_ZURG_SLAVESERVICE_H

#include <examples/zurg/proto/slave.pb.h>

#include <muduo/protorpc2/RpcServer.h>

#include <boost/noncopyable.hpp>

namespace zurg
{

class SlaveServiceImpl : public SlaveService
{
 public:
  explicit SlaveServiceImpl(muduo::net::EventLoop* loop)
    : loop_(loop)
  {
  }

  virtual void getFileContent(const boost::shared_ptr<const ::zurg::GetFileContentRequest>& request,
                              const ::zurg::GetFileContentResponse* responsePrototype,
                              const DoneCallback& done);

 private:
  muduo::net::EventLoop* loop_;
};

}
#endif  // MUDUO_PROTORPC_ZURG_SLAVESERVICE_H
