#include <examples/wordfreq/wordfreq.pb.h>

#include <muduo/base/BlockingQueue.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Logging.h>
#include <muduo/base/Thread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/TcpServer.h>

#include <muduo/protorpc2/RpcServer.h>

#include <algorithm>
#include <fstream>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace gpb = google::protobuf;
using namespace muduo;
using namespace muduo::net;

namespace wordfreq
{

typedef std::function<void()> Callback;
class Impl;

class RpcClient : noncopyable
{
 public:
  RpcClient(EventLoop* loop, const InetAddress& serverAddr, Impl* owner)
    : loop_(loop),
      owner_(owner),
      connected_(false),
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

  void connect()
  {
    client_.connect();
  }

  bool connected() const
  {
    return connected_;
  }

  void shardKey(ShardKeyRequest* request, const Callback& cb)
  {
    LOG_DEBUG << "[" << name_ << "] " << request
              << " part " << request->partition()
              << " size " << request->keys_size();
    stub_.ShardKey(*request, std::bind(&RpcClient::shardKeyCb, this, cb));
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    if (conn->connected())
    {
      connected_ = true;
      channel_->setConnection(conn);
    }
    else
    {
      connected_ = false;
    }
    LOG_INFO << "RpcClient[" << name_ << "] - connection " << connected_;
  }

  void shardKeyCb(const Callback& cb)
  {
    LOG_DEBUG << "[" << name_ << "] ";
    cb();
  }

  EventLoop* loop_;
  Impl* owner_;
  bool connected_;
  string name_;
  TcpClient client_;
  RpcChannelPtr channel_;
  WordFrequencyService::Stub stub_;
};

class Impl : public WordFrequencyService
{
 public:
  Impl(EventLoop* loop, const char* shardFile, const char* peers)
    : loop_(loop),
      maxKey_(0),
      shuffling_(false),
      concurrentRequests_(0),
      partition_(-1)
  {
    readKeys(shardFile);
    createPeers(peers);
    concurrentRequests_ = peers_.size() * kConcurrency;
  }

  virtual void GetInfo(const ::rpc2::EmptyPtr& request,
                       const GetInfoResponse* responsePrototype,
                       const RpcDoneCallback& done)
  {
    GetInfoResponse response;
    response.set_ready(isReady());
    response.set_maxkey(maxKey_);
    response.set_keycount(keys_.size());
    for (size_t i = 0; i < peerAddresses_.size(); ++i)
    {
      response.add_peers(peerAddresses_[i].toIpPort().c_str());
    }
    done(&response);
  }

  virtual void Quit(const ::rpc2::EmptyPtr& request,
                    const rpc2::Empty* responsePrototype,
                    const RpcDoneCallback& done)
  {
    done(responsePrototype);
    loop_->quit();
  }

  virtual void GetHistogram(const GetHistogramRequestPtr& request,
                            const GetHistogramResponse* responsePrototype,
                            const RpcDoneCallback& done)
  {
    // pivots
    const gpb::RepeatedField<int64_t>& pivots = request->pivots();
    assert(isSorted(pivots.begin(), pivots.end()));
    GetHistogramResponse response;
    for (int i = 0; i < pivots.size()+1; ++i)
    {
      response.add_counts(0);
    }

    // keys_ is unsorted in this version

    for (size_t i = 0; i < keys_.size(); ++i)
    {
      // TODO: linear searching could be faster in some cases.
      const int64_t* it = std::lower_bound(pivots.begin(), pivots.end(), keys_[i]);
      int idx = static_cast<int>(it - pivots.begin());
      assert(idx < response.counts().size());
      int64_t* count = response.mutable_counts()->Mutable(idx);
      ++*count;
    }

    done(&response);
  }

  virtual void ShuffleKey(const ShuffleKeyRequestPtr& request,
                          const ShuffleKeyResponse* responsePrototype,
                          const RpcDoneCallback& done)
  {
    Timestamp start(Timestamp::now());
    const gpb::RepeatedField<int64_t>& pivots = request->pivots();
    if (implicit_cast<size_t>(pivots.size()) == peers_.size()
        && pivots.Get(pivots.size()-1) >= maxKey_
        && isSorted(pivots.begin(), pivots.end())
        && !shuffling_)
    {
      shuffling_ = true;
      Thread thr(std::bind(&Impl::shuffle, this, request, done, start));
      thr.start();
    }
    else
    {
      LOG_ERROR << "ShuffleKey";
      ShuffleKeyResponse response;
      response.set_error(shuffling_ ? "Shuffling" : "Invalid pivots.");
      done(&response);
    }
  }

  virtual void ShardKey(const ShardKeyRequestPtr& request,
                        const rpc2::Empty* responsePrototype,
                        const RpcDoneCallback& done)
  {
    LOG_DEBUG << "ShardKey " << request->keys_size();
    if (partition_ == -1)
    {
      partition_ = request->partition();
      assert(partitionKeys_.empty());
    }
    else if (partition_ != request->partition())
    {
      LOG_ERROR << "Wrong partition, was " << partition_
                << " now " << request->partition();
    }
    partitionKeys_.insert(partitionKeys_.end(), request->keys().begin(), request->keys().end());
    done(&rpc2::Empty::default_instance());
  }

  virtual void SortKey(const ::rpc2::EmptyPtr& request,
                       const SortKeyResponse* responsePrototype,
                       const RpcDoneCallback& done)
  {
    std::sort(partitionKeys_.begin(), partitionKeys_.end());

    {
    std::ofstream out("partition");
    std::copy(partitionKeys_.begin(), partitionKeys_.end(),
              std::ostream_iterator<int64_t>(out, "\n"));
    }

    SortKeyResponse response;
    response.set_partition(partition_);
    response.set_count(partitionKeys_.size());
    if (!partitionKeys_.empty())
    {
      response.set_minkey(partitionKeys_.front());
      response.set_maxkey(partitionKeys_.back());
    }

    done(&response);
  }

