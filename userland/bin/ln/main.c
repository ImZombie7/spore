#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if (argc == 2 && streq(argv[1], "--help")) { return usage("ln", "TARGET LINK_NAME"); }
  if (argc > 1 && streq(argv[1], "-s")) {
    fputs("ln: symbolic links are not supported yet\n", stderr);
    return EXIT_FAILURE;
  }
  if (argc != 3) { return usage("ln", "TARGET LINK_NAME"); }
  if (link(argv[1], argv[2]) != 0) {
    perror("ln");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
