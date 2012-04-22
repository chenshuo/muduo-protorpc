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
  printf("Usage\n%s [-d listen_port] master_ip:master_port\n", prog);
}

SlaveConfig parseCommandLine(int argc, char* argv[])
{
  bool succeed = true;

  int opt = 0;
  int listenPort = -1;
  while ( (opt = ::getopt(argc, argv, "d:")) != -1)
  {
    switch (opt)
    {
      case 'd':
        listenPort = atoi(optarg);
        break;
      default:
        succeed = false;
        ;
    }
  }

  const char* masterAddress = NULL;
  if (optind >= argc)
  {
    succeed = false;
  }
  else
  {
    masterAddress = argv[optind];
  }

  return succeed ? SlaveConfig(masterAddress, listenPort) : SlaveConfig();
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
  if (config.succeed_)
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
