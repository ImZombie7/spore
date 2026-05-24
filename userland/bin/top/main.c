#include <fcntl.h>
#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

enum {
  MAX_PROCS = 32,
};

static struct termios saved_termios;
static int have_saved_termios;

static const char *state_name(uint32_t state) {
  switch (state) {
  case 1:
    return "running";
  case 2:
    return "blocked";
  case 3:
    return "zombie";
  default:
    return "?";
  }
}

static void restore_terminal(void) {
  static const char leave_alt[] = "\033[?25h\033[?1049l";
  (void)write(STDOUT_FILENO, leave_alt, sizeof(leave_alt) - 1);
  if (have_saved_termios) { (void)tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios); }
}

static void setup_terminal(void) {
  if (tcgetattr(STDIN_FILENO, &saved_termios) == 0) {
    struct termios raw = saved_termios;
    have_saved_termios = 1;
    raw.c_lflag &= ~(unsigned)(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    (void)tcsetattr(STDIN_FILENO, TCSANOW, &raw);
  }
  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  if (flags >= 0) { (void)fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK); }
  static const char enter_alt[] = "\033[?1049h\033[?25l\033[2J\033[H";
  (void)write(STDOUT_FILENO, enter_alt, sizeof(enter_alt) - 1);
  atexit(restore_terminal);
}

static unsigned screen_rows(void) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row >= 8) { return ws.ws_row; }
  return 24;
}

static void sleep_short(void) {
  struct timespec ts = {.tv_sec = 1, .tv_nsec = 0};
  (void)nanosleep(&ts, NULL);
}

static int read_key(void) {
  unsigned char c;
  ssize_t n = read(STDIN_FILENO, &c, 1);
  return n == 1 ? c : -1;
}

static int proc_cmp(const void *a, const void *b) {
  const struct proc_info *lhs = a;
  const struct proc_info *rhs = b;
  if (lhs->resident_pages < rhs->resident_pages) { return 1; }
  if (lhs->resident_pages > rhs->resident_pages) { return -1; }
  if (lhs->pid > rhs->pid) { return 1; }
  if (lhs->pid < rhs->pid) { return -1; }
  return 0;
}

static int draw(void) {
  struct proc_info infos[MAX_PROCS];
  long n = syscall(SYS_spore_procinfo, infos, sizeof(infos));
  if (n < 0) { return -1; }
  if (n > MAX_PROCS) { n = MAX_PROCS; }
  qsort(infos, (size_t)n, sizeof(infos[0]), proc_cmp);

  unsigned rows = screen_rows();
  printf("\033[H\033[2J");
  printf("\033[7m Spore top - %ld thread%s  (q to quit) \033[m\r\n", n, n == 1 ? "" : "s");
  printf("PID  TID  PPID  STATE    RSS-PG  BUDGET       CWD\r\n");
  unsigned used_rows = 2;
  for (long i = 0; i < n && used_rows + 1 < rows; ++i, ++used_rows) {
    char budget[32];
    if (infos[i].max_ticks == 0) {
      snprintf(budget, sizeof(budget), "unlimited");
    } else {
      snprintf(budget, sizeof(budget), "%llu/%llu", (unsigned long long)infos[i].remaining_ticks,
               (unsigned long long)infos[i].max_ticks);
    }
    printf("%3u  %3u  %4u  %-7s  %6llu  %-11s  %s\r\n", infos[i].pid, infos[i].tid, infos[i].ppid,
           state_name(infos[i].state), (unsigned long long)infos[i].resident_pages, budget, infos[i].cwd);
  }
  while (used_rows++ < rows - 1) {
    printf("\033[K\r\n");
  }
  printf("\033[7m ^L refresh   q quit \033[m");
  fflush(stdout);
  return 0;
}

int main(int argc, char **argv) {
  int once = argc > 1 && streq(argv[1], "-b");
  if (!once) { setup_terminal(); }
  for (;;) {
    if (draw() != 0) {
      perror("top");
      return EXIT_FAILURE;
    }
    if (once) { return EXIT_SUCCESS; }
    for (int i = 0; i < 10; ++i) {
      int key = read_key();
      if (key == 'q' || key == 'Q' || key == 3) { return EXIT_SUCCESS; }
      if (key == 12) { break; }
      struct timespec ts = {.tv_sec = 0, .tv_nsec = 100000000};
      (void)nanosleep(&ts, NULL);
    }
    sleep_short();
  }
}
