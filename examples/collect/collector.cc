#include <examples/collect/collect.pb.h>
#include <examples/collect/ProcFs.h>

#include <muduo/base/LogFile.h>
#include <muduo/base/Logging.h>
#include <muduo/net/inspect/Inspector.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/protobuf/ProtobufCodecLite.h>
#include <muduo/protorpc2/RpcServer.h>

using namespace muduo;

namespace collect
{

extern const char tag[] = "SYS0";
typedef muduo::net::ProtobufCodecLiteT<SystemInfo, tag> Codec;

class CollectLogger : muduo::noncopyable
{
 public:
  CollectLogger(muduo::net::EventLoop* loop, const string& filename)
    : loop_(loop),
      codec_(std::bind(&CollectLogger::onMessage, this, _1, _2, _3)),
      file_(filename, 1000*1000, false, 60, 30) // flush every 60s, check every 30 writes
  {
  }

  void start()
  {
    if (proc_.fill(SnapshotRequest_Level_kSystemInfoInitialSnapshot, &info_))
    {
      LOG_INFO << info_.DebugString();
      save();
      info_.Clear();
    }
    loop_->runEvery(1.0, std::bind(&CollectLogger::snapshot, this));
  }

  bool fill(SnapshotRequest_Level level, SystemInfo* info)
  {
    return proc_.fill(level, info);
  }

  void snapshot()
  {
    if (proc_.fill(SnapshotRequest_Level_kSystemInfoAndThreads, &info_))
    {
      save();
      info_.Clear();
    }
  }

 private:
  void onMessage(const muduo::net::TcpConnectionPtr&,
                 const SystemInfoPtr& messagePtr,
                 muduo::Timestamp receiveTime)
  {
  }

  void save()
  {
    codec_.fillEmptyBuffer(&output_, info_);
    // LOG_INFO << output_.readableBytes();
    file_.append(output_.peek(), static_cast<int>(output_.readableBytes()));
    output_.retrieveAll();
  }

  muduo::net::EventLoop* loop_;
  ProcFs proc_;
  Codec codec_;
  muduo::LogFile file_;
  muduo::net::Buffer output_;  // for scratch
  SystemInfo info_;  // for scratch
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

  Inspector ins(&loop, InetAddress(12345), "collector");
  ins.remove("pprof", "profile"); // remove 30s blocking

  collect::CollectLogger logger(&loop, "collector");
  logger.start();

  collect::CollectServiceImpl impl(&logger);
  RpcServer server(&loop, InetAddress(54321));
  server.registerService(&impl);
  server.start();

  loop.loop();
}
