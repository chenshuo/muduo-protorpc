#pragma once
#include <examples/collect/collect.pb.h>
#include <examples/collect/FileCache.h>

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
  void fillInitial(SystemInfo* info);
  void fillLoadAvg(SystemInfo* info);
  void fillStat(SystemInfo* info);
  void fillCpu(SystemInfo_Cpu* cpu, muduo::StringPiece value);

  bool readFile(muduo::StringArg filename, CacheLevel cache);

  const std::string& chomp()
  {
    if (!content_.empty() && content_.back() == '\n')
    {
      // why doesn't string have 'pop_back' ?
      content_.resize(content_.size() - 1);
    }
    return content_;
  }

  const int64_t millisecondsInOneTick_;
  FileCache file_;

  std::string content_;  // for scratch
};
}
