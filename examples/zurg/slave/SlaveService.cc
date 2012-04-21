#include <examples/zurg/slave/SlaveService.h>

#include <examples/zurg/slave/ChildManager.h>
#include <examples/zurg/slave/Process.h>

#include <muduo/base/FileUtil.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <boost/weak_ptr.hpp>

using namespace muduo::net;
using namespace zurg;

SlaveServiceImpl::SlaveServiceImpl(EventLoop* loop)
  : loop_(loop),
    children_(new ChildManager(loop_))
{
}

SlaveServiceImpl::~SlaveServiceImpl()
{
}

void SlaveServiceImpl::start()
{
  children_->start();
}

void SlaveServiceImpl::getFileContent(const GetFileContentRequestPtr& request,
    const GetFileContentResponse* responsePrototype,
    const RpcDoneCallback& done)
{
  LOG_INFO << "SlaveServiceImpl::getFileContent - " << request->file_name()
    << " maxSize = " << request->max_size();
  GetFileContentResponse response;
  int64_t file_size = 0;
  int err = muduo::FileUtil::readFile(request->file_name(),
                                      request->max_size(),
                                      response.mutable_content(),
                                      &file_size);
  response.set_error_code(err);
  response.set_file_size(file_size);

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

