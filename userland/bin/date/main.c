#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  printf("Spore uptime %lld.%03lds\n", (long long)ts.tv_sec, ts.tv_nsec / 1000000);
  return EXIT_SUCCESS;
}
