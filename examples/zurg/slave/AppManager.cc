#include <examples/zurg/slave/AppManager.h>

#include <examples/zurg/slave/ChildManager.h>
#include <examples/zurg/slave/Process.h>

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

AppManager::AppManager(muduo::net::EventLoop* loop, ChildManager* children)
  : loop_(loop),
    children_(children)
{
}

AppManager::~AppManager()
{
}

void AppManager::add(const AddApplicationRequestPtr& request,
                     const muduo::net::RpcDoneCallback& done)
{
  assert(request->name().find('/') == std::string::npos); // FIXME
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

void AppManager::start(const StartApplicationsRequestPtr& request,
                       const muduo::net::RpcDoneCallback& done)
{
  StartApplicationsResponse response;
  for (int i = 0; i < request->names_size(); ++i)
  {
    const std::string& appName = request->names(i);
    AppMap::iterator it = apps_.find(appName);
    if (it != apps_.end())
    {
      const AddApplicationRequestPtr& appRequest(it->second.request);
      const ApplicationStatus& old_status(it->second.status);
      ApplicationStatus* status = response.add_status();
      status->CopyFrom(old_status);

      if (old_status.state() != kRunning)
      {
        ProcessPtr process(new Process(loop_, appRequest, done));
        int err = 12; // ENOMEM;
        try
        {
          err = process->start();
        }
        catch (...)
        {
        }

        if (err)
        {
          status->set_state(kError);
          // FIXME
        }
        else
        {
          status->set_state(kRunning);
          status->set_pid(process->pid());
          // FIXME
          children_->runAtExit(process->pid(),  // bind strong ptr
              boost::bind(&AppManager::onProcessExit, this, process, _1, _2));
        }
      }
      else
      {
        // already running
      }
    }
    else
    {
      // application not found
      ApplicationStatus* status = response.add_status();
      status->set_state(kUnknown);
      status->set_name(appName);
    }
  }
  done(&response);
}

void AppManager::stop(const StopApplicationRequestPtr& request,
                      const muduo::net::RpcDoneCallback& done)
{
}

void AppManager::onProcessExit(const ProcessPtr&, int status, const struct rusage&)
{
}
