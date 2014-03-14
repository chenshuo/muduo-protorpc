#ifndef MUDUO_PROTORPC_ZURG_SLAVESERVICE_H
#define MUDUO_PROTORPC_ZURG_SLAVESERVICE_H

#include <examples/zurg/proto/slave.pb.h>

namespace muduo
{
namespace net
{
class EventLoop;
}
}

namespace zurg
{

class AppManager;
class ChildManager;

class SlaveServiceImpl : public SlaveService
{
 public:
  SlaveServiceImpl(muduo::net::EventLoop* loop, int zombieInterval);
  ~SlaveServiceImpl();

  void start();

  virtual void getHardware(const GetHardwareRequestPtr& request,
                           const GetHardwareResponse* responsePrototype,
                           const RpcDoneCallback& done);

  virtual void getFileContent(const GetFileContentRequestPtr& request,
                              const GetFileContentResponse* responsePrototype,
                              const RpcDoneCallback& done);

  virtual void getFileChecksum(const GetFileChecksumRequestPtr& request,
                               const GetFileChecksumResponse* responsePrototype,
                               const RpcDoneCallback& done);

  virtual void listProcesses(const ListProcessesRequestPtr& request,
                             const ListProcessesResponse* responsePrototype,
                             const RpcDoneCallback& done);

  virtual void runCommand(const RunCommandRequestPtr& request,
                          const RunCommandResponse* responsePrototype,
                          const RpcDoneCallback& done);

  virtual void runScript(const RunScriptRequestPtr& request,
                         const RunCommandResponse* responsePrototype,
                         const RpcDoneCallback& done);

  virtual void addApplication(const AddApplicationRequestPtr& request,
                              const AddApplicationResponse* responsePrototype,
                              const RpcDoneCallback& done);

  virtual void startApplications(const StartApplicationsRequestPtr& request,
                                 const StartApplicationsResponse* responsePrototype,
                                 const RpcDoneCallback& done);

  virtual void stopApplication(const StopApplicationRequestPtr& request,
                               const StopApplicationResponse* responsePrototype,
                               const RpcDoneCallback& done);

  virtual void getApplications(const GetApplicationsRequestPtr& request,
                               const GetApplicationsResponse* responsePrototype,
                               const RpcDoneCallback& done);

  virtual void listApplications(const ListApplicationsRequestPtr& request,
                                const ListApplicationsResponse* responsePrototype,
                                const RpcDoneCallback& done);

  virtual void removeApplications(const RemoveApplicationsRequestPtr& request,
                                  const RemoveApplicationsResponse* responsePrototype,
                                  const RpcDoneCallback& done);

 private:
  void getFileChecksumDone(const GetFileChecksumRequestPtr& request,
                           const google::protobuf::Message* response,
                           const RpcDoneCallback& done);

  muduo::net::EventLoop* loop_;
  std::unique_ptr<ChildManager> children_;
  std::unique_ptr<AppManager> apps_;
};

}
#endif  // MUDUO_PROTORPC_ZURG_SLAVESERVICE_H
