#include <examples/wordfreq/wordfreq.pb.h>

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>

#include <muduo/protorpc2/RpcServer.h>

#include <algorithm>
#include <fstream>
#include <vector>

namespace gpb = google::protobuf;
using namespace muduo;
using namespace muduo::net;

namespace wordfreq
{

typedef std::vector<std::pair<int64_t, string> > WordCountList;

class Impl : public WordFrequencyService
{
 public:
  Impl(const char* shardFile)
    : maxCount_(0)
  {
    read(shardFile);
  }

  virtual void GetMaxCount(const ::rpc2::EmptyPtr& request,
                           const GetMaxCountResponse* responsePrototype,
                           const RpcDoneCallback& done)
  {
    GetMaxCountResponse response;
    response.set_count(maxCount_);
    done(&response);
  }

  virtual void GetPeerList(const ::rpc2::EmptyPtr& request,
                           const GetPeerListResponse* responsePrototype,
                           const RpcDoneCallback& done)
  {
    // FIXME
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

    // wordCounts_ is unsorted in this version

    for (size_t i = 0; i < wordCounts_.size(); ++i)
    {
      // TODO: linear searching could be faster in some cases.
      const int64_t* it = std::lower_bound(pivots.begin(), pivots.end(),
                                           wordCounts_[i].first);
      int idx = static_cast<int>(it - pivots.begin());
      assert(idx < response.counts().size());
      int64_t* count = response.mutable_counts()->Mutable(idx);
      ++*count;
    }

    done(&response);
  }

  virtual void ShuffleKey(const ShuffleRequestPtr& request,
                          const ::rpc2::Empty* responsePrototype,
                          const RpcDoneCallback& done)
  {
    // FIXME
  }

  virtual void ShardKey(const ShardKeyRequestPtr& request,
                        const ::rpc2::Empty* responsePrototype,
                        const RpcDoneCallback& done)
  {
    // FIXME
  }

 private:

  bool isSorted(const int64_t* begin, const int64_t* end)
  {
    const int64_t* it = std::adjacent_find(begin, end, std::greater_equal<int64_t>());
    return it == end;
  }

  void read(const char* file)
  {
    std::ifstream in(file);
    std::string line;
    while (getline(in, line))
    {
      size_t tab = line.find('\t');
      if (tab != string::npos)
      {
        int64_t count = strtoll(line.c_str() + tab, NULL, 10);
        if (count > 0)
        {
          string word(line.begin(), line.begin()+tab);
          wordCounts_.push_back(make_pair(count, word));
          if (count > maxCount_)
          {
            maxCount_ = count;
          }
        }
      }
    }
  }

  int64_t maxCount_;
  WordCountList wordCounts_;
};
}

int main(int argc, char* argv[])
{
  if (argc == 4)
  {
    EventLoop loop;
    int port = atoi(argv[1]);
    InetAddress listenAddr(static_cast<uint16_t>(port));
    wordfreq::Impl impl(argv[2]);
    RpcServer server(&loop, listenAddr);
    server.registerService(&impl);
    server.start();
    loop.loop();
    google::protobuf::ShutdownProtobufLibrary();
  }
  else
  {
    printf("Usage: %s listen_port shard_file peer_list\n", argv[0]);
  }
}
