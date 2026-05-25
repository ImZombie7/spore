#include "msh.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char history[HISTORY_CAP][LINE_CAP];
static size_t history_len;
static char history_path[160];
static bool history_loaded;

static void chomp(char *s) {
  size_t len = strlen(s);
  while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
    s[--len] = '\0';
  }
}

static void history_path_init(void) {
  if (history_path[0] != '\0') { return; }
  const char *home = getenv("HOME");
  if (home == NULL || home[0] == '\0') { home = "/"; }
  if (streq(home, "/")) {
    snprintf(history_path, sizeof(history_path), "/.msh_history");
  } else {
    snprintf(history_path, sizeof(history_path), "%s/.msh_history", home);
  }
}

static void history_push_memory(const char *line) {
  if (line[0] == '\0') { return; }
  if (history_len > 0 && strcmp(history[history_len - 1], line) == 0) { return; }
  if (history_len == HISTORY_CAP) {
    for (size_t i = 1; i < history_len; ++i) {
      memcpy(history[i - 1], history[i], sizeof(history[0]));
    }
    --history_len;
  }
  snprintf(history[history_len++], sizeof(history[0]), "%s", line);
}

void sh_history_load(void) {
  if (history_loaded) { return; }
  history_loaded = true;
  history_path_init();

  FILE *f = fopen(history_path, "r");
  if (f == NULL) { return; }
  char line[LINE_CAP];
  while (fgets(line, sizeof(line), f) != NULL) {
    chomp(line);
    history_push_memory(line);
  }
  fclose(f);
}

void sh_history_add(const char *line) {
  if (line == NULL || line[0] == '\0') { return; }
  sh_history_load();
  if (history_len > 0 && strcmp(history[history_len - 1], line) == 0) { return; }
  history_push_memory(line);

  FILE *f = fopen(history_path, "a");
  if (f == NULL) { return; }
  fprintf(f, "%s\n", line);
  fclose(f);
}

size_t sh_history_count(void) {
  sh_history_load();
  return history_len;
}

const char *sh_history_get(size_t index) {
  sh_history_load();
  return index < history_len ? history[index] : "";
}
