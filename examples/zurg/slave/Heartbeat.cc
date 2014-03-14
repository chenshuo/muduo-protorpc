#include <examples/zurg/slave/Heartbeat.h>

#include <examples/zurg/slave/SlaveConfig.h>

#include <examples/zurg/proto/master.pb.h>
#include <muduo/protorpc2/rpc2.pb.h>

#include <muduo/base/FileUtil.h>
#include <muduo/base/Logging.h>
#include <muduo/base/ProcessInfo.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/EventLoop.h>

#include <algorithm>

#include <sys/utsname.h>

namespace zurg
{

void ignoreCallback(const ::rpc2::EmptyPtr&)
{
}

// unique_ptr is better
typedef std::shared_ptr<muduo::FileUtil::ReadSmallFile> SmallFilePtr;

class ProcFs : muduo::noncopyable
{
 public:
  typedef std::map<muduo::StringPiece, SmallFilePtr> FileMap;

  // char* to save construction of string.
  int readTo(const char* filename, std::string* out)
  {
    FileMap::iterator it = files_.find(filename);
    if (it == files_.end())
    {
      SmallFilePtr ptr(new muduo::FileUtil::ReadSmallFile(filename));
      it = files_.insert(std::make_pair(filename, ptr)).first;
    }
    assert (it != files_.end());
    int size = 0;
    int err = it->second->readToBuffer(&size);
    LOG_TRACE << filename << " " << err << " " << size;
    if (size > 0)
    {
      out->assign(it->second->buffer(), size);
    }
    return err;
  }

 private:
  FileMap files_;
};

void strip_cpuinfo(std::string* cpuinfo)
{
  // FIXME:
}

void fill_uname(SlaveHeartbeat* hb)
{
  struct utsname buf;
  if (::uname(&buf) == 0)
  {
    SlaveHeartbeat::Uname* p = hb->mutable_uname();
    p->set_sysname(buf.sysname);
    p->set_nodename(buf.nodename);
    p->set_release(buf.release);
    p->set_version(buf.version);
    p->set_machine(buf.machine);
    p->set_domainname(buf.domainname);
  }
  else
  {
    LOG_SYSERR << "uname";
  }
}

void strip_diskstats(std::string* diskstats)
{
  std::string result;
  result.reserve(diskstats->size());
  muduo::StringPiece zeros(" 0 0 0 0 0 0 0 0 0 0 0\n");

  const char* p = diskstats->c_str();
  const char* nl = NULL;
  while ( (nl = ::strchr(p, '\n')) != NULL)
  {
    int pos = static_cast<int>(nl - p + 1);

    muduo::StringPiece line(p, pos);
    if (line.size() > zeros.size())
    {
      muduo::StringPiece end(line.data() + line.size()-zeros.size(), zeros.size());
      if (end != zeros)
      {
        result.append(line.data(), line.size());
      }
    }

    p += pos;
  }
  assert(p == &*diskstats->end());
  diskstats->swap(result);
}

struct AreBothSpaces
{
  bool operator()(char x, char y) const
  {
    return x == ' ' && y == ' ';
  }
};

void strip_meminfo(std::string* meminfo)
{
  std::string::iterator it = std::unique(meminfo->begin(), meminfo->end(), AreBothSpaces());
  meminfo->erase(it, meminfo->end());
}

void strip_stat(std::string* proc_stat)
{
  const char* intr = ::strstr(proc_stat->c_str(), "\nintr ");
  if (intr != NULL)
  {
    const char* nl = ::strchr(intr + 1, '\n');
    assert(nl != NULL);

    muduo::StringPiece line(intr+1, static_cast<int>(nl-intr-1));
    const char* p = nl;
    while (p[-1] == '0' && p[-2] == ' ')
    {
      p -= 2;
    }

    ++p;
    proc_stat->erase(p - proc_stat->c_str(), nl-p);
  }
}

}

extern const char* slave_version;

using namespace zurg;
using namespace muduo;
using namespace muduo::net;

Heartbeat::Heartbeat(muduo::net::EventLoop* loop,
                     const SlaveConfig& config,
                     MasterService_Stub* stub)
  : loop_(loop),
    name_(config.name_),
    port_(config.listenPort_),
    stub_(stub),
    procFs_(new ProcFs),
    beating_(false)
{
  loop_->runEvery(config.heartbeatInterval_, std::bind(&Heartbeat::onTimer, this));
}

Heartbeat::~Heartbeat()
{
}

void Heartbeat::start()
{
  beating_ = true;
  beat(true);
}

void Heartbeat::stop()
{
  beating_ = false;
}

void Heartbeat::onTimer()
{
  if (beating_)
  {
    beat(false);
  }
}

#define FILL_HB(PROC_FILE, FIELD)               \
  if (procFs_->readTo(PROC_FILE, hb.mutable_##FIELD()) != 0)     \
      hb.clear_##FIELD();

void Heartbeat::beat(bool showStatic)
{
  LOG_DEBUG << (showStatic ? "full" : "");
  SlaveHeartbeat hb;
  hb.set_slave_name(name_);
  if (showStatic)
  {
    hb.set_host_name(ProcessInfo::hostname().c_str());
    if (port_ > 0)
    {
      hb.set_listen_port(port_);
    }
    hb.set_slave_pid(ProcessInfo::pid());
    hb.set_start_time_us(ProcessInfo::startTime().microSecondsSinceEpoch());
    hb.set_slave_version(slave_version);
    FILL_HB("/proc/cpuinfo", cpuinfo);
    FILL_HB("/proc/version", version);
    FILL_HB("/etc/mtab", etc_mtab);
    strip_cpuinfo(hb.mutable_cpuinfo());
    // sysctl
    fill_uname(&hb);
  }
  hb.set_send_time_us(Timestamp::now().microSecondsSinceEpoch());

  FILL_HB("/proc/meminfo", meminfo);
  FILL_HB("/proc/stat", proc_stat);
  FILL_HB("/proc/loadavg", loadavg);
  FILL_HB("/proc/diskstats", diskstats);
  FILL_HB("/proc/net/dev", net_dev);
  FILL_HB("/proc/net/tcp", net_tcp);
  strip_diskstats(hb.mutable_diskstats());
  strip_meminfo(hb.mutable_meminfo());
  strip_stat(hb.mutable_proc_stat());

  stub_->slaveHeartbeat(hb, ignoreCallback);
}
