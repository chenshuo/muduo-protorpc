#pragma once
#include <muduo/base/Logging.h>
#include <muduo/base/ProcessInfo.h>
#include <list>
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
    File() noexcept
      : fd_(-1)
    {
    }

    File(const std::string& filename)
      : fd_(::open(filename.c_str(), O_RDONLY | O_CLOEXEC))
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
  FileCache(unsigned maxFiles)
    : maxFiles_(maxFiles)
  {
  }

  int getFile(const std::string& filename, CacheLevel cache)
  {
    assert(cache == kKeep || cache == kLRU);

    if (cache == kKeep)
    {
      File& f = keep_[filename];
      if (!f.valid())
      {
        f = File(filename);
      }
      return f.fd();
    }
    else
    {
      assert(list_.size() == map_.size());
      auto it = map_.find(filename);
      if (it != map_.end())
      {
        list_.splice(list_.begin(), list_, it->second.second);
        return it->second.first.fd();
      }
      else
      {
        list_.push_front(filename);
        // emplace is not available in g++ 4.6
        auto result = map_.insert(std::make_pair(filename, std::make_pair(File(filename), list_.begin())));
        assert(result.second);
        assert(list_.size() == map_.size());
        if (map_.size() > maxFiles_)
        {
          map_.erase(list_.back());
          list_.pop_back();
        }
        assert(list_.size() == map_.size());
        return result.first->second.first.fd();  // 1-2-1 1-2-1 1-2-1
      }
    }
  }

  void closeFile(const std::string& filename)
  {
    auto it = map_.find(filename);
    if (it != map_.end())
    {
      list_.erase(it->second.second);
      map_.erase(it);
    }
  }

  void print()
  {
    for (auto& x : list_)
    {
      printf("%s\n", x.c_str());
    }
    puts("==========");
  }

  void setSentryFile(const std::string& sentry)
  {
    getFile(sentry, kLRU);
  }

  void closeUnusedFiles(const std::string& sentry)
  {
    if (map_.count(sentry) > 0)
    {
      while (!list_.empty() && list_.back() != sentry)
      {
        LOG_DEBUG << "close " << list_.back();
        map_.erase(list_.back());
        list_.pop_back();
      }
    }
  }

 private:
  std::unordered_map<std::string, File> keep_;
  const unsigned maxFiles_;
  std::list<std::string> list_;
  typedef std::pair<File, std::list<std::string>::iterator> Entry;
  std::unordered_map<std::string, Entry> map_;
};

}

