#include <examples/collect/ProcFs.h>
#include <examples/collect/collect.pb.h>

#include <muduo/base/Logging.h>
#include <muduo/base/ProcessInfo.h>

// namespace ProcessInfo = muduo::ProcessInfo;
using muduo::Timestamp;
using muduo::StringPiece;

namespace collect
{

int64_t toMilliseconds(double seconds)
{
  return static_cast<int64_t>(seconds * 1000.0 + 0.5);
}

ProcFs::ProcFs()
  : millisecondsInOneTick_(1000 / muduo::ProcessInfo::clockTicksPerSecond())
{
  long hz = muduo::ProcessInfo::clockTicksPerSecond();
  if (1000 % hz != 0)
  {
    LOG_FATAL << "_SC_CLK_TCK is not a divisible";
  }
}

bool ProcFs::fill(SnapshotRequest_Level level, SystemInfo* info)
{
  info->set_muduo_timestamp(Timestamp::now().microSecondsSinceEpoch());
  muduo::ProcessInfo::CpuTime t = muduo::ProcessInfo::cpuTime();
  info->set_user_cpu_ms(toMilliseconds(t.userSeconds));
  info->set_sys_cpu_ms(toMilliseconds(t.systemSeconds));

  if (level >= SnapshotRequest_Level_kSystemInfoAndProcesses)
  {
    if (level >= SnapshotRequest_Level_kSystemInfoAndThreads)
    {
      if (level >= SnapshotRequest_Level_kSystemInfoInitialSnapshot)
      {
        fillInitial(info);
      }
    }
  }
  fillLoadAvg(info);
  fillStat(info);
  return true;
}

bool ProcFs::readFile(muduo::StringArg filename, CacheLevel cache)
{
  if (cache == kNoCache)
  {
    return 0 == muduo::FileUtil::readFile(filename, 64*1024, &content_);
  }
  else
  {
    int fd = file_.getFile(filename, cache);
    if (fd >= 0)
    {
      char buf[64*1024];
      ssize_t n = ::pread(fd, buf, sizeof(buf), 0);
      if (n >= 0)
      {
        content_.assign(buf, n);
        return true;
      }
    }
    return false;
  }
}

void ProcFs::fillInitial(SystemInfo* info)
{
  // kernel versoin
  if (readFile("/proc/version", kNoCache))
  {
    info->mutable_basic()->set_kernel_version(chomp());
  }
  // kernel command line
  if (readFile("/proc/cmdline", kNoCache))
  {
    info->mutable_basic()->set_kernel_cmdline(chomp());
  }
  // cpuinfo
  if (readFile("/proc/cpuinfo", kNoCache))
  {
    info->mutable_basic()->set_cpuinfo(content_);
  }
}

void ProcFs::fillLoadAvg(SystemInfo* info)
{

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
      perf->set_loadavg_1m_milli(static_cast<int>(toMilliseconds(loadavg_1m)));
      perf->set_loadavg_5m_milli(static_cast<int>(toMilliseconds(loadavg_5m)));
      perf->set_loadavg_15m_milli(static_cast<int>(toMilliseconds(loadavg_15m)));
      perf->set_running_tasks(running);
      perf->set_total_tasks(total);
      perf->set_last_pid(lastpid);
    }
  }
}

struct LineReader
{
 public:
  LineReader(StringPiece content)
    : content_(content)
  {
    next();
  }

  void next()
  {
    const char* pos = static_cast<const char*>(::memchr(content_.data(), '\n', content_.size()));
    if (pos)
    {
      line_.set(content_.data(), static_cast<int>(pos - content_.data()));
      content_.set(pos+1, content_.size() - line_.size() - 1);
    }
    else
    {
      line_ = content_;
      content_.set("");
    }

    // FIXME: doesn't work for /proc/pid/status, '\t' is the seperator
    const char* sp = static_cast<const char*>(::memchr(line_.data(), ' ', line_.size()));
    if (sp)
    {
      key.set(line_.data(), static_cast<int>(sp - line_.data()));
      value.set(sp+1, line_.size() - key.size() - 1);
    }
    else
    {
      key.set("");
      value.set("");
    }
  }

  int64_t valueAsInt64() const
  {
    return static_cast<int64_t>(::strtoll(value.data(), NULL, 10));
  }

  int32_t valueAsInt32() const
  {
    return static_cast<int32_t>(::strtol(value.data(), NULL, 10));
  }

  StringPiece content_;
  StringPiece line_;
  StringPiece key;
  StringPiece value;
};

void ProcFs::fillCpu(SystemInfo_Cpu* cpu, StringPiece value)
{
  long user = 0, nice = 0, sys = 0, idle = 0;
  long iowait = 0, irq = 0, softirq = 0;

  if (sscanf(value.data(), "%ld %ld %ld %ld %ld %ld %ld",
             &user, &nice, &sys, &idle,
             &iowait, &irq, &softirq) == 7)
  {
    cpu->set_user_ms(user * millisecondsInOneTick_);
    cpu->set_nice_ms(nice * millisecondsInOneTick_);
    cpu->set_sys_ms(sys * millisecondsInOneTick_);
    cpu->set_idle_ms(idle * millisecondsInOneTick_);
    cpu->set_iowait_ms(iowait * millisecondsInOneTick_);
    cpu->set_irq_ms(irq * millisecondsInOneTick_);
    cpu->set_softirq_ms(softirq * millisecondsInOneTick_);
  }
}

void ProcFs::fillStat(SystemInfo* info)
{
  if (readFile("/proc/stat", kKeep))
  {
    LineReader r(content_);
    if (r.key == "cpu")
    {
      auto cpu = info->mutable_all_cpu();
      fillCpu(cpu, r.value);
    }

    r.next();
    while (r.key.starts_with("cpu"))
    {
      auto cpu = info->add_cpus();
      fillCpu(cpu, r.value);
      r.next();
    }

    if (r.key == "intr")
    {
      // FIXME
    }

    auto perf = info->mutable_performance();

    r.next();
    if (r.key == "ctxt")
    {
      perf->set_context_switches(r.valueAsInt64());
    }

    r.next();
    if (r.key == "btime")
    {
      perf->set_boot_time(r.valueAsInt64());
    }

    r.next();
    if (r.key == "processes")
    {
      perf->set_processes_created(r.valueAsInt64());
    }

    r.next();
    if (r.key == "procs_running")
    {
      perf->set_processes_running(r.valueAsInt32());
    }

    r.next();
    if (r.key == "procs_blocked")
    {
      perf->set_processes_blocked(r.valueAsInt32());
    }

    r.next();
    if (r.key == "softirq")
    {
      // FIXME
    }
  }
}

}
