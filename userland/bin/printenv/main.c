#include <stdio.h>
#include <stdlib.h>

extern char **environ;

int main(int argc, char **argv) {
  if (argc == 1) {
    for (char **env = environ; env != NULL && *env != NULL; ++env) {
      puts(*env);
    }
    return EXIT_SUCCESS;
  }
  int rc = EXIT_SUCCESS;
  for (int i = 1; i < argc; ++i) {
    const char *value = getenv(argv[i]);
    if (value == NULL) {
      rc = EXIT_FAILURE;
    } else {
      puts(value);
    }
  }
  return rc;
}
