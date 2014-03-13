#include <examples/zurg/slave/AppManager.h>

#include <examples/zurg/slave/ChildManager.h>
#include <examples/zurg/slave/Process.h>

#include <muduo/base/Logging.h>

#include <stdio.h>
#include <sys/resource.h>
#include <sys/wait.h>

namespace zurg
{

struct Application
{
  AddApplicationRequestPtr request;
  ApplicationStatus status;
};

}

using namespace zurg;
using std::placeholders::_1;
using std::placeholders::_2;

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
      startApp(&it->second, response.add_status());
    }
    else
    {
      // application not found
      ApplicationStatus* status = response.add_status();
      status->set_state(kUnknown);
      status->set_name(appName);
      status->set_message("Application is unknown.");
    }
  }
  done(&response);
}

void AppManager::startApp(Application* app, ApplicationStatus* out)
{
  const AddApplicationRequestPtr& appRequest(app->request);
  ApplicationStatus* status = &app->status;

  if (status->state() != kRunning)
  {
    ProcessPtr process(new Process(appRequest));
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
          std::bind(&AppManager::onProcessExit, this, process, _1, _2));
    }
    out->CopyFrom(*status);
  }
  else
  {
    out->CopyFrom(*status);
    out->set_message("Already running.");
  }
}

void AppManager::stop(const StopApplicationRequestPtr& request,
                      const muduo::net::RpcDoneCallback& done)
{
}

#pragma GCC diagnostic ignored "-Wold-style-cast"
// FIXME: dup Process::onCommandExit
void AppManager::onProcessExit(const ProcessPtr& process, int status, const struct rusage&)
{
  const std::string& appName = process->name();

  char buf[256];
  if (WIFEXITED(status))
  {
    snprintf(buf, sizeof buf, "exit status %d", WEXITSTATUS(status));
    // response.set_exit_status(WEXITSTATUS(status));
  }
  else if (WIFSIGNALED(status))
  {
    snprintf(buf, sizeof buf, "signaled %d%s",
             WTERMSIG(status), WCOREDUMP(status) ? " (core dump)" : "");
    // response.set_signaled(WTERMSIG(status));
    // response.set_coredump(WCOREDUMP(status));
  }

  LOG_WARN << "AppManager[" << appName << "] onProcessExit - " << buf;

  AppMap::iterator it = apps_.find(appName);
  if (it != apps_.end())
  {
    Application& app = it->second;
    app.status.set_state(kExited);


    // FIXME: notify master
  }
  else
  {
    LOG_ERROR << "AppManager[" << appName << "] - Unknown app ";
  }
}