 private:

  bool isSorted(const int64_t* begin, const int64_t* end)
  {
    const int64_t* it = std::adjacent_find(begin, end, std::greater_equal<int64_t>());
    return it == end;
  }

  void readKeys(const char* file)
  {
    std::ifstream in(file);
    std::string line;
    while (getline(in, line))
    {
      size_t tab = line.find('\t');
      if (tab != string::npos)
      {
        int64_t key = strtoll(line.c_str() + tab, NULL, 10);
        if (key > 0)
        {
          keys_.push_back(key);
          if (key > maxKey_)
          {
            maxKey_ = key;
          }
        }
      }
    }
    // sort keys_ ???
  }

  void createPeers(const char* peers)
  {
    std::vector<string> addresses;
    boost::split(addresses, peers, boost::is_any_of(","));
    for (size_t i = 0; i < addresses.size(); ++i)
    {
      const string& addr = addresses[i];
      size_t colon = addr.find(':');
      if (colon != string::npos)
      {
        string ip = addr.substr(0, colon);
        uint16_t port = static_cast<uint16_t>(atoi(addr.c_str()+colon+1));
        peerAddresses_.push_back(InetAddress(ip, port));
        peers_.push_back(new RpcClient(loop_, peerAddresses_.back(), this));
        peers_.back().connect();
      }
      else
      {
        LOG_ERROR << "Invalid address " << addr;
      }
    }
  }

  bool isReady()
  {
    bool allConnected = true;
    for (size_t i = 0; i < peers_.size() && allConnected; ++i)
      allConnected = allConnected && peers_[i].connected();
    return allConnected;
  }

  void shuffle(const ShuffleKeyRequestPtr& request,
               const RpcDoneCallback& done,
               Timestamp start)
  {
    const gpb::RepeatedField<int64_t>& pivots = request->pivots();
    assert(implicit_cast<size_t>(pivots.size()) == peers_.size());

    std::vector<ShardKeyRequest*> shardRequests;
    shardRequests.reserve(peers_.size());
    for (size_t i = 0; i < peers_.size(); ++i)
    {
      shardRequests.push_back(new ShardKeyRequest);
    }
    assert(shardRequests.size() == peers_.size());

    assert(freeShards_.size() == 0);
    for (size_t i = shardRequests.size(); i < concurrentRequests_; ++i)
    {
      freeShards_.put(new ShardKeyRequest);
    }

    for (size_t i = 0; i < keys_.size(); ++i)
    {
      int64_t key = keys_[i];
      const int64_t* it = std::lower_bound(pivots.begin(), pivots.end(), key);
      assert(it != pivots.end());

      int part = static_cast<int>(it - pivots.begin());
      ShardKeyRequest* shardReq = shardRequests[part];
      shardReq->add_keys(key);
      if (shardReq->keys_size() >= kBatch)
      {
        shardReq->set_partition(part);
        peers_[part].shardKey(shardReq, std::bind(&Impl::doneShardKey, this, shardReq));
        LOG_DEBUG << "taking";
        shardReq = freeShards_.take();
        LOG_DEBUG << "taken " << shardReq;;
        shardReq->Clear();
        shardRequests[part] = shardReq;
      }
    }

    for (size_t part = 0; part < shardRequests.size(); ++part)
    {
      ShardKeyRequest* shardReq = shardRequests[part];
      if (shardReq->keys_size() > 0)
      {
        shardReq->set_partition(static_cast<int>(part));
        peers_[part].shardKey(shardReq, std::bind(&Impl::doneShardKey, this, shardReq));
      }
    }

    for (size_t i = 0; i < concurrentRequests_; ++i)
    {
      LOG_DEBUG << "taking";
      ShardKeyRequest* shardReq = freeShards_.take();
      LOG_DEBUG << "taken " << shardReq;;
      delete shardReq;
    }
    ShuffleKeyResponse response;
    response.set_elapsed(timeDifference(Timestamp::now(), start));
    done(&response);
    done(&ShuffleKeyResponse::default_instance());
  }

  void doneShardKey(ShardKeyRequest* req)
  {
    LOG_DEBUG << req;
    freeShards_.put(req);
  }

  EventLoop* loop_;
  std::vector<InetAddress> peerAddresses_;
  boost::ptr_vector<RpcClient> peers_;

  int64_t maxKey_;
  std::vector<int64_t> keys_;

  bool shuffling_;
  size_t concurrentRequests_;
  BlockingQueue<ShardKeyRequest*> freeShards_;
  const static int kConcurrency = 4;
  const static int kBatch = 8192;

  int partition_;
  std::vector<int64_t> partitionKeys_;
};
}

int main(int argc, char* argv[])
{
  if (argc == 4)
  {
    EventLoop loop;
    int port = atoi(argv[1]);
    InetAddress listenAddr(static_cast<uint16_t>(port));
    wordfreq::Impl impl(&loop, argv[2], argv[3]);
    RpcServer server(&loop, listenAddr);
    server.registerService(&impl);
    server.start();
    loop.loop();
    google::protobuf::ShutdownProtobufLibrary();
  }
  else
  {
    printf("Usage: %s listen_port shard_file peer_list\n", argv[0]);
    printf("peer_list is ip1:port1,ip2:port2,ip3:port3,...\n");
  }
}
