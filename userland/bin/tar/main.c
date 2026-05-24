#include <errno.h>
#include <fcntl.h>
#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct tar_header {
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char chksum[8];
  char typeflag;
  char linkname[100];
  char magic[6];
  char version[2];
  char uname[32];
  char gname[32];
  char devmajor[8];
  char devminor[8];
  char prefix[155];
  char pad[12];
};

static unsigned long parse_octal(const char *s, size_t n) {
  unsigned long out = 0;
  for (size_t i = 0; i < n && s[i] != '\0'; ++i) {
    if (s[i] >= '0' && s[i] <= '7') { out = out * 8 + (unsigned long)(s[i] - '0'); }
  }
  return out;
}

static void write_octal(char *dst, size_t n, unsigned long value) {
  snprintf(dst, n, "%0*lo", (int)n - 2, value);
}

static int zero_block(const unsigned char *buf) {
  for (size_t i = 0; i < 512; ++i) {
    if (buf[i] != 0) { return 0; }
  }
  return 1;
}

static void fill_checksum(struct tar_header *h) {
  memset(h->chksum, ' ', sizeof(h->chksum));
  unsigned sum = 0;
  const unsigned char *p = (const unsigned char *)h;
  for (size_t i = 0; i < sizeof(*h); ++i) {
    sum += p[i];
  }
  snprintf(h->chksum, sizeof(h->chksum), "%06o", sum);
  h->chksum[6] = '\0';
  h->chksum[7] = ' ';
}

static int add_file(FILE *tar, const char *path) {
  struct stat st;
  if (stat(path, &st) != 0) {
    perror(path);
    return EXIT_FAILURE;
  }
  if (!S_ISREG(st.st_mode)) {
    eprintf("tar: skipping non-regular file: %s\n", path);
    return EXIT_SUCCESS;
  }
  FILE *in = fopen(path, "rb");
  if (in == NULL) {
    perror(path);
    return EXIT_FAILURE;
  }
  struct tar_header h;
  memset(&h, 0, sizeof(h));
  snprintf(h.name, sizeof(h.name), "%s", path);
  write_octal(h.mode, sizeof(h.mode), (unsigned long)(st.st_mode & 0777));
  write_octal(h.uid, sizeof(h.uid), 0);
  write_octal(h.gid, sizeof(h.gid), 0);
  write_octal(h.size, sizeof(h.size), (unsigned long)st.st_size);
  write_octal(h.mtime, sizeof(h.mtime), 0);
  h.typeflag = '0';
  memcpy(h.magic, "ustar", 5);
  memcpy(h.version, "00", 2);
  fill_checksum(&h);
  fwrite(&h, 1, sizeof(h), tar);

  char buf[512];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
    fwrite(buf, 1, n, tar);
    if (n < sizeof(buf)) {
      memset(buf, 0, sizeof(buf) - n);
      fwrite(buf, 1, sizeof(buf) - n, tar);
    }
  }
  fclose(in);
  return ferror(tar) ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int create_archive(const char *archive, int argc, char **argv, int first) {
  FILE *tar = fopen(archive, "wb");
  if (tar == NULL) {
    perror(archive);
    return EXIT_FAILURE;
  }
  int rc = EXIT_SUCCESS;
  for (int i = first; i < argc; ++i) {
    if (add_file(tar, argv[i]) != EXIT_SUCCESS) { rc = EXIT_FAILURE; }
  }
  char zero[1024] = {0};
  fwrite(zero, 1, sizeof(zero), tar);
  fclose(tar);
  return rc;
}

static int skip_file(FILE *tar, unsigned long size) {
  unsigned long padded = (size + 511) & ~511ul;
  return fseek(tar, (long)padded, SEEK_CUR) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int list_archive(const char *archive) {
  FILE *tar = fopen(archive, "rb");
  if (tar == NULL) {
    perror(archive);
    return EXIT_FAILURE;
  }
  for (;;) {
    unsigned char block[512];
    if (fread(block, 1, sizeof(block), tar) != sizeof(block)) { break; }
    if (zero_block(block)) { break; }
    struct tar_header *h = (struct tar_header *)block;
    puts(h->name);
    if (skip_file(tar, parse_octal(h->size, sizeof(h->size))) != EXIT_SUCCESS) { break; }
  }
  fclose(tar);
  return EXIT_SUCCESS;
}

static int extract_archive(const char *archive) {
  FILE *tar = fopen(archive, "rb");
  if (tar == NULL) {
    perror(archive);
    return EXIT_FAILURE;
  }
  int rc = EXIT_SUCCESS;
  for (;;) {
    unsigned char block[512];
    if (fread(block, 1, sizeof(block), tar) != sizeof(block)) { break; }
    if (zero_block(block)) { break; }
    struct tar_header *h = (struct tar_header *)block;
    unsigned long size = parse_octal(h->size, sizeof(h->size));
    FILE *out = fopen(h->name, "wb");
    if (out == NULL) {
      perror(h->name);
      (void)skip_file(tar, size);
      rc = EXIT_FAILURE;
      continue;
    }
    unsigned long left = size;
    while (left > 0) {
      char data[512];
      if (fread(data, 1, sizeof(data), tar) != sizeof(data)) {
        rc = EXIT_FAILURE;
        break;
      }
      size_t n = left < sizeof(data) ? (size_t)left : sizeof(data);
      fwrite(data, 1, n, out);
      left -= n;
    }
    fclose(out);
  }
  fclose(tar);
  return rc;
}

int main(int argc, char **argv) {
  if (argc < 3) { return usage("tar", "-cf ARCHIVE FILE... | -tf ARCHIVE | -xf ARCHIVE"); }
  if (streq(argv[1], "-cf")) {
    if (argc < 4) { return usage("tar", "-cf ARCHIVE FILE..."); }
    return create_archive(argv[2], argc, argv, 3);
  }
  if (streq(argv[1], "-tf")) { return list_archive(argv[2]); }
  if (streq(argv[1], "-xf")) { return extract_archive(argv[2]); }
  return usage("tar", "-cf ARCHIVE FILE... | -tf ARCHIVE | -xf ARCHIVE");
}
