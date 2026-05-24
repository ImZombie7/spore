#include "spore.h"
#include "spore_version.h"

#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>

int main(int argc, char **argv) {
  int all = 0;
  if (argc > 2) { return usage("uname", "[-a]"); }
  if (argc == 2) {
    if (!streq(argv[1], "-a")) { return usage("uname", "[-a]"); }
    all = 1;
  }
  struct utsname u;
  if (uname(&u) != 0) {
    perror("uname");
    return EXIT_FAILURE;
  }
  if (all) {
    printf("%s %s %s %s root:%s %s %s %s\n", u.sysname, u.nodename, u.release, u.version, SPORE_ROOT_TAG, u.machine,
           u.machine, u.domainname);
  } else {
    printf("%s\n", u.sysname);
  }
  return EXIT_SUCCESS;
}
