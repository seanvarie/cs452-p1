#include <stdio.h>
#include "../src/lab.h"

int main(int argc, char * argv[])
{
  int opt = -1;

  while((opt = getopt(argc, argv, "v")) != -1)
  {
    switch (opt)
    {
      case 'v':
        printf("Simple Shell version %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
      break;
    }
  }
  return 0;
}
