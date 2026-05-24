#include <spore.h>
#include <stdio.h>
#include <stdlib.h>

struct proc_row {
  unsigned pid;
  unsigned ppid;
  char state[16];
  char wait[16];
  unsigned long long rss_pages;
  unsigned long long cpu_ticks;
  unsigned long long age_ticks;
  unsigned long long budget_remaining;
  unsigned long long budget_max;
  char name[32];
  char exec_path[128];
  char cwd[64];
  char cmdline[160];
};

static char state_letter(const char *state) {
  if (streq(state, "running")) { return 'R'; }
  if (streq(state, "blocked")) { return 'S'; }
  if (streq(state, "zombie")) { return 'Z'; }
  return '?';
}

static void fmt_mem(unsigned long long pages, char *out, size_t cap) {
  unsigned long long kib = pages * 4ull;
  if (kib < 1024) {
    snprintf(out, cap, "%lluK", kib);
    return;
  }
  unsigned long long tenths = (kib * 10ull + 512ull) / 1024ull;
  if (tenths < 100 || (tenths % 10ull) != 0) {
    snprintf(out, cap, "%llu.%lluM", tenths / 10ull, tenths % 10ull);
    return;
  }
  snprintf(out, cap, "%lluM", tenths / 10ull);
}

int main(void) {
  FILE *f = fopen("/proc/procinfo", "r");
  if (f == NULL) {
    perror("ps");
    return EXIT_FAILURE;
  }
  char header[160];
  (void)fgets(header, sizeof(header), f);
  puts("PID  PPID  S  WAIT      MEM  CPU  TIME  CWD      CMD");
  struct proc_row p;
  while (fscanf(f, "%u %u %15s %15s %llu %llu %llu %llu %llu %31s %127s %63s %159[^\n]\n", &p.pid, &p.ppid, p.state,
                p.wait, &p.rss_pages, &p.cpu_ticks, &p.age_ticks, &p.budget_remaining, &p.budget_max, p.name,
                p.exec_path, p.cwd, p.cmdline) == 13) {
    char mem[16];
    fmt_mem(p.rss_pages, mem, sizeof(mem));
    printf("%3u  %4u  %c  %-6s  %6s  %3llu  %4llu  %-7s  %s\n", p.pid, p.ppid, state_letter(p.state), p.wait, mem,
           p.cpu_ticks, p.age_ticks, p.cwd, p.cmdline);
  }
  fclose(f);
  return EXIT_SUCCESS;
}
