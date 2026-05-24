#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int grep_stream(FILE *f, const char *needle, const char *name, int prefix_name, int number) {
  char line[1024];
  unsigned long lineno = 0;
  int matched = 0;
  while (fgets(line, sizeof(line), f) != NULL) {
    ++lineno;
    if (strstr(line, needle) == NULL) { continue; }
    if (prefix_name) { printf("%s:", name); }
    if (number) { printf("%lu:", lineno); }
    fputs(line, stdout);
    if (strchr(line, '\n') == NULL) { putchar('\n'); }
    matched = 1;
  }
  if (ferror(f)) { return 2; }
  return matched ? 0 : 1;
}

int main(int argc, char **argv) {
  int number = 0;
  int first = 1;
  if (argc > 1 && streq(argv[1], "-n")) {
    number = 1;
    first = 2;
  }
  if (argc <= first) { return usage("grep", "[-n] PATTERN [FILE...]"); }
  const char *needle = argv[first++];
  if (first == argc) { return grep_stream(stdin, needle, NULL, 0, number); }
  int rc = 1;
  int prefix_name = argc - first > 1;
  for (int i = first; i < argc; ++i) {
    FILE *f = fopen(argv[i], "r");
    if (f == NULL) {
      perror(argv[i]);
      rc = 2;
      continue;
    }
    int got = grep_stream(f, needle, argv[i], prefix_name, number);
    if (got == 0) {
      rc = 0;
    } else if (got == 2 && rc != 0) {
      rc = 2;
    }
    fclose(f);
  }
  return rc;
}
