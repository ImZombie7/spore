#include <spore.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc != 3) { return usage("mv", "SOURCE DEST"); }
  if (rename(argv[1], argv[2]) != 0) {
    perror("mv");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
