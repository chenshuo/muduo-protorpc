#include <examples/sudoku/sudoku.pb.h>

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/protorpc2/RpcServer.h>

using namespace muduo;
using namespace muduo::net;

namespace sudoku
{

class SudokuServiceImpl : public SudokuService
{
 public:
  virtual void Solve(const ::sudoku::SudokuRequestPtr& request,
                     const ::sudoku::SudokuResponse* responsePrototype,
                     const RpcDoneCallback& done)
  {
    LOG_INFO << "SudokuServiceImpl::Solve";
    SudokuResponse response;
    response.set_solved(true);
    response.set_checkerboard("1234567");
    done(&response);
  }
};

}

int main()
{
  LOG_INFO << "pid = " << getpid();
  EventLoop loop;
  InetAddress listenAddr(9981);
  sudoku::SudokuServiceImpl impl;
  RpcServer server(&loop, listenAddr);
  server.registerService(&impl);
  server.start();
  loop.loop();
}

