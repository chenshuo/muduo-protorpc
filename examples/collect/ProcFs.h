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
  void fillMemory(SystemInfo* info);
  void fillStat(SystemInfo* info);
  void fillCpu(SystemInfo_Cpu* cpu, muduo::StringPiece value);
  void fillProcesses(SnapshotRequest_Level, SystemInfo* info);
  void fillProcess(ProcessInfo* info);
  void fillCmdline(int pid, ProcessInfo_Basic* basic);

  bool readFile(const std::string& filename, CacheLevel cache);
  typedef std::function<void(const char*)> DIRFUNC;
  void readDir(const char* dirname, DIRFUNC&& func);

  const std::string& chomp()
  {
    if (!content_.empty() && content_.back() == '\n')
    {
      // why doesn't string have 'pop_back' ?
      content_.resize(content_.size() - 1);
    }
    return content_;
  }

  const int64_t millisecondsPerTick_;
  const int32_t kbPerPage_;  // doesn't work on VAX, which has 512-byte pages
  FileCache file_;
  std::unordered_map<int, int64_t> starttime_;
  int count_;
  // FIXME: std::unordered_map<int, int> ppid_;

  std::string name_;  // for scratch
  std::string filename_;  // for scratch
  std::string content_;  // for scratch
};
}
