#include <examples/median/kth.h>
#include <examples/median/median.pb.h>
#include <muduo/base/CountDownLatch.h>
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

namespace median
{

class RpcClient : noncopyable
{
 public:
  RpcClient(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop),
      connectLatch_(NULL),
      client_(loop, serverAddr, "RpcClient" + serverAddr.toIpPort()),
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

  Sorter::Stub* stub() { return &stub_; } 

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
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
  CountDownLatch* connectLatch_;
  TcpClient client_;
  RpcChannelPtr channel_;
  Sorter::Stub stub_;
};

class Collector : noncopyable
{
 public:
  Collector(EventLoop* loop, const std::vector<InetAddress>& addresses)
    : loop_(loop)
  {
    for (const auto& addr : addresses)
    {
      sorters_.push_back(new RpcClient(loop, addr));
    }
  }

  void connect()
  {
    assert(!loop_->isInLoopThread());
    CountDownLatch latch(static_cast<int>(sorters_.size()));
    for (RpcClient& sorter : sorters_)
    {
      sorter.connect(&latch);
    }
    latch.wait();
  }

  void run()
  {
    QueryResponse stats;
    stats.set_min(std::numeric_limits<int64_t>::max());
    stats.set_max(std::numeric_limits<int64_t>::min());
    getStats(&stats);
    LOG_INFO << "stats:\n" << stats.DebugString();

    const int64_t count = stats.count();
    if (count > 0)
    {
      LOG_INFO << "mean: " << static_cast<double>(stats.sum()) / static_cast<double>(count);
    }

    if (count <= 0)
    {
      LOG_INFO << "***** No median";
    }
    else
    {
      const int64_t k = (count+1)/2;
      std::pair<int64_t, bool> median = getKth(
          std::bind(&Collector::search, this, _1, _2, _3),
          k, count, stats.min(), stats.max());
      if (median.second)
      {
        LOG_INFO << "***** Median is " << median.first;
      }
      else
      {
        LOG_ERROR << "***** Median not found";
      }
    }
  }

 private:
  void getStats(QueryResponse* result)
  {
    assert(!loop_->isInLoopThread());
    CountDownLatch latch(static_cast<int>(sorters_.size()));
    for (RpcClient& sorter : sorters_)
    {
      ::rpc2::Empty req;
      sorter.stub()->Query(req, [this, result, &latch](const QueryResponsePtr& resp)
        {
          assert(loop_->isInLoopThread());
          result->set_count(result->count() + resp->count());  // result->count += resp->count
          result->set_sum(result->sum() + resp->sum());  // result->sum += resp->sum
          if (resp->count() > 0)
          {
            if (resp->min() < result->min())
              result->set_min(resp->min());
            if (resp->max() > result->max())
              result->set_max(resp->max());
          }
          latch.countDown();
        });
    }
    latch.wait();
  }

  void search(int64_t guess, int64_t* smaller, int64_t* same)
  {
    assert(!loop_->isInLoopThread());
    *smaller = 0;
    *same = 0;
    CountDownLatch latch(static_cast<int>(sorters_.size()));
    for (RpcClient& sorter : sorters_)
    {
      SearchRequest req;
      req.set_guess(guess);
      sorter.stub()->Search(req, [this, smaller, same, &latch](const SearchResponsePtr& resp)
        {
          assert(loop_->isInLoopThread());
          *smaller += resp->smaller();
          *same += resp->same();
          latch.countDown();
        });
    }
    latch.wait();
  }

  EventLoop* loop_;
  boost::ptr_vector<RpcClient> sorters_;
};

}  // namespace median

std::vector<InetAddress> getAddresses(int argc, char* argv[])
{
  std::vector<InetAddress> result;
  for (int i = 1; i < argc; ++i)
  {
    string addr = argv[i];
    size_t colon = addr.find(':');
    if (colon != string::npos)
    {
      string ip = addr.substr(0, colon);
      uint16_t port = static_cast<uint16_t>(atoi(addr.c_str()+colon+1));
      result.push_back(InetAddress(ip, port));
    }
    else
    {
      LOG_ERROR << "Invalid address " << addr;
    }
  }
  return result;
}

int main(int argc, char* argv[])
{
  if (argc > 1)
  {
    LOG_INFO << "Starting";
    EventLoopThread loop;
    median::Collector collector(loop.startLoop(), getAddresses(argc, argv));
    collector.connect();
    LOG_INFO << "All connected";
    collector.run();
    // FIXME: proper exit
  }
  else
  {
    printf("Usage: %s sorter_addresses\n", argv[0]);
  }
}
