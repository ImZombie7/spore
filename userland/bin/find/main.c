#include <dirent.h>
#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int walk(const char *path) {
  puts(path);
  struct stat st;
  if (stat(path, &st) != 0) {
    perror(path);
    return EXIT_FAILURE;
  }
  if (!S_ISDIR(st.st_mode)) { return EXIT_SUCCESS; }

  DIR *dir = opendir(path);
  if (dir == NULL) {
    perror(path);
    return EXIT_FAILURE;
  }
  int rc = EXIT_SUCCESS;
  struct dirent *ent;
  while ((ent = readdir(dir)) != NULL) {
    if (streq(ent->d_name, ".") || streq(ent->d_name, "..")) { continue; }
    char child[512];
    int n = snprintf(child, sizeof(child), "%s%s%s", path, streq(path, "/") ? "" : "/", ent->d_name);
    if (n < 0 || (size_t)n >= sizeof(child)) {
      eprintf("find: path too long: %s/%s\n", path, ent->d_name);
      rc = EXIT_FAILURE;
      continue;
    }
    if (walk(child) != EXIT_SUCCESS) { rc = EXIT_FAILURE; }
  }
  closedir(dir);
  return rc;
}

int main(int argc, char **argv) {
  if (argc == 1) { return walk("."); }
  int rc = EXIT_SUCCESS;
  for (int i = 1; i < argc; ++i) {
    if (streq(argv[i], "--help")) { return usage("find", "[PATH...]"); }
    if (walk(argv[i]) != EXIT_SUCCESS) { rc = EXIT_FAILURE; }
  }
  return rc;
}
