#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { FILE_CAP = 8 };

int main(int argc, char **argv) {
  int append = 0;
  int first = 1;
  if (argc > 1 && strcmp(argv[1], "-a") == 0) {
    append = 1;
    first = 2;
  }
  FILE *files[FILE_CAP];
  int count = 0;
  for (int i = first; i < argc && count < FILE_CAP; ++i) {
    files[count] = fopen(argv[i], append ? "a" : "w");
    if (files[count] == NULL) {
      perror(argv[i]);
      return EXIT_FAILURE;
    }
    ++count;
  }
  char buf[512];
  while (fgets(buf, sizeof(buf), stdin) != NULL) {
    fputs(buf, stdout);
    for (int i = 0; i < count; ++i) {
      fputs(buf, files[i]);
    }
  }
  for (int i = 0; i < count; ++i) {
    fclose(files[i]);
  }
  return ferror(stdin) ? EXIT_FAILURE : EXIT_SUCCESS;
}
