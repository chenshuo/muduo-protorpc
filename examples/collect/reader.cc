#include <examples/collect/collect.pb.h>
#include <muduo/base/GzipFile.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/protobuf/ProtobufCodecLite.h>

namespace collect
{
extern const char tag[] = "SYS0";
typedef muduo::net::ProtobufCodecLiteT<SystemInfo, tag> Codec;
}

using namespace muduo;

void foreach(GzipFile& f, std::function<void (const collect::SystemInfo& msg)>&& func)
{
  collect::Codec codec([&func](const muduo::net::TcpConnectionPtr&,
                               const collect::SystemInfoPtr& msg,
                               Timestamp) {
    func(*msg);
  });
  muduo::net::Buffer buffer;
  int nr = 0;
  while ( (nr = f.read(buffer.beginWrite(), static_cast<int>(buffer.writableBytes()))) > 0)
  {
    buffer.hasWritten(nr);
    codec.onMessage(muduo::net::TcpConnectionPtr(), &buffer, Timestamp::invalid());
    buffer.ensureWritableBytes(4096);
  }
}

int main(int argc, char* argv[])
{
  int total = 0;
  for (int i = 1; i < argc; ++i)
  {
    GzipFile f = GzipFile::openForRead(argv[i]);
    if (f.valid())
    {
      foreach(f, [&total](const collect::SystemInfo& msg) {
        //printf("%ld\n", msg.muduo_timestamp());
        ++total;
      });
    }
  }
  printf("%d\n", total);
}
