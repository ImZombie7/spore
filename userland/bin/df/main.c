#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(void) {
  struct fs_info info;
  if (syscall(SYS_spore_fsinfo, &info) != 0) {
    perror("df");
    return EXIT_FAILURE;
  }
  unsigned long long total = (info.block_count * info.block_size) / 1024;
  unsigned long long free = (info.free_blocks * info.block_size) / 1024;
  unsigned long long used = total > free ? total - free : 0;
  unsigned pct = total == 0 ? 0 : (unsigned)((used * 100 + total - 1) / total);
  puts("Filesystem  1K-blocks  Used  Available  Use%  Mounted on");
  printf("ext2-root   %9llu  %4llu  %9llu  %3u%%  /\n", total, used, free, pct);
  return EXIT_SUCCESS;
}
