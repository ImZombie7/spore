#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    fputs("usage: basename PATH\n", stderr);
    return EXIT_FAILURE;
  }
  const char *path = argv[1];
  size_t len = strlen(path);
  while (len > 1 && path[len - 1] == '/') {
    --len;
  }
  const char *start = path + len;
  while (start > path && start[-1] != '/') {
    --start;
  }
  printf("%.*s\n", (int)(path + len - start), start);
  return EXIT_SUCCESS;
}
