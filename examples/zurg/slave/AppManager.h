#ifndef MUDUO_PROTORPC_ZURG_APPMANAGER_H
#define MUDUO_PROTORPC_ZURG_APPMANAGER_H

#include <examples/zurg/proto/slave.pb.h>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <string>

namespace muduo
{
namespace net
{
class EventLoop;
}
}

struct rusage;

namespace zurg
{

struct Application;
class ChildManager;
//typedef boost::shared_ptr<Application> ApplicationPtr;
class Process;
typedef boost::shared_ptr<Process> ProcessPtr;

class AppManager : boost::noncopyable
{
 public:
  AppManager(muduo::net::EventLoop*, ChildManager*);
  ~AppManager();

  void add(const AddApplicationRequestPtr& request,
           const muduo::net::RpcDoneCallback& done);

  void start(const StartApplicationsRequestPtr& request,
             const muduo::net::RpcDoneCallback& done);

  void stop(const StopApplicationRequestPtr& request,
            const muduo::net::RpcDoneCallback& done);

  void get(const GetApplicationsRequestPtr& request,
           const muduo::net::RpcDoneCallback& done);

  void list(const ListApplicationsRequestPtr& request,
            const muduo::net::RpcDoneCallback& done);

  void remove(const RemoveApplicationsRequestPtr& request,
              const muduo::net::RpcDoneCallback& done);

 private:
  void startApp(Application* app, ApplicationStatus* out);
  void onProcessExit(const ProcessPtr&, int status, const struct rusage&);

  typedef std::map<std::string, Application> AppMap;
  muduo::net::EventLoop* loop_;
  ChildManager* children_;
  AppMap apps_;
};

}

#endif  // MUDUO_PROTORPC_ZURG_APPMANAGER_H
