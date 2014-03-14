#include <examples/wordfreq/partition.h>
#include <examples/wordfreq/wordfreq.pb.h>

#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Exception.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/TcpConnection.h>

#include <boost/ptr_container/ptr_vector.hpp>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

namespace wordfreq
{

class Controller;

class RpcClient : noncopyable
{
 public:
  RpcClient(EventLoop* loop, const InetAddress& serverAddr, Controller* owner)
    : loop_(loop),
      owner_(owner),
      connectLatch_(NULL),
      name_(serverAddr.toIpPort()),
      client_(loop, serverAddr, name_),
      channel_(new RpcChannel),
      stub_(get_pointer(channel_))
  {
    client_.setConnectionCallback(
        std::bind(&RpcClient::onConnection, this, _1));
    client_.setMessageCallback(
        std::bind(&RpcChannel::onMessage, get_pointer(channel_), _1, _2, _3));
    // client_.enableRetry();
  }

  void connect(CountDownLatch* connectLatch)
  {
    connectLatch_ = connectLatch;
    client_.connect();
  }

  void getInfo(const std::function<void(const GetInfoResponsePtr&)>& cb)
  {
    stub_.GetInfo(rpc2::Empty::default_instance(), cb);
  }

  void getHistogram()
  {
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    LOG_INFO << "RpcClient[" << name_ << "] - connection " << conn->connected();
    if (conn->connected())
    {
      channel_->setConnection(conn);
      if (connectLatch_)
      {
        connectLatch_->countDown();
        connectLatch_ = NULL;
      }
    }
  }

  EventLoop* loop_;
  Controller* owner_;
  CountDownLatch* connectLatch_;
  string name_;
  TcpClient client_;
  RpcChannelPtr channel_;
  WordFrequencyService::Stub stub_;
};

struct Worker
{
  bool gotInfo;
  bool ready;
  int64_t shardMaxKey;
  int64_t shardKeyCount;
  std::string peers;

  Worker()
    : gotInfo(false),
      ready(false),
      shardMaxKey(-1),
      shardKeyCount(-1)
  {
  }
};

class Controller : noncopyable
{
 public:
  Controller(EventLoop* loop, const InetAddress& headWorker)
    : loop_(loop),
      headAddress_(headWorker),
      maxKey_(0),
      keyCount_(0)
  {
  }

  void run()
  {
    try
    {
      prepare();
      LOG_INFO << "All ready, num of workers = " << workers_.size()
               << ", max key = " << maxKey_ << ", total count = " << keyCount_;
      findPivots();
    }
    catch(const Exception& ex)
    {
      LOG_ERROR << ex.what();
    }
  }

 private:
  void prepare()
  {
    assert(!loop_->isInLoopThread());
    assert(clients_.empty());
    assert(workers_.empty());

    // this version makes N+1 connections.
    // FIXME: reuse head client
    std::string peers = getPeers();

    LOG_INFO << "connecting to " << clients_.size() << " wokers: " << peers;
    connectAll();

    LOG_INFO << "got info of all";
    for (Worker& worker : workers_)
    {
      keyCount_ += worker.shardKeyCount;
      if (worker.shardMaxKey > maxKey_)
      {
        maxKey_ = worker.shardMaxKey;
      }
      if (!worker.ready)
      {
        LOG_FATAL << "Worker is not ready";
      }
      if (worker.peers != peers)
      {
        LOG_FATAL << "Different peer address list of workers";
      }
    }
  }

  std::string getPeers()
  {
    RpcClient head(loop_, headAddress_, this);
    {
      CountDownLatch latch(1);
      head.connect(&latch);
      latch.wait();
    }

    std::string peers;
    LOG_INFO << "getInfo of head";
    {
      CountDownLatch latch(1);
      head.getInfo(std::bind(&Controller::getHeadInfoCb, this, _1, &latch, &peers));
      latch.wait();
    }

    return peers;
  }

  void getHeadInfoCb(const GetInfoResponsePtr& response,
                     CountDownLatch* latch,
                     std::string* peers)
  {
    LOG_DEBUG << response->DebugString();

    for (int i = 0; i < response->peers_size(); ++i)
    {
      const std::string& addr = response->peers(i);
      *peers += addr;
      *peers += ',';

      size_t colon = addr.find(':');
      if (colon != string::npos)
      {
        std::string ip = addr.substr(0, colon);
        uint16_t port = static_cast<uint16_t>(atoi(addr.c_str()+colon+1));
        clients_.push_back(new RpcClient(loop_, InetAddress(ip.c_str(), port), this));
      }
      else
      {
        LOG_FATAL << "Invalid peer address: " << addr;
      }
    }
    latch->countDown();
  }

  void connectAll()
  {
    {
      CountDownLatch latch(static_cast<int>(clients_.size()));
      for (RpcClient& client : clients_)
      {
        client.connect(&latch);
      }
      latch.wait();
    }

    LOG_INFO << "getInfo of all";
    workers_.resize(clients_.size());
    {
      CountDownLatch latch(static_cast<int>(clients_.size()));
      for (size_t i = 0; i < clients_.size(); ++i)
      {
        clients_[i].getInfo(std::bind(&Controller::getInfoCb, this, _1, &latch, i));
      }
      latch.wait();
    }
    assert(clients_.size() == workers_.size());
  }

  void getInfoCb(const GetInfoResponsePtr& response, CountDownLatch* latch, int part)
  {
    LOG_DEBUG << response->DebugString();

    Worker& worker = workers_[part];
    assert(!worker.gotInfo);
    worker.gotInfo = true;
    worker.ready = response->ready();
    if (!worker.ready)
    {
      // FIXME: retry with delay
      LOG_FATAL << "worker " << part << " is not ready.";
    }
    worker.shardMaxKey = response->maxkey();
    worker.shardKeyCount = response->keycount();
    for (int i = 0; i < response->peers_size(); ++i)
    {
      const std::string& addr = response->peers(i);
      worker.peers += addr;
      worker.peers += ',';
    }
    latch->countDown();
  }

  void findPivots()
  {
    // FIXME:
  }

  EventLoop* loop_;
  InetAddress headAddress_;
  boost::ptr_vector<RpcClient> clients_;
  std::vector<Worker> workers_;
  int64_t maxKey_;
  int64_t keyCount_;
};

}

int main(int argc, char* argv[])
{
  if (argc == 2)
  {
    const string& addr = argv[1];
    size_t colon = addr.find(':');
    if (colon != string::npos)
    {
      string ip = addr.substr(0, colon);
      uint16_t port = static_cast<uint16_t>(atoi(addr.c_str()+colon+1));
      EventLoopThread loop;
      wordfreq::Controller controller(loop.startLoop(), InetAddress(ip, port));
      controller.run();
    }
    else
    {
      LOG_ERROR << "Invalid address " << addr;
    }
  }
  else
  {
    printf("Usage: %s ip:port", argv[0]);
  }
}

