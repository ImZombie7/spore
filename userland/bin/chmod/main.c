#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

static int parse_octal_mode(const char *s, mode_t *out) {
  mode_t mode = 0;
  if (*s == '\0') { return -1; }
  for (const char *p = s; *p != '\0'; ++p) {
    if (*p < '0' || *p > '7') { return -1; }
    mode = (mode_t)((mode << 3) | (mode_t)(*p - '0'));
    if (mode > 07777) { return -1; }
  }
  *out = mode;
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 3) { return usage("chmod", "MODE FILE..."); }
  mode_t mode = 0;
  if (parse_octal_mode(argv[1], &mode) != 0) { return usage("chmod", "MODE FILE..."); }

  int rc = EXIT_SUCCESS;
  for (int i = 2; i < argc; ++i) {
    if (chmod(argv[i], mode) != 0) {
      perror(argv[i]);
      rc = EXIT_FAILURE;
    }
  }
  return rc;
}
