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
  AddApplicationRequestPtr& requestRef = apps_[request->name()].request;
  AddApplicationRequestPtr prev_request(requestRef);
  requestRef = request;
  ApplicationStatus& status = apps_[request->name()].status;

  status.set_name(request->name());
  if (!status.has_state())
  {
    LOG_INFO << "new app";
    status.set_state(kNewApp);
  }

  AddApplicationResponse response;
  response.mutable_status()->CopyFrom(status);
  if (prev_request)
  {
    response.mutable_prev_request()->CopyFrom(*prev_request);
  }
  done(&response);
}
