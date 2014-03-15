#include <examples/collect/collect.pb.h>

#include <muduo/base/FileUtil.h>
#include <muduo/base/StringPiece.h>

namespace collect
{

class ProcFs : muduo::noncopyable
{
 public:
  ProcFs();

  bool fill(SnapshotRequest_Level, SystemInfo* info);

 private:
  enum CacheLevel
  {
    kNoCache, kLRU, kKeep,
  };

  void fillInitial(SystemInfo* info);
  void fillLoadAvg(SystemInfo* info);
  void fillStat(SystemInfo* info);
  void fillCpu(SystemInfo_Cpu* cpu, muduo::StringPiece value);

  // FIXME: cache file descriptor
  bool readFile(muduo::StringArg filename, CacheLevel cache)
  {
    return 0 == muduo::FileUtil::readFile(filename, 64*1024, &content_);
  }

  const std::string& chomp()
  {
    if (!content_.empty() && content_.back() == '\n')
    {
      content_.resize(content_.size() - 1);
    }
    return content_;
  }

  const int64_t millisecondsInOneTick_;

  std::string content_;  // for scratch
};
}
