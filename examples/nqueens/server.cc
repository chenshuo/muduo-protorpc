#include <examples/nqueens/nqueens.pb.h>

#include <muduo/base/Logging.h>
#include <muduo/base/ThreadPool.h>
#include <muduo/net/EventLoop.h>
#include <muduo/protorpc2/RpcServer.h>

using namespace muduo;
using namespace muduo::net;

namespace nqueens
{

struct BackTracking
{
  const static int kMaxQueens = 20;

  const int N;
  int64_t count;
  // bitmasks, 1 means occupied, all 0s initially
  uint32_t columns[kMaxQueens];
  uint32_t diagnoal[kMaxQueens];
  uint32_t antidiagnoal[kMaxQueens];

  BackTracking(int nqueens)
      : N(nqueens), count(0)
  {
    assert(0 < N && N <= kMaxQueens);
    bzero(columns, sizeof columns);
    bzero(diagnoal, sizeof diagnoal);
    bzero(antidiagnoal, sizeof antidiagnoal);
  }

  void search(const int row)
  {
    uint32_t avail = columns[row] | diagnoal[row] | antidiagnoal[row];
    avail = ~avail;

    while (avail) {
      int i = __builtin_ctz(avail); // counting trailing zeros
      if (i >= N) {
        break;
      }
      if (row == N - 1) {
        ++count;
      } else {
        const uint32_t mask = 1 << i;
        columns[row+1] = columns[row] | mask;
        diagnoal[row+1] = (diagnoal[row] | mask) >> 1;
        antidiagnoal[row+1] = (antidiagnoal[row] | mask) << 1;
        search(row+1);
      }

      avail &= avail-1;  // turn off last bit
    }
  }
};

int64_t backtrackingsub(int N, int first_row, int second_row)
{
  const uint32_t m0 = 1 << first_row;
  BackTracking bt(N);
  bt.columns[1] = m0;
  bt.diagnoal[1] = m0 >> 1;
  bt.antidiagnoal[1] = m0 << 1;

  if (second_row >= 0)
  {
    const int row = 1;
    const uint32_t m1 = 1 << second_row;
    uint32_t avail = bt.columns[row] | bt.diagnoal[row] | bt.antidiagnoal[row];
    avail = ~avail;
    if (avail & m1)
    {
      bt.columns[row+1] = bt.columns[row] | m1;
      bt.diagnoal[row+1] = (bt.diagnoal[row] | m1) >> 1;
      bt.antidiagnoal[row+1] = (bt.antidiagnoal[row] | m1) << 1;
      bt.search(row+1);
      return bt.count;
    }
  }
  else
  {
    bt.search(1);
    return bt.count;
  }
  return 0;
}

class NQueensServiceImpl : public NQueensService
{
 public:
  NQueensServiceImpl(int threads)
  {
    pool_.start(threads);
  }

  void Solve(const ::nqueens::SubProblemRequestPtr& request,
             const ::nqueens::SubProblemResponse* responsePrototype,
             const RpcDoneCallback& done) override
  {
    LOG_INFO << "NQueensServiceImpl::Solve " << request->ShortDebugString();
    pool_.run(std::bind(&NQueensServiceImpl::doSolve, request, done));
  }


  static void doSolve(const ::nqueens::SubProblemRequestPtr& request,
                      const RpcDoneCallback& done)
  {
    Timestamp start(Timestamp::now());
    int64_t count = backtrackingsub(request->nqueens(),
                                    request->first_row(),
                                    request->second_row());
    SubProblemResponse response;
    response.set_count(count);
    response.set_seconds(timeDifference(Timestamp::now(), start));
    done(&response);
  }

 private:
  muduo::ThreadPool pool_;
};

}

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  EventLoop loop;
  int threads = argc > 1 ? atoi(argv[1]) : 0;
  nqueens::NQueensServiceImpl impl(threads);
  int port = argc > 2 ? atoi(argv[2]) : 9352;
  InetAddress listenAddr(static_cast<uint16_t>(port));
  RpcServer server(&loop, listenAddr);
  server.registerService(&impl);
  server.start();
  loop.loop();
}

