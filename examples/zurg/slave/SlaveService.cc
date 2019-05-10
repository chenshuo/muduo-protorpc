#include <examples/zurg/slave/SlaveService.h>

#include <examples/zurg/slave/AppManager.h>
#include <examples/zurg/slave/ChildManager.h>
#include <examples/zurg/slave/GetHardwareTask.h>
#include <examples/zurg/slave/Process.h>
#include <examples/zurg/slave/SlaveApp.h>

#include <examples/zurg/common/Util.h>

#include <muduo/base/FileUtil.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

using namespace muduo::net;
using namespace zurg;
using std::placeholders::_1;
using std::placeholders::_2;

SlaveServiceImpl::SlaveServiceImpl(EventLoop* loop, int zombieInterval)
  : loop_(loop),
    children_(new ChildManager(loop_, zombieInterval)),
    apps_(new AppManager(loop_, get_pointer(children_)))
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
  int err = muduo::FileUtil::readFile(request->file_name().c_str(),
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

void SlaveServiceImpl::getFileChecksum(const GetFileChecksumRequestPtr& request,
                                       const GetFileChecksumResponse* responsePrototype,
                                       const RpcDoneCallback& done)
{
  LOG_DEBUG ;
  if (request->files_size() > 0)
  {
    RunCommandRequestPtr runCommandReq(new RunCommandRequest);
    runCommandReq->set_command("md5sum");
    for (int i = 0; i < request->files_size(); ++i)
    {
      assert(request->files(i).find('\n') == std::string::npos);
      runCommandReq->add_args(request->files(i));
    }
    runCommand(runCommandReq, NULL,
        std::bind(&SlaveServiceImpl::getFileChecksumDone, this, request, _1, done));
  }
  else
  {
    GetFileChecksumResponse response;
    done(&response);
  }
}

void SlaveServiceImpl::getFileChecksumDone(const GetFileChecksumRequestPtr& request,
                                           const google::protobuf::Message* message,
                                           const RpcDoneCallback& done)
{
  const zurg::RunCommandResponse* runCommandResp =
      google::protobuf::down_cast<const zurg::RunCommandResponse*>(message);

  const std::string& lines = runCommandResp->std_output();
  std::map<muduo::StringPiece, muduo::StringPiece> md5sums;
  parseMd5sum(lines, &md5sums);

  GetFileChecksumResponse response;
  for (int i = 0; i < request->files_size(); ++i)
  {
    md5sums[request->files(i)].CopyToString(response.add_md5sums());
  }
  done(&response);
}

void SlaveServiceImpl::listProcesses(const ListProcessesRequestPtr& request,
                                     const ListProcessesResponse* responsePrototype,
                                     const RpcDoneCallback& done)
{
  ListProcessesResponse response;
  if (request->get_load())
  {
  }
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
                         std::bind(&Process::onCommandExit, process, _1, _2));
    std::weak_ptr<Process> weakProcessPtr(process);
    TimerId timerId = loop_->runAfter(request->timeout(),
                                      std::bind(&Process::onTimeoutWeak, weakProcessPtr));
    process->setTimerId(timerId);
  }
}

void SlaveServiceImpl::runScript(const RunScriptRequestPtr& request,
                                 const RunCommandResponse* responsePrototype,
                                 const RpcDoneCallback& done)
{
  RunCommandRequestPtr runCommandReq(new RunCommandRequest);

  std::string scriptFile = writeTempFile(SlaveApp::instance().name(), request->script());
  if (!scriptFile.empty())
  {
    LOG_INFO << "runScript - write to " << scriptFile;
    // FIXME: interpreter
    runCommandReq->set_command(scriptFile);
    // FIXME: others
    runCommand(runCommandReq, NULL, done);
  }
  else
  {
    LOG_ERROR << "runScript - failed to write script file";
    // FIXME: done
  }
}

void SlaveServiceImpl::addApplication(const AddApplicationRequestPtr& request,
                                      const AddApplicationResponse* responsePrototype,
                                      const RpcDoneCallback& done)
{
  apps_->add(request, done);
}

void SlaveServiceImpl::startApplications(const StartApplicationsRequestPtr& request,
                                         const StartApplicationsResponse* responsePrototype,
                                         const RpcDoneCallback& done)
{
  apps_->start(request, done);
}

void SlaveServiceImpl::stopApplication(const StopApplicationRequestPtr& request,
                                       const StopApplicationResponse* responsePrototype,
                                       const RpcDoneCallback& done)
{
  apps_->stop(request, done);
}

void SlaveServiceImpl::getApplications(const GetApplicationsRequestPtr& request,
                                       const GetApplicationsResponse* responsePrototype,
                                       const RpcDoneCallback& done)
{
}

void SlaveServiceImpl::listApplications(const ListApplicationsRequestPtr& request,
                                        const ListApplicationsResponse* responsePrototype,
                                        const RpcDoneCallback& done)
{
}

void SlaveServiceImpl::removeApplications(const RemoveApplicationsRequestPtr& request,
                                          const RemoveApplicationsResponse* responsePrototype,
                                          const RpcDoneCallback& done)
{
}

