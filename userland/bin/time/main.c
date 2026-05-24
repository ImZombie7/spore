#include <spore.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static double seconds_since(const struct timespec *start, const struct timespec *end) {
  return (double)(end->tv_sec - start->tv_sec) + (double)(end->tv_nsec - start->tv_nsec) / 1000000000.0;
}

int main(int argc, char **argv) {
  if (argc < 2) { return usage("time", "COMMAND [ARG]..."); }
  struct timespec start;
  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return EXIT_FAILURE;
  }
  if (pid == 0) {
    execvp(argv[1], &argv[1]);
    perror(argv[1]);
    _exit(127);
  }
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    perror("waitpid");
    return EXIT_FAILURE;
  }
  clock_gettime(CLOCK_MONOTONIC, &end);
  fprintf(stderr, "real %.3fs\n", seconds_since(&start, &end));
  return WIFEXITED(status) ? WEXITSTATUS(status) : 128;
}
