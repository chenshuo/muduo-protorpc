#include <examples/median/kth.h>
#include <examples/median/median.pb.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/TcpConnection.h>

#include <boost/foreach.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

class RpcClient : boost::noncopyable
{
 public:
  typedef boost::function<void(int64_t, int64_t, int64_t)> QueryCallback;
  typedef boost::function<void(int64_t, int64_t)> SearchCallback;

  RpcClient(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop),
      connectLatch_(NULL),
      client_(loop, serverAddr, "RpcClient" + serverAddr.toIpPort()),
      channel_(new RpcChannel),
      stub_(get_pointer(channel_))
  {
    client_.setConnectionCallback(
        boost::bind(&RpcClient::onConnection, this, _1));
    client_.setMessageCallback(
        boost::bind(&RpcChannel::onMessage, get_pointer(channel_), _1, _2, _3));
    // client_.enableRetry();
  }

  void connect(CountDownLatch* connectLatch)
  {
    connectLatch_ = connectLatch;
    client_.connect();
  }

  void query(const QueryCallback& cb)
  {
    ::rpc2::Empty req;
    stub_.Query(req, boost::bind(&RpcClient::queryCb, this, _1, cb)); // need C++11 lambda
  }

  void search(int64_t guess, const SearchCallback& cb)
  {
    median::SearchRequest req;
    req.set_guess(guess);
    stub_.Search(req, boost::bind(&RpcClient::searchCb, this, _1, cb)); // need C++11 lambda
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    if (conn->connected())
    {
      //channel_.reset(new RpcChannel(conn));
      channel_->setConnection(conn);
      if (connectLatch_)
      {
        connectLatch_->countDown();
        connectLatch_ = NULL;
      }
    }
  }

  void queryCb(const median::QueryResponsePtr& resp, const QueryCallback& cb)
  {
    LOG_DEBUG << resp->DebugString();
    cb(resp->count(), resp->min(), resp->max());
  }

  void searchCb(const median::SearchResponsePtr& resp, const SearchCallback& cb)
  {
    LOG_DEBUG << resp->DebugString();
    cb(resp->smaller(), resp->same());
  }

  EventLoop* loop_;
  CountDownLatch* connectLatch_;
  TcpClient client_;
  RpcChannelPtr channel_;
  median::Sorter::Stub stub_;
};

class Merger : boost::noncopyable
{
 public:
  Merger(EventLoop* loop, const std::vector<InetAddress>& addresses)
    : loop_(loop)
  {
    for (size_t i = 0; i < addresses.size(); ++i)
    {
      sorters_.push_back(new RpcClient(loop, addresses[i]));
    }
  }

  void connect()
  {
    assert(!loop_->isInLoopThread());
    CountDownLatch latch(static_cast<int>(sorters_.size()));
    BOOST_FOREACH(RpcClient& sorter, sorters_)
    {
      sorter.connect(&latch);
    }
    latch.wait();
  }

  void getCount(int64_t* count, int64_t* min, int64_t* max)
  {
    assert(!loop_->isInLoopThread());
    CountDownLatch latch(static_cast<int>(sorters_.size()));
    BOOST_FOREACH(RpcClient& sorter, sorters_)
    {
      sorter.query(boost::bind(&Merger::queryCb, this, &latch, count, min, max, _1, _2, _3)); // need C++11 lambda
    }
    latch.wait();
  }

  void search(int64_t guess, int64_t* smaller, int64_t* same)
  {
    assert(!loop_->isInLoopThread());
    *smaller = 0;
    *same = 0;
    CountDownLatch latch(static_cast<int>(sorters_.size()));
    BOOST_FOREACH(RpcClient& sorter, sorters_)
    {
      sorter.search(guess, boost::bind(&Merger::searchCb, this, &latch, smaller, same, _1, _2)); // need C++11 lambda
    }
    latch.wait();
  }

 private:

  void queryCb(CountDownLatch* latch,
               int64_t* count, int64_t* min, int64_t* max,
               int64_t thiscount, int64_t thismin, int64_t thismax)
  {
    assert(loop_->isInLoopThread());
    *count += thiscount;
    if (thiscount > 0)
    {
      if (thismin < *min)
        *min = thismin;
      if (thismax > *max)
        *max = thismax;
    }
    latch->countDown();
  }

  void searchCb(CountDownLatch* latch,
                int64_t* smaller, int64_t* same,
                int64_t thissmaller, int64_t thissame)
  {
    assert(loop_->isInLoopThread());
    *smaller += thissmaller;
    *same += thissame;
    latch->countDown();
  }

  EventLoop* loop_;
  boost::ptr_vector<RpcClient> sorters_;
};

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

void run(Merger* merger)
{
  int64_t count = 0;
  int64_t min = std::numeric_limits<int64_t>::max();
  int64_t max = std::numeric_limits<int64_t>::min();

  merger->getCount(&count, &min, &max);
  LOG_INFO << "count = " << count
           << ", min = " << min
           << ", max = " << max;

  if (count <= 0)
  {
    LOG_INFO << "***** No median";
  }
  else
  {
    std::pair<int64_t, bool> median = getKth(
        boost::bind(&Merger::search, merger, _1, _2, _3),
        (count+1)/2, count, min, max);
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

int main(int argc, char* argv[])
{
  if (argc > 1)
  {
    LOG_INFO << "Starting";
    EventLoopThread loop;
    Merger merger(loop.startLoop(), getAddresses(argc, argv));
    merger.connect();
    LOG_INFO << "All connected";
    run(&merger);
  }
  else
  {
    printf("Usage: %s sorter_addresses\n", argv[0]);
  }
}
