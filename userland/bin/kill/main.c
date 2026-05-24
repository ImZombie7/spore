#include <spore.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  int sig = SIGTERM;
  int first = 1;
  if (argc > 2 && streq(argv[1], "-s")) {
    sig = atoi(argv[2]);
    first = 3;
  } else if (argc > 1 && argv[1][0] == '-' && argv[1][1] >= '0' && argv[1][1] <= '9') {
    sig = atoi(argv[1] + 1);
    first = 2;
  }
  if (first >= argc) { return usage("kill", "[-s SIGNAL] PID..."); }
  int rc = EXIT_SUCCESS;
  for (int i = first; i < argc; ++i) {
    if (kill(atoi(argv[i]), sig) != 0) {
      perror(argv[i]);
      rc = EXIT_FAILURE;
    }
  }
  return rc;
}
