#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct subst {
  char old[128];
  char new[128];
  int global;
};

static int parse_subst(const char *expr, struct subst *out) {
  if (expr[0] != 's' || expr[1] == '\0') { return 0; }
  char sep = expr[1];
  const char *a = expr + 2;
  const char *b = strchr(a, sep);
  if (b == NULL) { return 0; }
  const char *c = strchr(b + 1, sep);
  if (c == NULL) { return 0; }
  size_t old_len = (size_t)(b - a);
  size_t new_len = (size_t)(c - (b + 1));
  if (old_len == 0 || old_len >= sizeof(out->old) || new_len >= sizeof(out->new)) { return 0; }
  memcpy(out->old, a, old_len);
  out->old[old_len] = '\0';
  memcpy(out->new, b + 1, new_len);
  out->new[new_len] = '\0';
  out->global = c[1] == 'g';
  return c[1] == '\0' || (c[1] == 'g' && c[2] == '\0');
}

static void substitute_line(const char *line, const struct subst *s) {
  const char *p = line;
  size_t old_len = strlen(s->old);
  int changed = 0;
  while (*p != '\0') {
    const char *hit = strstr(p, s->old);
    if (hit == NULL || (changed && !s->global)) {
      fputs(p, stdout);
      return;
    }
    fwrite(p, 1, (size_t)(hit - p), stdout);
    fputs(s->new, stdout);
    p = hit + old_len;
    changed = 1;
  }
}

static int run(FILE *f, const struct subst *s) {
  char line[1024];
  while (fgets(line, sizeof(line), f) != NULL) {
    substitute_line(line, s);
  }
  return ferror(f) ? EXIT_FAILURE : EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  if (argc < 2) { return usage("sed-lite", "'s/OLD/NEW/[g]' [FILE...]"); }
  struct subst s;
  if (!parse_subst(argv[1], &s)) { return usage("sed-lite", "'s/OLD/NEW/[g]' [FILE...]"); }
  if (argc == 2) { return run(stdin, &s); }
  int rc = EXIT_SUCCESS;
  for (int i = 2; i < argc; ++i) {
    FILE *f = fopen(argv[i], "r");
    if (f == NULL) {
      perror(argv[i]);
      rc = EXIT_FAILURE;
      continue;
    }
    if (run(f, &s) != EXIT_SUCCESS) { rc = EXIT_FAILURE; }
    fclose(f);
  }
  return rc;
}
