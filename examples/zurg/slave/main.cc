#include <examples/zurg/slave/SlaveApp.h>

#include <examples/zurg/common/ProtobufLog.h>
#include <examples/zurg/slave/SlaveConfig.h>

#include <muduo/base/ProcessInfo.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace muduo;
using namespace zurg;

void usage(const char* prog)
{
  printf("Usage\n%s [-d listen_port] -n instance_name master_ip:master_port\n", prog);
}

SlaveConfig parseCommandLine(int argc, char* argv[])
{
  int opt = 0;
  SlaveConfig config;
  while ( (opt = ::getopt(argc, argv, "d:n:")) != -1)
  {
    switch (opt)
    {
      case 'd':
        config.listenPort_ = atoi(optarg);
        break;
      case 'n':
        config.name_ = optarg;
        break;
      default:
        config.error_ = true;
        ;
    }
  }

  if (optind < argc)
  {
    config.masterAddress_ = argv[optind];
  }

  return config;
}

int main(int argc, char* argv[])
{
  if (ProcessInfo::uid() == 0 || ProcessInfo::euid() == 0)
  {
    fprintf(stderr, "Cannot run as root.\n");
    return 1;
  }

  // we can't setrlimit as it will be inherited by child processes.

  google::protobuf::SetLogHandler(zurgLogHandler);
  SlaveConfig config(parseCommandLine(argc, argv));
  if (config.valid())
  {
    SlaveApp app(config);
    app.run();
    return 0;
  }
  else
  {
    usage(argv[0]);
    return 1;
  }
}
