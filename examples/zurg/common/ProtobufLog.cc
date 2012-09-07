#include <examples/zurg/common/ProtobufLog.h>
#include <muduo/base/Logging.h>

namespace zurg
{

muduo::Logger::LogLevel toMuduoLogLevel(google::protobuf::LogLevel level)
{
  switch(level)
  {
    case google::protobuf::LOGLEVEL_WARNING:
      return muduo::Logger::WARN;
    case google::protobuf::LOGLEVEL_ERROR:
      return muduo::Logger::ERROR;
    case google::protobuf::LOGLEVEL_FATAL:
      return muduo::Logger::FATAL;
    case google::protobuf::LOGLEVEL_INFO:
      return muduo::Logger::INFO;
    default:
      return muduo::Logger::INFO;
  }
}

void zurgLogHandler(google::protobuf::LogLevel level,
                    const char* filename,
                    int line,
                    const std::string& message)
{
  muduo::Logger::LogLevel logLevel = toMuduoLogLevel(level);
  if (muduo::Logger::logLevel() <= logLevel)
  {
    muduo::Logger(muduo::Logger::SourceFile(filename),
                  line,
                  logLevel).stream() << "Protobuf - " << message;
  }
}

}
