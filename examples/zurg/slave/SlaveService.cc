#include <examples/zurg/slave/SlaveService.h>

#include <examples/zurg/slave/AppManager.h>
#include <examples/zurg/slave/ChildManager.h>
#include <examples/zurg/slave/GetHardwareTask.h>
#include <examples/zurg/slave/Process.h>

#include <examples/zurg/common/Util.h>

#include <muduo/base/FileUtil.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <boost/weak_ptr.hpp>

using namespace muduo::net;
using namespace zurg;

SlaveServiceImpl::SlaveServiceImpl(EventLoop* loop, int zombieInterval)
  : loop_(loop),
    apps_(new AppManager),
    children_(new ChildManager(loop_, zombieInterval))
{
}

SlaveServiceImpl::~SlaveServiceImpl()
{
}

void SlaveServiceImpl::start()
{
  children_->start();
}

void SlaveServiceImpl::getHardware(const GetHardwareRequestPtr& request,
                                   const GetHardwareResponse* responsePrototype,
                                   const RpcDoneCallback& done)
{
  LOG_INFO << "SlaveServiceImpl::getHardware - lshw = " << request->lshw();

  GetHardwareTaskPtr task(new GetHardwareTask(request, done));
  task->start(this);
}

void SlaveServiceImpl::getFileContent(const GetFileContentRequestPtr& request,
                                      const GetFileContentResponse* responsePrototype,
                                      const RpcDoneCallback& done)
{
  LOG_INFO << "SlaveServiceImpl::getFileContent - " << request->file_name()
           << " maxSize = " << request->max_size();
  GetFileContentResponse response;
  int64_t file_size = 0;
  int64_t modify_time = 0;
  int64_t create_time = 0;
  int err = muduo::FileUtil::readFile(request->file_name(),
                                      request->max_size(),
                                      response.mutable_content(),
                                      &file_size,
                                      &modify_time,
                                      &create_time);
  response.set_error_code(err);
  response.set_file_size(file_size);
  response.set_modify_time(modify_time);
  response.set_create_time(create_time);

  LOG_DEBUG << "SlaveServiceImpl::getFileContent - err " << err;
  done(&response);
}

void SlaveServiceImpl::runCommand(const RunCommandRequestPtr& request,
                                  const RunCommandResponse* responsePrototype,
                                  const RpcDoneCallback& done)
{
  LOG_INFO << "SlaveServiceImpl::runCommand - " << request->command();
  ProcessPtr process(new Process(loop_, request, done));
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
    RunCommandResponse response;
    response.set_error_code(err);
    done(&response);
  }
  else
  {
    children_->runAtExit(process->pid(),  // bind strong ptr
                         boost::bind(&Process::onExit, process, _1, _2));
    boost::weak_ptr<Process> weakProcessPtr(process);
    TimerId timerId = loop_->runAfter(request->timeout(),
                                      boost::bind(&Process::onTimeoutWeak, weakProcessPtr));
    process->setTimerId(timerId);
  }
}

void SlaveServiceImpl::runScript(const RunScriptRequestPtr& request,
                                 const RunCommandResponse* responsePrototype,
                                 const RpcDoneCallback& done)
{
  RunCommandRequestPtr runCommandReq(new RunCommandRequest);

  std::string scriptFile = writeTempFile(request->script());
  LOG_INFO << "runScript - write to " << scriptFile;
  //FIXME: interpreter
  runCommandReq->set_command(scriptFile);
  // FIXME: others
  runCommand(runCommandReq, NULL, done);
}

void SlaveServiceImpl::addApplication(const AddApplicationRequestPtr& request,
                                      const ApplicationStatus* responsePrototype,
                                      const RpcDoneCallback& done)
{
  apps_->add(request, done);
}

void SlaveServiceImpl::startApplication(const StartApplicationRequestPtr& request,
                                const StartApplicationResponse* responsePrototype,
                                const RpcDoneCallback& done)
{
}

void SlaveServiceImpl::stopApplication(const StopApplicationRequestPtr& request,
                               const StopApplicationResponse* responsePrototype,
                               const RpcDoneCallback& done)
{
}

