#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

static const char *state_name(uint32_t state) {
  switch (state) {
  case 1:
    return "R";
  case 2:
    return "S";
  case 3:
    return "Z";
  default:
    return "?";
  }
}

int main(void) {
  struct proc_info infos[32];
  long n = syscall(SYS_spore_procinfo, infos, sizeof(infos));
  if (n < 0) {
    perror("ps");
    return EXIT_FAILURE;
  }
  if (n > (long)(sizeof(infos) / sizeof(infos[0]))) { n = (long)(sizeof(infos) / sizeof(infos[0])); }
  puts("PID  TID  PPID  S  RSSP  CWD");
  for (long i = 0; i < n; ++i) {
    printf("%3u  %3u  %4u  %s  %4llu  %s\n", infos[i].pid, infos[i].tid, infos[i].ppid, state_name(infos[i].state),
           (unsigned long long)infos[i].resident_pages, infos[i].cwd);
  }
  return EXIT_SUCCESS;
}
