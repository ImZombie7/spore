#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void expand_set(const char *in, unsigned char *out, size_t *len) {
  *len = 0;
  for (size_t i = 0; in[i] != '\0' && *len < 256; ++i) {
    if (in[i + 1] == '-' && in[i + 2] != '\0' && (unsigned char)in[i] <= (unsigned char)in[i + 2]) {
      for (unsigned char c = (unsigned char)in[i]; c <= (unsigned char)in[i + 2] && *len < 256; ++c) {
        out[(*len)++] = c;
      }
      i += 2;
    } else {
      out[(*len)++] = (unsigned char)in[i];
    }
  }
}

int main(int argc, char **argv) {
  int delete = 0;
  int arg = 1;
  if (argc > 1 && streq(argv[1], "-d")) {
    delete = 1;
    arg = 2;
  }
  if ((!delete && argc != arg + 2) || (delete && argc != arg + 1)) { return usage("tr", "[-d] SET1 [SET2]"); }

  unsigned char set1[256];
  unsigned char set2[256];
  size_t n1 = 0;
  size_t n2 = 0;
  expand_set(argv[arg], set1, &n1);
  if (!delete) { expand_set(argv[arg + 1], set2, &n2); }
  int map[256];
  for (int i = 0; i < 256; ++i) {
    map[i] = i;
  }
  for (size_t i = 0; i < n1; ++i) {
    map[set1[i]] = delete ? -1 : set2[i < n2 ? i : n2 - 1];
  }
  for (;;) {
    int c = getchar();
    if (c == EOF) { break; }
    if (map[(unsigned char)c] >= 0) { putchar(map[(unsigned char)c]); }
  }
  return ferror(stdin) || ferror(stdout) ? EXIT_FAILURE : EXIT_SUCCESS;
}
