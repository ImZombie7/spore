#include <spore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static uint32_t table[256];

static void init_table(void) {
  for (uint32_t i = 0; i < 256; ++i) {
    uint32_t c = i << 24;
    for (int j = 0; j < 8; ++j) {
      c = (c & 0x80000000u) != 0 ? (c << 1) ^ 0x04c11db7u : c << 1;
    }
    table[i] = c;
  }
}

static int cksum_stream(FILE *f, const char *name) {
  uint32_t crc = 0;
  unsigned long long len = 0;
  unsigned char buf[1024];
  for (;;) {
    size_t n = fread(buf, 1, sizeof(buf), f);
    for (size_t i = 0; i < n; ++i) {
      crc = (crc << 8) ^ table[((crc >> 24) ^ buf[i]) & 0xffu];
    }
    len += n;
    if (n < sizeof(buf)) { break; }
  }
  if (ferror(f)) { return EXIT_FAILURE; }
  unsigned long long l = len;
  while (l != 0) {
    crc = (crc << 8) ^ table[((crc >> 24) ^ (l & 0xffu)) & 0xffu];
    l >>= 8;
  }
  crc = ~crc;
  printf("%u %llu", crc, len);
  if (name != NULL) { printf(" %s", name); }
  putchar('\n');
  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  init_table();
  if (argc == 1) { return cksum_stream(stdin, NULL); }
  int rc = EXIT_SUCCESS;
  for (int i = 1; i < argc; ++i) {
    FILE *f = fopen(argv[i], "rb");
    if (f == NULL) {
      perror(argv[i]);
      rc = EXIT_FAILURE;
      continue;
    }
    if (cksum_stream(f, argv[i]) != EXIT_SUCCESS) { rc = EXIT_FAILURE; }
    fclose(f);
  }
  return rc;
}
