#ifndef MUDUO_PROTORPC_ZURG_SINK_H
#define MUDUO_PROTORPC_ZURG_SINK_H

#include <muduo/base/Logging.h>
#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>

namespace zurg
{

// a minimal TcpConnection which only receives.
class Sink : public std::enable_shared_from_this<Sink>,
             muduo::noncopyable
{
 public:
  Sink(muduo::net::EventLoop* loop, int fd, int maxLength, const char* which)
    : loop_(loop),
      fd_(fd),
      maxLength_(maxLength),
      whoami_(which),
      ch_(loop, fd_)
  {
    using std::placeholders::_1;
    LOG_TRACE << whoami_ << fd_;
    ch_.setReadCallback(std::bind(&Sink::onRead, this, _1));
    ch_.setCloseCallback(std::bind(&Sink::onClose, this));
    ch_.doNotLogHup();
  }

  ~Sink()
  {
    LOG_TRACE << whoami_ << fd_;
    assert(!loop_->eventHandling());
    if (fd_ >= 0)
    {
      ch_.remove();
      ::close(fd_);
    }
  }

  void start()
  {
    ch_.tie(shared_from_this());
    ch_.enableReading();
  }

  void stop(int pid)
  {
    if (!ch_.isNoneEvent())
    {
      LOG_WARN << pid << whoami_ << " stop before child hup";
      ch_.disableAll();
    }
  }

  std::string bufferAsStdString() const
  {
    return std::string(buf_.peek(), buf_.readableBytes());
  }

 private:
  void onRead(muduo::Timestamp t)
  {
    int savedErrno = 0;
    ssize_t n = buf_.readFd(fd_, &savedErrno);
    LOG_TRACE << "read fd " << n << " bytes";
    if (n == 0)
    {
      LOG_DEBUG << "disableAll";
      ch_.disableAll();
    }
    if (static_cast<int32_t>(buf_.readableBytes()) > maxLength_)
    {
      ch_.disableAll();
      // ::kill(pid_, SIGPIPE); doesn't work
      loop_->queueInLoop(std::bind(&Sink::delayedClose, shared_from_this()));
    }
  }

  void onClose()
  {
    LOG_DEBUG << "disableAll";
    ch_.disableAll();
  }

  void delayedClose()
  {
    assert(ch_.isNoneEvent());
    ch_.remove();
    ::close(fd_);
    fd_ = -1;
  }

  muduo::net::EventLoop* loop_;
  int fd_;
  int maxLength_;
  const char* whoami_;
  muduo::net::Channel ch_;
  muduo::net::Buffer buf_;
};

}

#endif  // MUDUO_PROTORPC_ZURG_SINK_H
