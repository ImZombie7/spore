#include "util.h"

#include <stdio.h>

int main(void) {
  fputs("\033[H\033[2J\033[3J", stdout);
  return SPORE_OK;
}
