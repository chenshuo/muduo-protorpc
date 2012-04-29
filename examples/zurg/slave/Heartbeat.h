#ifndef MUDUO_PROTORPC_ZURG_HEARTBEAT_H
#define MUDUO_PROTORPC_ZURG_HEARTBEAT_H

#include <boost/noncopyable.hpp>

namespace zurg
{

class Heartbeat : boost::noncopyable
{
 public:
  void start();
 private:
};

}
#endif  // MUDUO_PROTORPC_ZURG_HEARTBEAT_H
