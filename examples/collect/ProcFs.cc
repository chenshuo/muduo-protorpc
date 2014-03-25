#include <examples/collect/ProcFs.h>
#include <examples/collect/collect.pb.h>

#include <muduo/base/Logging.h>
#include <muduo/base/ProcessInfo.h>

#include <dirent.h>

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
  : millisecondsPerTick_(1000 / muduo::ProcessInfo::clockTicksPerSecond()),
    kbPerPage_(muduo::ProcessInfo::pageSize() / 1024),
    file_(std::min(1000, muduo::ProcessInfo::maxOpenFiles() - 100)),
    count_(0)
{
  long hz = muduo::ProcessInfo::clockTicksPerSecond();
  if (1000 % hz != 0)
  {
    LOG_FATAL << "_SC_CLK_TCK is not a divisible";
  }
  if (muduo::ProcessInfo::pageSize() % 1024 != 0)
  {
    LOG_FATAL << "_SC_PAGE_SIZE is not a divisible by 1024";
  }
  name_.reserve(20);
  filename_.reserve(60);
  content_.reserve(500);
}

bool ProcFs::fill(SnapshotRequest_Level level, SystemInfo* info)
{
  if (++count_ >= 60)
  {
    count_ = 0;
    starttime_.clear();
  }
  filename_ = "/dev/null";
  file_.setSentryFile(filename_);
  info->set_muduo_timestamp(Timestamp::now().microSecondsSinceEpoch());
  muduo::ProcessInfo::CpuTime t = muduo::ProcessInfo::cpuTime();
  info->set_user_cpu_ms(toMilliseconds(t.userSeconds));
  info->set_sys_cpu_ms(toMilliseconds(t.systemSeconds));

  if (level >= SnapshotRequest_Level_kSystemInfoAndProcesses)
  {
    fillProcesses(level, info);
  }
  if (level >= SnapshotRequest_Level_kSystemInfoInitialSnapshot)
  {
    fillInitial(info);
  }
  fillLoadAvg(info);
  fillMemory(info);
  fillStat(info);
  filename_ = "/dev/null";
  file_.closeUnusedFiles(filename_);
  return true;
}

bool ProcFs::readFile(const std::string& filename, CacheLevel cache)
{
  if (cache == kNoCache)
  {
    return 0 == muduo::FileUtil::readFile(filename, 64*1024, &content_);
  }
  else
  {
    int fd = file_.getFile(filename, cache);
    // file_.print();
    if (fd >= 0)
    {
      char buf[64*1024];
      ssize_t n = ::pread(fd, buf, sizeof(buf), 0);
      if (n >= 0)
      {
        content_.assign(buf, n);
        return true;
      }
      else
      {
        LOG_ERROR << "read error " << filename;
        file_.closeFile(filename);
        // FIXME: reopen and retry
      }
    }
    return false;
  }
}

class Defer : muduo::noncopyable
{
 public:
  Defer(std::function<void()>&& f)
    : func_(std::move(f))
  {
  }

  ~Defer()
  {
    func_();
  }
 private:
  std::function<void()> func_;
};

