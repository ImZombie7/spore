#include <spore.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  bool user = false;
  bool group = false;
  bool groups = false;
  bool name = false;
  for (int i = 1; i < argc; ++i) {
    if (streq(argv[i], "-u")) {
      user = true;
    } else if (streq(argv[i], "-g")) {
      group = true;
    } else if (streq(argv[i], "-G")) {
      groups = true;
    } else if (streq(argv[i], "-n")) {
      name = true;
    } else {
      return usage("id", "[-u|-g|-G] [-n]");
    }
  }
  if (name) {
    puts(user ? "root" : "root");
  } else if (user) {
    printf("%u\n", (unsigned)getuid());
  } else if (group) {
    printf("%u\n", (unsigned)getgid());
  } else if (groups) {
    printf("%u\n", (unsigned)getgid());
  } else {
    printf("uid=%u(root) gid=%u(root) groups=%u(root)\n", (unsigned)getuid(), (unsigned)getgid(), (unsigned)getgid());
  }
  return EXIT_SUCCESS;
}
