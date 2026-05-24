#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef SYS_uname
#define SYS_uname 160
#endif

struct spore_utsname {
  char sysname[65];
  char nodename[65];
  char release[65];
  char version[65];
  char machine[65];
  char domainname[65];
};

int main(void) {
  struct spore_utsname u;
  if (syscall(SYS_uname, &u) != 0) {
    perror("hostname");
    return EXIT_FAILURE;
  }
  puts(u.nodename);
  return EXIT_SUCCESS;
}
