#include <examples/zurg/common/Util.h>

#include <muduo/base/Logging.h>
#include <muduo/base/ProcessInfo.h>

#include <string>

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/stat.h>

using namespace muduo;

namespace zurg
{

int g_tempFileCount = 0;

std::string writeTempFile(StringPiece prefix, StringPiece content)
{
  char buf[256];
  ::snprintf(buf, sizeof buf, "/tmp/%s-runScript-%s-%d-%05d-XXXXXX",
             prefix.data(),
             ProcessInfo::startTime().toString().c_str(),
             ProcessInfo::pid(),
             ++g_tempFileCount);
  int tempfd = ::mkostemp(buf, O_CLOEXEC);
  ssize_t n = ::pwrite(tempfd, content.data(), content.size(), 0);
  ::fchmod(tempfd, 0755);
  ::close(tempfd);
  return n == content.size() ? buf : "";
}

void parseMd5sum(const std::string& lines, std::map<StringPiece, StringPiece>* md5sums)
{
  const char* p = lines.c_str();
  size_t nl = 0;
  while (*p)
  {
    StringPiece md5(p, 32);
    nl = lines.find('\n', nl);
    assert(nl != std::string::npos);
    StringPiece file(p+34, static_cast<int>(lines.c_str()+nl-p-34));
    (*md5sums)[file] = md5;
    p = lines.c_str()+nl+1;
    ++nl;
  }
}

void setupWorkingDir(const std::string& cwd)
{
  ::mkdir(cwd.c_str(), 0755); // don't check return value
  char buf[256];
  ::snprintf(buf, sizeof buf, "/%s/XXXXXX", cwd.c_str());
  int fd = ::mkstemp(buf);
  if (fd < 0)
  {
    LOG_FATAL << '/' << cwd << " is not accessible.";
  }
  ::close(fd);
  ::unlink(buf);

  ::snprintf(buf, sizeof buf, "/%s/pid", cwd.c_str());
  fd = ::open(buf, O_WRONLY | O_CREAT | O_CLOEXEC, 0644);
  if (fd < 0)
  {
    LOG_FATAL << "Failed to create pid file at " << buf;
  }

  if (::flock(fd, LOCK_EX | LOCK_NB))
  {
    LOG_SYSFATAL << "Failed to lock pid file at " << buf
                 << ", another instance running ?";
  }

  if (::ftruncate(fd, 0))
  {
    LOG_SYSERR << "::ftruncate " << buf;
  }

  char pid[32];
  int len = ::snprintf(pid, sizeof pid, "%d\n", ProcessInfo::pid());
  ssize_t n = ::write(fd, pid, len);
  assert(n == len); (void)n;
  // leave fd open, to keep the lock
}

void setNonBlockAndCloseOnExec(int fd)
{
  // non-block
  int flags = ::fcntl(fd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  int ret = ::fcntl(fd, F_SETFL, flags);
  // FIXME check

  // close-on-exec
  flags = ::fcntl(fd, F_GETFD, 0);
  flags |= FD_CLOEXEC;
  ret = ::fcntl(fd, F_SETFD, flags);
  // FIXME check

  (void)ret;
}

}
