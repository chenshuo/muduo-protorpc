#include <examples/collect/collect.pb.h>

#include <muduo/base/FileUtil.h>
#include <muduo/base/StringPiece.h>

namespace collect
{

// Read small files in /proc.
// TODO: caches file descriptor to avoid reopening every time.
class ProcFs : muduo::noncopyable
{
 public:
  bool fill(SnapshotRequest_Level, SystemInfo* info);

 private:
  enum CacheLevel
  {
    kKeep, kLRU
  };

  void fillLoadAvg(SystemInfo* info);

  bool readFile(muduo::StringArg filename, CacheLevel cache)
  {
    return 0 == muduo::FileUtil::readFile(filename, 64*1024, &content_);
  }

  muduo::string content_;
};
}
