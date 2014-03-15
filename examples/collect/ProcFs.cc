#include <examples/collect/ProcFs.h>
#include <examples/collect/collect.pb.h>

#include <muduo/base/Logging.h>

namespace collect
{

bool ProcFs::fill(SnapshotRequest_Level level, SystemInfo* info)
{
  fillLoadAvg(info);

  if (level >= SnapshotRequest_Level_kSystemInfoAndProcesses)
  {
    if (level >= SnapshotRequest_Level_kSystemInfoAndThreads)
    {
    }
  }
  return true;
}

void ProcFs::fillLoadAvg(SystemInfo* info)
{
  auto toMills = [](double x) { return static_cast<int>(x * 1000.0 + 0.5); };

  if (readFile("/proc/loadavg", kKeep))
  {
    // LOG_INFO << "loadavg " << content_;
    double loadavg_1m = 0, loadavg_5m = 0, loadavg_15m = 0;
    int running = 0, total = 0, lastpid = 0;
    if (sscanf(content_.c_str(), "%lf %lf %lf %d/%d %d",
               &loadavg_1m, &loadavg_5m, &loadavg_15m,
               &running, &total, &lastpid) == 6)
    {
      auto perf = info->mutable_performance();
      perf->set_loadavg_1m_milli(toMills(loadavg_1m));
      perf->set_loadavg_5m_milli(toMills(loadavg_5m));
      perf->set_loadavg_15m_milli(toMills(loadavg_15m));
      perf->set_running_tasks(running);
      perf->set_total_tasks(total);
      perf->set_last_pid(lastpid);
    }
  }
}
}
