#ifndef MUDUO_PROTORPC_ZURG_SLAVECONFIG_H
#define MUDUO_PROTORPC_ZURG_SLAVECONFIG_H

#include <muduo/base/Types.h>

namespace zurg
{
using muduo::string;

struct SlaveConfig
{
  SlaveConfig()
    : error_(false),
      listenPort_(-1),
      zombieInterval_(10),
      heartbeatInterval_(10)
  {
  }

  bool valid() const
  {
    return !(error_ || myName_.empty() || masterAddress_.empty());
  }

  string myName_;
  string masterAddress_;
  bool error_;
  int listenPort_;
  int zombieInterval_;
  int heartbeatInterval_;
};

}

#endif  // MUDUO_PROTORPC_ZURG_SLAVECONFIG_H
