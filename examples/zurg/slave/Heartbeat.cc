#include <examples/zurg/slave/Heartbeat.h>

#include <examples/zurg/slave/SlaveConfig.h>

#include <examples/zurg/proto/master.pb.h>
#include <muduo/protorpc2/rpc2.pb.h>

#include <muduo/base/FileUtil.h>
#include <muduo/base/Logging.h>
#include <muduo/base/ProcessInfo.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/EventLoop.h>

namespace zurg
{

void ignoreCallback(const EmptyPtr&)
{
}

// unique_ptr is better
typedef boost::shared_ptr<muduo::FileUtil::SmallFile> SmallFilePtr;

class ProcFs : boost::noncopyable
{
 public:
  typedef std::map<muduo::StringPiece, SmallFilePtr> FileMap;

  // char* to save construction of string.
  int readTo(const char* filename, std::string* out)
  {
    FileMap::iterator it = files_.find(filename);
    if (it == files_.end())
    {
      SmallFilePtr ptr(new muduo::FileUtil::SmallFile(filename));
      it = files_.insert(std::make_pair(filename, ptr)).first;
    }
    assert (it != files_.end());
    int size = 0;
    int err = it->second->readToBuffer(&size);
    LOG_DEBUG << filename << " " << err << " " << size;
    if (size > 0)
    {
      out->assign(it->second->buffer(), size);
    }
    return err;
  }

 private:
  FileMap files_;
};
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
    stub_(stub),
    procFs_(new ProcFs),
    beating_(false)
{
  loop_->runEvery(config.heartbeatInterval_, boost::bind(&Heartbeat::onTimer, this));
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
  LOG_DEBUG << showStatic;
  SlaveHeartbeat hb;
  hb.set_slave_name(name_);
  if (showStatic)
  {
    hb.set_host_name(ProcessInfo::hostname().c_str());
    hb.set_slave_pid(ProcessInfo::pid());
    hb.set_start_time_us(ProcessInfo::startTime().microSecondsSinceEpoch());
    hb.set_slave_version(slave_version);
    FILL_HB("/proc/cpuinfo", cpuinfo);
    FILL_HB("/proc/version", version);
    FILL_HB("/etc/mtab", etc_mtab);
  }
  hb.set_send_time_us(Timestamp::now().microSecondsSinceEpoch());

  FILL_HB("/proc/meminfo", meminfo);
  FILL_HB("/proc/stat", proc_stat);
  FILL_HB("/proc/loadavg", loadavg);
  FILL_HB("/proc/diskstats", diskstats);
  FILL_HB("/proc/net/dev", net_dev);
  FILL_HB("/proc/net/tcp", net_tcp);

  stub_->slaveHeartbeat(hb, ignoreCallback);
}
