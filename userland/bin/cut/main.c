#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_num(const char *s) {
  char *end = NULL;
  long v = strtol(s, &end, 10);
  return end != s && *end == '\0' && v > 0 ? (int)v : -1;
}

static void cut_chars(const char *line, int start, int end) {
  size_t len = strlen(line);
  if (len > 0 && line[len - 1] == '\n') { --len; }
  for (int i = start; i <= end && i <= (int)len; ++i) {
    putchar(line[i - 1]);
  }
  putchar('\n');
}

static void cut_field(const char *line, char delim, int field) {
  int cur = 1;
  const char *start = line;
  for (const char *p = line;; ++p) {
    if (*p == delim || *p == '\n' || *p == '\0') {
      if (cur == field) {
        fwrite(start, 1, (size_t)(p - start), stdout);
        putchar('\n');
        return;
      }
      if (*p == '\0' || *p == '\n') { return; }
      ++cur;
      start = p + 1;
    }
  }
}

static int run(FILE *f, int char_mode, int start, int end, char delim, int field) {
  char line[1024];
  while (fgets(line, sizeof(line), f) != NULL) {
    if (char_mode) {
      cut_chars(line, start, end);
    } else {
      cut_field(line, delim, field);
    }
  }
  return ferror(f) ? EXIT_FAILURE : EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  int char_mode = 0;
  int start = 1;
  int end = 1;
  char delim = '\t';
  int field = 1;
  int i = 1;
  for (; i < argc; ++i) {
    if (streq(argv[i], "-c") && i + 1 < argc) {
      char_mode = 1;
      char *dash = strchr(argv[++i], '-');
      if (dash != NULL) {
        *dash = '\0';
        start = parse_num(argv[i]);
        end = parse_num(dash + 1);
      } else {
        start = end = parse_num(argv[i]);
      }
    } else if (streq(argv[i], "-d") && i + 1 < argc) {
      delim = argv[++i][0];
    } else if (streq(argv[i], "-f") && i + 1 < argc) {
      field = parse_num(argv[++i]);
    } else {
      break;
    }
  }
  if (start <= 0 || end < start || field <= 0) { return usage("cut", "[-c N[-M] | -d C -f N] [FILE...]"); }
  if (i == argc) { return run(stdin, char_mode, start, end, delim, field); }
  int rc = EXIT_SUCCESS;
  for (; i < argc; ++i) {
    FILE *f = fopen(argv[i], "r");
    if (f == NULL) {
      perror(argv[i]);
      rc = EXIT_FAILURE;
      continue;
    }
    if (run(f, char_mode, start, end, delim, field) != EXIT_SUCCESS) { rc = EXIT_FAILURE; }
    fclose(f);
  }
  return rc;
}
