#include <spore.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif

#ifndef S_IFIFO
#define S_IFIFO 0010000
#endif

static long k_mknodat(int dirfd, const char *path, mode_t mode) {
  register long x0 __asm__("x0") = dirfd;
  register long x1 __asm__("x1") = (long)path;
  register long x2 __asm__("x2") = (long)mode;
  register long x3 __asm__("x3") = 0;
  register long x8 __asm__("x8") = 33;
  __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x8) : "memory");
  if (x0 < 0 && x0 >= -4095) {
    errno = (int)-x0;
    return -1;
  }
  return x0;
}

int main(int argc, char **argv) {
  if (argc < 2) { return usage("mkfifo", "FILE..."); }
  int rc = EXIT_SUCCESS;
  for (int i = 1; i < argc; ++i) {
    if (k_mknodat(AT_FDCWD, argv[i], S_IFIFO | 0666) != 0) {
      perror(argv[i]);
      rc = EXIT_FAILURE;
    }
  }
  return rc;
}
