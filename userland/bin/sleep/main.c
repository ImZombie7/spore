#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static unsigned long monotonic_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (unsigned long)ts.tv_sec * 1000ul + (unsigned long)ts.tv_nsec / 1000000ul;
}

int main(int argc, char **argv) {
  if (argc != 2) { return usage("sleep", "SECONDS"); }
  unsigned long ms = strtoul(argv[1], NULL, 10) * 1000ul;
  unsigned long end = monotonic_ms() + ms;
  while (monotonic_ms() < end) {}
  return EXIT_SUCCESS;
}
