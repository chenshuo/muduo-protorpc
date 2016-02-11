#include <examples/rpcbench2/echo.pb.h>

#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Logging.h>
#include <muduo/base/ThreadLocalSingleton.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThreadPool.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/TcpConnection.h>

#include <boost/ptr_container/ptr_vector.hpp>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

std::string g_payload = "0123456789ABCDEF";

class PerThreadMeter : noncopyable
{
 public:
  PerThreadMeter()
    : loop_(NULL),
      lastTotal_(0),
      lastTime_(Timestamp::now())
  {
  }

  void setLoop(EventLoop* loop) { loop_ = loop; }

  void addConnection(int64_t* c)
  {
    loop_->assertInLoopThread();
    counts_.push_back(c);
  }

  void removeConnection(int64_t* c)
  {
    loop_->assertInLoopThread();
    std::vector<int64_t*>::iterator it = std::find(counts_.begin(), counts_.end(), c);
    assert(it != counts_.end());
    std::swap(*it, counts_.back());
    counts_.pop_back();
  }

  int connections() const
  {
    loop_->assertInLoopThread();
    return static_cast<int>(counts_.size());
  }

  int64_t total() const
  {
    loop_->assertInLoopThread();
    int64_t sum = 0;
    for (size_t i = 0; i < counts_.size(); ++i)
      sum += *counts_[i];
    return sum;
  }

  double callsPerSecond()
  {
    loop_->assertInLoopThread();
    int64_t thisTotal = total();
    Timestamp now(Timestamp::now());
    double seconds = timeDifference(now, lastTime_);
    double cps = static_cast<double>(thisTotal - lastTotal_) / seconds;
    lastTime_ = now;
    lastTotal_ = thisTotal;
    return cps;
  }

 private:

  EventLoop* loop_;
  std::vector<int64_t*> counts_;
  int64_t lastTotal_;
  Timestamp lastTime_;
  int64_t padding_[3];  // make this object large enough to prevent false sharing
};

class RpcClient : noncopyable
{
 public:
  RpcClient(EventLoop* loop,
            const InetAddress& serverAddr,
            CountDownLatch* connected)
    : loop_(loop),
      client_(loop, serverAddr, "RpcClient"),
      channel_(new RpcChannel),
      stub_(get_pointer(channel_)),
      connected_(connected),
      count_(0),
      stop_(false),
      done_(std::bind(&RpcClient::replied, this, _1))
  {
    client_.setConnectionCallback(
        std::bind(&RpcClient::onConnection, this, _1));
    client_.setMessageCallback(
        std::bind(&RpcChannel::onMessage, get_pointer(channel_), _1, _2, _3));
    // client_.enableRetry();
  }

  void connect()
  {
    client_.connect();
  }

  void sendRequest()
  {
    echo::EchoRequest request;
    request.set_payload(g_payload);
    stub_.Echo(request, done_);
  }

  void stop()
  {
    loop_->runInLoop(std::bind(&RpcClient::stopInLoop, this));
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    if (conn->connected())
    {
      //channel_.reset(new RpcChannel(conn));
      ThreadLocalSingleton<PerThreadMeter>::instance().addConnection(&count_);
      channel_->setConnection(conn);
      if (connected_)
      {
        connected_->countDown();
        connected_ = NULL;
      }
    }
  }

  void replied(const echo::EchoResponsePtr& resp)
  {
    assert(resp->payload() == g_payload);
    ++count_;
    if (!stop_)
    {
      sendRequest();
    }
  }

  void stopInLoop()
  {
    stop_ = true;
    ThreadLocalSingleton<PerThreadMeter>::instance().removeConnection(&count_);
  }

  EventLoop* loop_;
  TcpClient client_;
  RpcChannelPtr channel_;
  echo::EchoService::Stub stub_;
  CountDownLatch* connected_;
  int64_t count_;
  bool stop_;
  std::function<void (const echo::EchoResponsePtr&)> done_;
};

void printPerThread()
{
  PerThreadMeter& meter = ThreadLocalSingleton<PerThreadMeter>::instance();
  int conns = meter.connections();
  if (conns > 0)
  {
    double cps = meter.callsPerSecond();
    LOG_INFO << "conns: " << conns
             << "  cps/thread: " << Fmt("%.1f", cps)
             << "  cps/conn: " << Fmt("%.1f", conns > 0 ? cps/conns : 0.0);
  }
  else
  {
    LOG_INFO << "conns: " << conns;
  }
}

void initPerThread(EventLoop* ioLoop)
{
  ThreadLocalSingleton<PerThreadMeter>::instance().setLoop(ioLoop);
  ioLoop->runEvery(1.0, printPerThread);
}

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid() << " sizeof " << sizeof(PerThreadMeter);
  if (argc > 1)
  {
    int nClients = argc > 2 ? atoi(argv[2]) : 1;
    int nThreads = argc > 3 ? atoi(argv[3]) : 1;
    int nPipeline = argc > 4 ? atoi(argv[4]) : 1;

    EventLoop loop;
    EventLoopThreadPool pool(&loop, "clients");
    pool.setThreadNum(nThreads);
    pool.start(initPerThread);
    InetAddress serverAddr(argv[1], 8888);
    sleep(3);
    boost::ptr_vector<RpcClient> clients;

    LOG_INFO << "creating clients";
    for (int i = 0; i < nClients; ++i)
    {
      CountDownLatch connected(1);
      clients.push_back(new RpcClient(pool.getNextLoop(), serverAddr, &connected));
      clients.back().connect();
      connected.wait();
      for (int k = 0; k < nPipeline; ++k)
      {
        clients[i].sendRequest();
      }
      sleep(10);
    }
    LOG_INFO << "all clients created";
    sleep(10);
    LOG_INFO << "stop";
    for (int i = 0; i < nClients; ++i)
    {
      clients[i].stop();
    }
    sleep(3);
    LOG_INFO << "done";
  }
  else
  {
    printf("Usage: %s host_ip numClients [numThreads [piplines]]\n", argv[0]);
  }
  google::protobuf::ShutdownProtobufLibrary();
}

