#include <spore.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

struct rule {
  char target[64];
  char recipe[256];
};

static char *trim(char *s) {
  while (isspace((unsigned char)*s)) {
    ++s;
  }
  char *end = s + strlen(s);
  while (end > s && isspace((unsigned char)end[-1])) {
    *--end = '\0';
  }
  return s;
}

static int split_words(char *line, char **argv, int cap) {
  int argc = 0;
  char *save = NULL;
  for (char *tok = strtok_r(line, " \t", &save); tok != NULL && argc + 1 < cap; tok = strtok_r(NULL, " \t", &save)) {
    argv[argc++] = tok;
  }
  argv[argc] = NULL;
  return argc;
}

static int run_recipe(const char *recipe, bool print_only, bool silent) {
  char line[256];
  snprintf(line, sizeof(line), "%s", recipe);
  char *argv[32];
  if (split_words(line, argv, 32) == 0) { return EXIT_SUCCESS; }
  if (!silent || print_only) { puts(recipe); }
  if (print_only) { return EXIT_SUCCESS; }
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return EXIT_FAILURE;
  }
  if (pid == 0) {
    execvp(argv[0], argv);
    perror(argv[0]);
    _exit(127);
  }
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    perror("waitpid");
    return EXIT_FAILURE;
  }
  return WIFEXITED(status) ? WEXITSTATUS(status) : 128;
}

int main(int argc, char **argv) {
  const char *mkfile = "mkfile";
  bool print_only = false;
  bool silent = false;
  const char *target = NULL;
  for (int i = 1; i < argc; ++i) {
    if (streq(argv[i], "-f") && i + 1 < argc) {
      mkfile = argv[++i];
    } else if (streq(argv[i], "-n")) {
      print_only = true;
    } else if (streq(argv[i], "-s")) {
      silent = true;
    } else if (argv[i][0] == '-') {
      return usage("mk", "[-n] [-s] [-f MKFILE] [TARGET]");
    } else {
      target = argv[i];
    }
  }

  FILE *f = fopen(mkfile, "r");
  if (f == NULL) {
    perror(mkfile);
    return EXIT_FAILURE;
  }
  struct rule rules[32];
  size_t rule_count = 0;
  char line[320];
  while (fgets(line, sizeof(line), f) != NULL) {
    if (line[0] == '#' || line[0] == '\n') { continue; }
    if (line[0] == '\t') {
      if (rule_count > 0 && rules[rule_count - 1].recipe[0] == '\0') {
        snprintf(rules[rule_count - 1].recipe, sizeof(rules[rule_count - 1].recipe), "%s", trim(line + 1));
      }
      continue;
    }
    char *colon = strchr(line, ':');
    if (colon == NULL || rule_count >= sizeof(rules) / sizeof(rules[0])) { continue; }
    *colon = '\0';
    snprintf(rules[rule_count].target, sizeof(rules[rule_count].target), "%s", trim(line));
    rules[rule_count].recipe[0] = '\0';
    ++rule_count;
  }
  fclose(f);

  if (rule_count == 0) {
    fputs("mk: no targets\n", stderr);
    return EXIT_FAILURE;
  }
  if (target == NULL) { target = rules[0].target; }
  for (size_t i = 0; i < rule_count; ++i) {
    if (streq(rules[i].target, target)) { return run_recipe(rules[i].recipe, print_only, silent); }
  }
  fprintf(stderr, "mk: no rule to make %s\n", target);
  return EXIT_FAILURE;
}
