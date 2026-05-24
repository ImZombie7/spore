#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

#define SYS_SPORE_SHUTDOWN 0x4006

int main(void) {
  (void)syscall(SYS_SPORE_SHUTDOWN);
  for (;;) {}
}
