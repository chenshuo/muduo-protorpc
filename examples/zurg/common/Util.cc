#include <examples/zurg/common/Util.h>
#include <examples/zurg/slave/SlaveApp.h>

#include <muduo/base/ProcessInfo.h>

#include <string>

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

using namespace muduo;

namespace zurg
{

int g_tempFileCount = 0;

std::string writeTempFile(StringPiece content)
{
  char buf[256];
  ::snprintf(buf, sizeof buf, "/tmp/%s-runScript-%s-%d-%05d-XXXXXX",
             SlaveApp::instance().name().c_str(),
             ProcessInfo::startTime().toString().c_str(),
             ProcessInfo::pid(),
             ++g_tempFileCount);
  int tempfd = ::mkostemp(buf, O_CLOEXEC);
  ::pwrite(tempfd, content.data(), content.size(), 0);
  ::fchmod(tempfd, 0755);
  ::close(tempfd);
  return buf;
}

}
