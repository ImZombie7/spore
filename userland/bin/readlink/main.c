#include <spore.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int canonicalize(const char *path, char *out, size_t cap) {
  char base[PATH_MAX];
  if (path[0] == '/') {
    snprintf(base, sizeof(base), "%s", path);
  } else {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) { return -1; }
    snprintf(base, sizeof(base), streq(cwd, "/") ? "/%s" : "%s/%s", cwd, path);
  }

  char comps[32][64];
  size_t count = 0;
  char *save = NULL;
  for (char *part = strtok_r(base, "/", &save); part != NULL; part = strtok_r(NULL, "/", &save)) {
    if (streq(part, ".")) { continue; }
    if (streq(part, "..")) {
      if (count > 0) { --count; }
      continue;
    }
    if (count >= 32 || strlen(part) >= sizeof(comps[0])) { return -1; }
    strcpy(comps[count++], part);
  }

  size_t pos = 0;
  out[pos++] = '/';
  for (size_t i = 0; i < count; ++i) {
    size_t len = strlen(comps[i]);
    if (pos + len + 1 >= cap) { return -1; }
    memcpy(out + pos, comps[i], len);
    pos += len;
    if (i + 1 < count) { out[pos++] = '/'; }
  }
  out[pos] = '\0';
  return 0;
}

int main(int argc, char **argv) {
  bool follow = false;
  int arg = 1;
  if (argc > 1 && streq(argv[1], "-f")) {
    follow = true;
    arg = 2;
  }
  if (argc != arg + 1) { return usage("readlink", "[-f] FILE"); }
  if (follow) {
    char path[PATH_MAX];
    if (canonicalize(argv[arg], path, sizeof(path)) != 0) {
      perror(argv[arg]);
      return EXIT_FAILURE;
    }
    puts(path);
    return EXIT_SUCCESS;
  }
  char buf[PATH_MAX];
  ssize_t n = readlink(argv[arg], buf, sizeof(buf) - 1);
  if (n < 0) {
    perror(argv[arg]);
    return EXIT_FAILURE;
  }
  buf[n] = '\0';
  puts(buf);
  return EXIT_SUCCESS;
}
