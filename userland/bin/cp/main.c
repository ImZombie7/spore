#include <spore.h>
#include <stdio.h>
#include <stdlib.h>

static int copy_one(const char *src, const char *dst) {
  FILE *in = fopen(src, "rb");
  if (in == NULL) {
    perror(src);
    return EXIT_FAILURE;
  }
  FILE *out = fopen(dst, "wb");
  if (out == NULL) {
    perror(dst);
    fclose(in);
    return EXIT_FAILURE;
  }
  char buf[512];
  int rc = EXIT_SUCCESS;
  for (;;) {
    size_t n = fread(buf, 1, sizeof(buf), in);
    if (n > 0 && fwrite(buf, 1, n, out) != n) {
      perror(dst);
      rc = EXIT_FAILURE;
      break;
    }
    if (n < sizeof(buf)) {
      if (ferror(in)) {
        perror(src);
        rc = EXIT_FAILURE;
      }
      break;
    }
  }
  fclose(out);
  fclose(in);
  return rc;
}

int main(int argc, char **argv) {
  if (argc != 3) { return usage("cp", "SOURCE DEST"); }
  return copy_one(argv[1], argv[2]);
}
