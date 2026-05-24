#include <spore.h>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

struct opts {
  bool all;
  bool human;
  bool summary;
};

static void fmt_blocks(unsigned long long blocks, bool human, char *out, size_t cap) {
  if (!human) {
    snprintf(out, cap, "%llu", blocks);
    return;
  }
  unsigned long long bytes = blocks * 512ull;
  const char *units[] = {"B", "K", "M", "G"};
  unsigned unit = 0;
  unsigned long long value = bytes;
  while (value >= 1024 && unit + 1 < sizeof(units) / sizeof(units[0])) {
    value = (value + 512) / 1024;
    ++unit;
  }
  snprintf(out, cap, "%llu%s", value, units[unit]);
}

static unsigned long long node_blocks(const struct stat *st) {
  if (st->st_blocks > 0) { return (unsigned long long)st->st_blocks; }
  return ((unsigned long long)st->st_size + 511ull) / 512ull;
}

static int join_path(char *out, size_t cap, const char *dir, const char *name) {
  int n = streq(dir, "/") ? snprintf(out, cap, "/%s", name) : snprintf(out, cap, "%s/%s", dir, name);
  return n >= 0 && n < (int)cap ? 0 : -1;
}

static unsigned long long du_path(const char *path, const struct opts *opts, bool top, dev_t root_dev, int *rc) {
  struct stat st;
  if (lstat(path, &st) != 0) {
    perror(path);
    *rc = EXIT_FAILURE;
    return 0;
  }
  if (!top && st.st_dev != root_dev) { return 0; }
  unsigned long long total = node_blocks(&st);
  if (S_ISLNK(st.st_mode)) {
    if (opts->all) {
      char size[32];
      fmt_blocks(total, opts->human, size, sizeof(size));
      printf("%s\t%s\n", size, path);
    }
    return total;
  }
  if (S_ISDIR(st.st_mode)) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
      perror(path);
      *rc = EXIT_FAILURE;
    } else {
      struct dirent *ent;
      while ((ent = readdir(dir)) != NULL) {
        if (streq(ent->d_name, ".") || streq(ent->d_name, "..")) { continue; }
        char child[256];
        if (join_path(child, sizeof(child), path, ent->d_name) == 0) {
          total += du_path(child, opts, false, root_dev, rc);
        }
      }
      closedir(dir);
    }
    (void)top;
    if (!opts->summary) {
      char size[32];
      fmt_blocks(total, opts->human, size, sizeof(size));
      printf("%s\t%s\n", size, path);
    }
  } else if (opts->all) {
    char size[32];
    fmt_blocks(total, opts->human, size, sizeof(size));
    printf("%s\t%s\n", size, path);
  }
  return total;
}

int main(int argc, char **argv) {
  struct opts opts = {0};
  int first = 1;
  for (; first < argc && argv[first][0] == '-'; ++first) {
    if (streq(argv[first], "--help")) { return usage("du", "[-ahs] [PATH...]"); }
    for (const char *p = argv[first] + 1; *p != '\0'; ++p) {
      if (*p == 'a') {
        opts.all = true;
      } else if (*p == 'h') {
        opts.human = true;
      } else if (*p == 's') {
        opts.summary = true;
      } else {
        return usage("du", "[-ahs] [PATH...]");
      }
    }
  }
  int rc = EXIT_SUCCESS;
  if (first == argc) {
    struct stat st;
    if (lstat(".", &st) != 0) {
      perror(".");
      return EXIT_FAILURE;
    }
    (void)du_path(".", &opts, true, st.st_dev, &rc);
  } else {
    for (int i = first; i < argc; ++i) {
      struct stat st;
      if (lstat(argv[i], &st) != 0) {
        perror(argv[i]);
        rc = EXIT_FAILURE;
        continue;
      }
      unsigned long long total = du_path(argv[i], &opts, true, st.st_dev, &rc);
      if (opts.summary) {
        char size[32];
        fmt_blocks(total, opts.human, size, sizeof(size));
        printf("%s\t%s\n", size, argv[i]);
      }
    }
  }
  return rc;
}
