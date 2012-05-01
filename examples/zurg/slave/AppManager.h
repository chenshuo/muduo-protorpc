#ifndef MUDUO_PROTORPC_ZURG_APPMANAGER_H
#define MUDUO_PROTORPC_ZURG_APPMANAGER_H

#include <examples/zurg/proto/slave.pb.h>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <string>

namespace zurg
{

struct Application;
//typedef boost::shared_ptr<Application> ApplicationPtr;

class AppManager : boost::noncopyable
{
 public:
  AppManager();
  ~AppManager();

  void add(const AddApplicationRequestPtr& request,
           const muduo::net::RpcDoneCallback& done);

 private:
  std::map<std::string, Application> apps_;
};

}

#endif  // MUDUO_PROTORPC_ZURG_APPMANAGER_H
