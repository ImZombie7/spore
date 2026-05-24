#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    fputs("usage: dirname PATH\n", stderr);
    return EXIT_FAILURE;
  }
  char path[256];
  snprintf(path, sizeof(path), "%s", argv[1]);
  size_t len = strlen(path);
  while (len > 1 && path[len - 1] == '/') {
    path[--len] = '\0';
  }
  char *slash = strrchr(path, '/');
  if (slash == NULL) {
    puts(".");
  } else if (slash == path) {
    puts("/");
  } else {
    *slash = '\0';
    puts(path);
  }
  return EXIT_SUCCESS;
}
