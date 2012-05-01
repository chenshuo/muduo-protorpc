#include <examples/zurg/slave/AppManager.h>

#include <muduo/base/Logging.h>

namespace zurg
{

struct Application
{
  AddApplicationRequestPtr request;
  ApplicationStatus status;
};

}

using namespace zurg;

AppManager::AppManager()
{
}

AppManager::~AppManager()
{
}

void AppManager::add(const AddApplicationRequestPtr& request,
                     const muduo::net::RpcDoneCallback& done)
{
  apps_[request->name()].request = request;
  ApplicationStatus& status = apps_[request->name()].status;

  status.set_name(request->name());
  if (!status.has_state())
  {
    LOG_INFO << "new app";
    status.set_state(kNewApp);
  }

  done(&status);
}
