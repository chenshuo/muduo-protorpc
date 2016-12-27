#include <examples/collect/collect.pb.h>
#include <examples/collect/ProcFs.h>

#include <muduo/base/LogFile.h>
#include <muduo/base/Logging.h>
#include <muduo/net/inspect/Inspector.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/protobuf/ProtobufCodecLite.h>
#include <muduo/protorpc2/RpcServer.h>
// #include <google/malloc_hook.h>

#include <signal.h>

extern const char* g_build_version;

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
      file_(filename, 100*1000*1000, false, 60, 30) // flush every 60s, check every 30 writes
  {
  }

  void start()
  {
    if (proc_.fill(SnapshotRequest_Level_kSystemInfoInitialSnapshot, &info_))
    {
      printf("%s\n", info_.DebugString().c_str());
      save();
      file_.flush();
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

  void flush()
  {
    file_.flush();
  }

  bool roll()
  {
    return file_.rollFile();
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
    LOG_DEBUG << output_.readableBytes();
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
  CollectServiceImpl(muduo::net::EventLoop* loop, CollectLogger* logger, int argc, char* argv[])
    : loop_(loop),
      logger_(logger),
      allowQuit_(muduo::ProcessInfo::isDebugBuild()),
      argc_(argc),
      argv_(argv),
      executable_(muduo::ProcessInfo::exePath())
  {
    LOG_INFO << "Executable: " << executable_;
  }

  virtual void getSnapshot(const ::collect::SnapshotRequestPtr& request,
                           const ::collect::SystemInfo* responsePrototype,
                           const ::muduo::net::RpcDoneCallback& done) // override
  {
    SystemInfo response;
    logger_->fill(request->level(), &response);
    done(&response);
  }

  virtual void flushFile(const ::rpc2::EmptyPtr& request,
                         const ::rpc2::Empty* responsePrototype,
                         const ::muduo::net::RpcDoneCallback& done) // override
  {
    logger_->flush();
    ::rpc2::Empty response;
    done(&response);
  }

  virtual void rollFile(const ::rpc2::EmptyPtr& request,
                        const ::collect::Result* responsePrototype,
                        const ::muduo::net::RpcDoneCallback& done) // override
  {
    Result response;
    response.set_succeed(logger_->roll());
    done(&response);
  }

  virtual void version(const ::rpc2::EmptyPtr& request,
                       const ::collect::Result* responsePrototype,
                       const ::muduo::net::RpcDoneCallback& done) // override
  {
    Result response;
    response.set_succeed(true);
    response.set_message(g_build_version);
    done(&response);
  }

  virtual void quit(const ::rpc2::EmptyPtr& request,
                    const ::collect::Result* responsePrototype,
                    const ::muduo::net::RpcDoneCallback& done) // override
  {
    Result response;
    response.set_succeed(allowQuit_);
    done(&response);
    if (allowQuit_)
    {
      loop_->quit();
    }
  }

  virtual void restart(const ::rpc2::EmptyPtr& request,
                       const ::collect::Result* responsePrototype,
                       const ::muduo::net::RpcDoneCallback& done) // override
  {
    Result response;
    response.set_succeed(false);
    done(&response);
    // FIXME: fork, close listening ports, exec, reopen listening ports if failed
  }

 private:
  muduo::net::EventLoop* loop_;
  CollectLogger* logger_;
  const bool allowQuit_;
  // saved for exec()
  const int argc_;
  char** argv_;
  const muduo::string executable_;
};
}

using namespace muduo::net;
EventLoop* g_loop;

void sighandler(int)
{
  g_loop->quit();
}

void NewHook(const void* ptr, size_t size)
{
  printf("alloc %zd %p\n", size, ptr);
}

int main(int argc, char* argv[])
{
  EventLoop loop;
  g_loop = &loop;
  signal(SIGINT, sighandler);
  signal(SIGTERM, sighandler);
  // MallocHook::SetNewHook(&NewHook);
  LOG_INFO << "Version: " << g_build_version;

  collect::CollectLogger logger(&loop, "collector");
  logger.start();

  collect::CollectServiceImpl impl(&loop, &logger, argc, argv);
  std::unique_ptr<RpcServer> server;
  if (argc > 1)
  {
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    server.reset(new RpcServer(&loop, InetAddress(port)));
    server->registerService(&impl);
    server->start();
  }

  std::unique_ptr<Inspector> ins;
  if (argc > 2)
  {
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    ins.reset(new Inspector(&loop, InetAddress(port), "collector"));
    ins->remove("pprof", "profile"); // remove 30s blocking
  }

  loop.loop();
  google::protobuf::ShutdownProtobufLibrary();
}
