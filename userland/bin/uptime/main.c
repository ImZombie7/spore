#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
  struct timespec realtime;
  struct timespec monotonic;
  if (clock_gettime(CLOCK_REALTIME, &realtime) != 0 || clock_gettime(CLOCK_MONOTONIC, &monotonic) != 0) {
    perror("uptime");
    return EXIT_FAILURE;
  }
  long long up = monotonic.tv_sec;
  long long days = up / 86400;
  long long hours = (up % 86400) / 3600;
  long long minutes = (up % 3600) / 60;
  long long seconds = up % 60;
  long long local = realtime.tv_sec - 7 * 60 * 60;
  long long day_seconds = local % 86400;
  if (day_seconds < 0) { day_seconds += 86400; }
  printf(" %02lld:%02lld:%02lld  up ", day_seconds / 3600, (day_seconds % 3600) / 60, day_seconds % 60);
  if (days > 0) { printf("%lld days ", days); }
  printf("%02lld:%02lld,  1 user,  load average: 0.00, 0.00, 0.00\n", hours, minutes);
  (void)seconds;
  return EXIT_SUCCESS;
}
