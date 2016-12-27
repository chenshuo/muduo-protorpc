#include <examples/nqueens/nqueens.pb.h>

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>

using namespace muduo;
using namespace muduo::net;

namespace nqueens
{

int g_got = 0;
int64_t g_total = 0;

void run(int N, NQueensService::Stub* stub, EventLoop* loop)
{
  LOG_INFO << "N=" << N;
  for (int col = 0; col < (N+1)/2; ++col)
  {
    SubProblemRequest request;
    request.set_nqueens(N);
    request.set_first_row(col);
    stub->Solve(request, [N, col, loop](const SubProblemResponsePtr& resp)
      {
        LOG_INFO << "response for col=" << col << " : " << resp->ShortDebugString();
        if (N % 2 == 1 && col == N / 2)
          g_total += resp->count();
        else
          g_total += 2*resp->count();

        ++g_got;
        if (g_got == (N+1)/2)
        {
          LOG_INFO << "total: " << g_total;
          // TODO: disconnect
        }
      });
  }
}

}

int main(int argc, char* argv[])
{
  if (argc > 1)
  {
    int N = atoi(argv[1]);
    const char* ip = argc > 2 ? argv[2] : "127.0.0.1";
    int port = argc > 3 ? atoi(argv[3]) : 9352;
    InetAddress serverAddr(ip, static_cast<uint16_t>(port));

    EventLoop loop;
    TcpClient client(&loop, serverAddr, "NqueensClient");
    RpcChannel channel;
    nqueens::NQueensService::Stub stub(&channel);

    client.setConnectionCallback(
      [N, &channel, &stub, &loop](const TcpConnectionPtr& conn) {
        if (conn->connected())
        {
          channel.setConnection(conn);
          run(N, &stub, &loop);  // Koenig lookup
        }
        else
        {
          channel.setConnection(TcpConnectionPtr());
          loop.quit();
        }
      });

    client.setMessageCallback(
        std::bind(&RpcChannel::onMessage, &channel, _1, _2, _3));
    client.connect();
    loop.loop();
    google::protobuf::ShutdownProtobufLibrary();
  }
  else
  {
    printf("Usage: %s N [host_ip [port]]\n", argv[0]);
  }
}
