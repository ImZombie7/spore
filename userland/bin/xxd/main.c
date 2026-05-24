#include <stdio.h>
#include <stdlib.h>

static int dump(FILE *f) {
  unsigned char buf[16];
  unsigned long off = 0;
  for (;;) {
    size_t n = fread(buf, 1, sizeof(buf), f);
    if (n == 0) { break; }
    printf("%08lx: ", off);
    for (size_t i = 0; i < 16; ++i) {
      if (i < n) {
        printf("%02x", buf[i]);
      } else {
        fputs("  ", stdout);
      }
      if ((i & 1) == 1) { putchar(' '); }
    }
    putchar(' ');
    for (size_t i = 0; i < n; ++i) {
      putchar(buf[i] >= 32 && buf[i] < 127 ? buf[i] : '.');
    }
    putchar('\n');
    off += (unsigned long)n;
  }
  return ferror(f) ? EXIT_FAILURE : EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  if (argc == 1) { return dump(stdin); }
  int rc = EXIT_SUCCESS;
  for (int i = 1; i < argc; ++i) {
    FILE *f = fopen(argv[i], "rb");
    if (f == NULL) {
      perror(argv[i]);
      rc = EXIT_FAILURE;
      continue;
    }
    if (dump(f) != EXIT_SUCCESS) { rc = EXIT_FAILURE; }
    fclose(f);
  }
  return rc;
}