void ProcFs::readDir(const char* dirname, DIRFUNC&& func)
{
  DIR* dir = ::opendir(dirname);
  if (dir)
  {
    // std::shared_ptr<void> x = std::shared_ptr<DIR>(dir, ::closedir);
    Defer d1([dir]{ ::closedir(dir); });
    struct dirent* ent;
    while ( (ent = ::readdir(dir)) != NULL)
    {
      func(ent->d_name);
    }
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
  filename_ = "/proc/loadavg";
  if (readFile(filename_, kKeep))
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
      content_.remove_prefix(line_.size() + 1);
    }
    else
    {
      line_ = content_;
      content_ = "";
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
      key = "";
      value = "";
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

void ProcFs::fillMemory(SystemInfo* info)
{
  filename_ = "/proc/meminfo";
  if (readFile(filename_, kKeep))
  {
    LineReader r(content_);
    if (r.key == "MemTotal:")
      info->mutable_memory()->set_total_kb(r.valueAsInt64());
    r.next();
    if (r.key == "MemFree:")
      info->mutable_memory()->set_free_kb(r.valueAsInt64());
    r.next();
    if (r.key == "Buffers:")
      info->mutable_memory()->set_buffers_kb(r.valueAsInt64());
    r.next();
    if (r.key == "Cached:")
      info->mutable_memory()->set_cached_kb(r.valueAsInt64());
    r.next();
    if (r.key == "SwapCached:")
      info->mutable_memory()->set_swap_cached_kb(r.valueAsInt64());
    r.next();
    while (r.key != "SwapTotal:")
      r.next();
    if (r.key == "SwapTotal:")
      info->mutable_memory()->set_swap_total_kb(r.valueAsInt64());
    r.next();
    if (r.key == "SwapFree:")
      info->mutable_memory()->set_swap_free_kb(r.valueAsInt64());
  }
}

void ProcFs::fillCpu(SystemInfo_Cpu* cpu, StringPiece value)
{
  long user = 0, nice = 0, sys = 0, idle = 0;
  long iowait = 0, irq = 0, softirq = 0;

  if (sscanf(value.data(), "%ld %ld %ld %ld %ld %ld %ld",
             &user, &nice, &sys, &idle,
             &iowait, &irq, &softirq) == 7)
  {
    cpu->set_user_ms(user * millisecondsPerTick_);
    cpu->set_nice_ms(nice * millisecondsPerTick_);
    cpu->set_sys_ms(sys * millisecondsPerTick_);
    cpu->set_idle_ms(idle * millisecondsPerTick_);
    cpu->set_iowait_ms(iowait * millisecondsPerTick_);
    cpu->set_irq_ms(irq * millisecondsPerTick_);
    cpu->set_softirq_ms(softirq * millisecondsPerTick_);
  }
}

void ProcFs::fillStat(SystemInfo* info)
{
  filename_ = "/proc/stat";
  if (readFile(filename_, kKeep))
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

void ProcFs::fillProcesses(SnapshotRequest_Level level, SystemInfo* info)
{
  readDir("/proc", [=](const char* name) {
    if (::isdigit(name[0]))
    {
      //int pid = atoi(name);
      char buf[64];
      snprintf(buf, sizeof buf, "/proc/%s/stat", name);
      filename_ = buf;
      if (readFile(filename_, kLRU))
      {
        auto proc = info->add_processes();
        fillProcess(proc);
        if (level >= SnapshotRequest_Level_kSystemInfoAndThreads)
        {
          // fillThreads(pid, info);
        }
      }
    }
  });
}

struct FieldReader
{
 public:
  FieldReader(StringPiece content)
    : content_(content)
  {
  }

  void next()
  {
    const char* sp = static_cast<const char*>(::memchr(content_.data(), ' ', content_.size()));
    if (sp)
    {
      moveForward(sp+1);
    }
    else
    {
      content_ = "";
    }
  }

  int64_t readInt64()
  {
    char* end = NULL;
    int64_t x = static_cast<int64_t>(::strtoll(content_.data(), &end, 10));
    while (*end == ' ')
      ++end;
    moveForward(end);
    return x;
  }

  int32_t readInt32()
  {
    char* end = NULL;
    int x = static_cast<int32_t>(::strtol(content_.data(), &end, 10));
    while (*end == ' ')
      ++end;
    moveForward(end);
    return x;
  }

  void readName(std::string* result)
  {
    assert(content_[0] == '(');
    const char* end = static_cast<const char*>(::memchr(content_.data(), ')', content_.size()));
    if (end)
    {
      result->assign(content_.data()+1, end);
      moveForward(end+2);
    }
  }

  void moveForward(const char* newstart)
  {
    assert(content_.begin() <= newstart && newstart < content_.end());
    content_.remove_prefix(static_cast<int>(newstart - content_.begin()));
    assert(newstart == content_.data());
  }

  StringPiece content_;
};

void ProcFs::fillProcess(ProcessInfo* info)
{
  FieldReader r(content_);
  int pid = r.readInt32();
  info->set_pid(pid);
  r.readName(&name_);
  char state = r.content_[0];
  r.next();
  info->set_state(state);
  int ppid = r.readInt32();
  r.next(); // pgrp
  r.next(); // session
  r.next(); // tty_nr
  r.next(); // tpgid
  r.next(); // flags
  info->set_minor_page_faults(r.readInt64());
  r.next(); // cminflt
  info->set_major_page_faults(r.readInt64());
  r.next(); // cmajflt
  info->set_user_cpu_ms(r.readInt64() * millisecondsPerTick_);
  info->set_sys_cpu_ms(r.readInt64() * millisecondsPerTick_);
  r.next(); // cutime
  r.next(); // cstime
  r.next(); // priority
  r.next(); // nice
  info->set_num_threads(r.readInt32());
  r.next(); // iteralvalue
  int64_t starttime = r.readInt64();
  info->set_vsize_kb(r.readInt64() / 1024);
  info->set_rss_kb(r.readInt64() * kbPerPage_);
  r.next(); // rsslim
  r.next(); // startcode
  r.next(); // endcode
  r.next(); // startstack
  r.next(); // kstkesp
  r.next(); // kstkeip
  r.next(); // signal
  r.next(); // blocked
  r.next(); // sigignore
  r.next(); // sigcatch
  int64_t wait_channel = r.readInt64();
  r.next(); // nswap
  r.next(); // cnswap
  r.next(); // exit_signal
  info->set_last_processor(r.readInt32());
  (void)wait_channel;
  // FIXME: info->set_wait_channel(r.readInt64());

  auto it = starttime_.find(pid);
  if (it == starttime_.end() || it->second != starttime)
  {
    starttime_[pid] = starttime;
    // FIXME: uid
    info->mutable_basic()->set_ppid(ppid);
    info->mutable_basic()->set_starttime(starttime);
    info->mutable_basic()->set_name(name_);
    fillCmdline(pid, info->mutable_basic());
    LOG_DEBUG << "new process " << pid << " "  << name_;
  }
}

void ProcFs::fillCmdline(int pid, ProcessInfo_Basic* basic)
{
  char buf[64];
  snprintf(buf, sizeof buf, "/proc/%d/cmdline", pid);
  filename_ = buf;
  if (readFile(filename_, kNoCache))
  {
    const char* end = content_.data() + content_.size();
    for (const char* s = content_.data(); s < end;)
    {
      size_t len = strlen(s);
      basic->add_cmdline(s);
      s += len+1;
    }
  }
  snprintf(buf, sizeof buf, "/proc/%d/exe", pid);
  char executable[1024];
  ssize_t len = ::readlink(buf, executable, sizeof executable);
  if (len > 0)
  {
    basic->set_executable(executable, len);
  }
}

}
