#include <examples/collect/collect.pb.h>
#include <examples/collect/ProcFs.h>

#include <muduo/base/Logging.h>
#include <muduo/net/inspect/Inspector.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/protorpc2/RpcServer.h>

namespace collect
{

class CollectLogger : muduo::noncopyable
{
 public:
  CollectLogger(muduo::net::EventLoop* loop)
    : loop_(loop)
  {
  }

  void start()
  {
    loop_->runEvery(3.0, std::bind(&CollectLogger::snapshot, this));
  }

  bool fill(SnapshotRequest_Level level, SystemInfo* info)
  {
    return proc_.fill(level, info);
  }

  void snapshot()
  {
    SystemInfo info;
    if (proc_.fill(SnapshotRequest_Level_kSystemInfoAndThreads, &info))
    {
      // save to disk
      LOG_INFO << info.DebugString();
    }
  }

 private:
  muduo::net::EventLoop* loop_;
  ProcFs proc_;
};

class CollectServiceImpl : public CollectService
{
 public:
  CollectServiceImpl(CollectLogger* logger)
    : logger_(logger)
  {
  }

  virtual void getSnapshot(const ::collect::SnapshotRequestPtr& request,
                           const ::collect::SystemInfo* responsePrototype,
                           const ::muduo::net::RpcDoneCallback& done)
  {
    SystemInfo response;
    logger_->fill(request->level(), &response);
    done(&response);
  }
 private:
  CollectLogger* logger_;
};
}

using namespace muduo::net;

int main(int argc, char* argv[])
{
  EventLoop loop;

  Inspector ins(&loop, InetAddress(12345), "test");
  ins.remove("pprof", "profile"); // remove 30s blocking

  collect::CollectLogger logger(&loop);
  logger.start();

  collect::CollectServiceImpl impl(&logger);
  RpcServer server(&loop, InetAddress(54321));
  server.registerService(&impl);
  server.start();

  loop.loop();
}
