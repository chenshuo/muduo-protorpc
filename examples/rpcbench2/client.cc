#include <examples/rpcbench2/echo.pb.h>

#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThreadPool.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/TcpConnection.h>

#include <boost/ptr_container/ptr_vector.hpp>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

static const int kRequests = 50000;
std::string g_payload = "0123456789ABCDEF";

class RpcClient : noncopyable
{
 public:

  RpcClient(EventLoop* loop,
            const InetAddress& serverAddr,
            CountDownLatch* allConnected,
            CountDownLatch* allFinished)
    : loop_(loop),
      client_(loop, serverAddr, "RpcClient"),
      channel_(new RpcChannel),
      stub_(get_pointer(channel_)),
      allConnected_(allConnected),
      allFinished_(allFinished),
      count_(0),
      finished_(false)
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
    stub_.Echo(request, std::bind(&RpcClient::replied, this, _1));
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    if (conn->connected())
    {
      //channel_.reset(new RpcChannel(conn));
      channel_->setConnection(conn);
      allConnected_->countDown();
    }
  }

  void replied(const echo::EchoResponsePtr& resp)
  {
    //LOG_INFO << "replied:\n" << resp->DebugString();
    //loop_->quit();
    assert(resp->payload() == g_payload);
    ++count_;
    if (count_ < kRequests)
    {
      sendRequest();
    }
    else if (!finished_)
    {
      finished_ = true;
      LOG_INFO << "RpcClient " << this << " finished";
      allFinished_->countDown();
    }
  }

  EventLoop* loop_;
  TcpClient client_;
  RpcChannelPtr channel_;
  echo::EchoService::Stub stub_;
  CountDownLatch* allConnected_;
  CountDownLatch* allFinished_;
  int count_;
  bool finished_;
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  if (argc > 1)
  {
    int nClients = argc > 2 ? atoi(argv[2]) : 1;
    int nThreads = argc > 3 ? atoi(argv[3]) : 1;
    int nPipeline = argc > 4 ? atoi(argv[4]) : 1;

    CountDownLatch allConnected(nClients);
    CountDownLatch allFinished(nClients);

    EventLoop loop;
    EventLoopThreadPool pool(&loop, "clients");
    pool.setThreadNum(nThreads);
    pool.start();
    InetAddress serverAddr(argv[1], 8888);

    boost::ptr_vector<RpcClient> clients;
    for (int i = 0; i < nClients; ++i)
    {
      clients.push_back(new RpcClient(pool.getNextLoop(), serverAddr, &allConnected, &allFinished));
      clients.back().connect();
    }
    allConnected.wait();
    LOG_INFO << "all connected";
    sleep(5);
    LOG_WARN << "start";
    Timestamp start(Timestamp::now());
    for (int i = 0; i < nClients; ++i)
    {
      // pipline
      for (int k = 0; k < nPipeline; ++k)
        clients[i].sendRequest();
    }
    allFinished.wait();
    Timestamp end(Timestamp::now());
    LOG_INFO << "all finished";
    double seconds = timeDifference(end, start);
    printf("%f seconds\n", seconds);
    printf("%.1f calls per second\n", nClients * kRequests / seconds);

    sleep(5);
    google::protobuf::ShutdownProtobufLibrary();
  }
  else
  {
    printf("Usage: %s host_ip numClients [numThreads [piplines]]\n", argv[0]);
  }
}

