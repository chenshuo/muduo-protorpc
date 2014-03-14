#ifndef MUDUO_PROTORPC_ZURG_GETHARDWARETASK_H
#define MUDUO_PROTORPC_ZURG_GETHARDWARETASK_H

#include <muduo/base/Logging.h>

namespace zurg
{

class GetHardwareTask;
typedef std::shared_ptr<GetHardwareTask> GetHardwareTaskPtr;

class GetHardwareTask : public std::enable_shared_from_this<GetHardwareTask>,
                        muduo::noncopyable
{
 public:
  GetHardwareTask(const GetHardwareRequestPtr& request, const muduo::net::RpcDoneCallback& done)
    : lshw_(request->lshw()),
      lspciDone_(false),
      lscpuDone_(false),
      ifconfigDone_(false),
      lshwDone_(false),
      done_(done)
  {
    LOG_DEBUG << this;
  }

  ~GetHardwareTask()
  {
    LOG_DEBUG << this;
  }

  void start(SlaveServiceImpl* slave)
  {
    using std::placeholders::_1;
    GetHardwareTaskPtr thisPtr(shared_from_this());

    if (lshw_)
    {
      RunCommandRequestPtr lshw(new RunCommandRequest);
      lshw->set_command("lshw");
      slave->runCommand(lshw, NULL,  std::bind(&GetHardwareTask::lshwDone, thisPtr, _1));
    }

    RunCommandRequestPtr lspci(new RunCommandRequest);
    lspci->set_command("lspci");
    slave->runCommand(lspci, NULL,  std::bind(&GetHardwareTask::lspciDone, thisPtr, _1));

    RunCommandRequestPtr lscpu(new RunCommandRequest);
    lscpu->set_command("lscpu");
    slave->runCommand(lscpu, NULL,  std::bind(&GetHardwareTask::lscpuDone, thisPtr, _1));

    RunCommandRequestPtr ifconfig(new RunCommandRequest);
    ifconfig->set_command("/sbin/ifconfig");
    slave->runCommand(ifconfig, NULL,  std::bind(&GetHardwareTask::ifconfigDone, thisPtr, _1));
  }

 private:

#define DEFINE_DONE(KEY)                                        \
  void KEY##Done(const google::protobuf::Message* message)      \
  {                                                             \
    assert(KEY##Done_ == false);                                \
    KEY##Done_ = true;                                          \
    const zurg::RunCommandResponse* out =                       \
      google::protobuf::down_cast<const zurg::RunCommandResponse*>(message);    \
    if (out->error_code() == 0)                                 \
    {                                                           \
      resp_.set_##KEY(out->std_output());                       \
    }                                                           \
    checkAllDone();                                             \
  }

  DEFINE_DONE(lspci)
  DEFINE_DONE(lscpu)
  DEFINE_DONE(lshw)
  DEFINE_DONE(ifconfig)

#undef DEFINE_DONE

  void checkAllDone()
  {
    LOG_DEBUG << this;
    bool allDone = lspciDone_ && lscpuDone_ && ifconfigDone_ && (lshw_ == false || lshwDone_);
    if (allDone)
    {
      LOG_INFO << "GetHardwareTask all done";
      done_(&resp_);
    }
  }

  bool lshw_;
  bool lspciDone_;
  bool lscpuDone_;
  bool ifconfigDone_;
  bool lshwDone_;
  GetHardwareResponse resp_;
  muduo::net::RpcDoneCallback done_;
};

}

#endif  // MUDUO_PROTORPC_ZURG_GETHARDWARETASK_H
