#pragma once
#include <muduo/base/ProcessInfo.h>
#include <unordered_map>
#include <fcntl.h>

namespace collect
{

enum CacheLevel
{
  kNoCache, kLRU, kKeep,
};

// a LRU cache for opened file descriptors.
class FileCache : muduo::noncopyable
{
  class File : muduo::noncopyable
  {
   public:
    File(int f = -1) noexcept
      : fd_(f)
    {
    }

    ~File() noexcept
    {
      if (fd_ >= 0)
      {
        ::close(fd_);
      }
    }

    File(File&& rhs) noexcept
      : fd_(rhs.fd_)
    {
      rhs.fd_ = -1;
    }

    File& operator=(File&& rhs) noexcept
    {
      swap(rhs);
      return *this;
    }

    int fd() noexcept { return fd_; }
    bool valid() noexcept { return fd_ >= 0; }

    void swap(File& rhs) noexcept
    {
      std::swap(fd_, rhs.fd_);
    }

   private:
    int fd_;
  };

 public:
  FileCache()
    : maxFiles_(muduo::ProcessInfo::maxOpenFiles())
  {
  }

  int getFile(muduo::StringArg filename, CacheLevel cache)
  {
    assert(cache == kKeep || cache == kLRU);
    
    if (cache == kKeep)
    {
      File& f = keep_[filename.c_str()];
      if (!f.valid())
      {
        f = File(::open(filename.c_str(), O_RDONLY | O_CLOEXEC));
      }
      return f.fd();
    }
    else
    {
      // FIXME:
      abort();
    }
  }

 private:
  std::unordered_map<std::string, File> keep_;
  const int maxFiles_;
};

}

