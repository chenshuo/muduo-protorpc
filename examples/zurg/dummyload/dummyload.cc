#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void segfault()
{
  int* p = NULL;
  *p = 1;
}

void output(FILE* file, int bytes)
{
  const char* charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\n";
  int len = static_cast<int>(strlen(charset));
  for (int i = 0; i < bytes; ++i)
  {
    putc(charset[i % len], file);
  }
}

int main(int argc, char* argv[])
{
  int opt = 0;
  while ( (opt = getopt(argc, argv, "ace:m:o:ps:v")) != -1)
  {
    switch (opt)
    {
      case 'a':
        assert(false && "something is wrong");
        break;
      case 'c':
        segfault();
        break;
      case 'e':
        output(stderr, atoi(optarg));
        break;
      case 'm':
        free(calloc(atoi(optarg), 1024));
        break;
      case 'o':
        output(stdout, atoi(optarg));
        break;
      case 'p':
        {
          char buf[PATH_MAX] = { 0 };
          puts(getcwd(buf, sizeof(buf) - 1));
        }
        break;
      case 's':
        usleep(static_cast<useconds_t>(atof(optarg) * 1000000));
        break;
      case 'v':
        for (int i = 0; i < argc; ++i)
        {
          printf("%d '%s'\n", i, argv[i]);
        }
        fflush(stdout);
        break;
      default:
        fprintf(stderr, "unknown option '%c'\n", opt);
        return 1;
    }
  }
  return 0;
}
