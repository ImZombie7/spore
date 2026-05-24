#include <spore.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct sha1 {
  uint64_t len;
  uint32_t h[5];
  uint8_t buf[64];
  size_t used;
};

static uint32_t rol(uint32_t x, unsigned n) {
  return (x << n) | (x >> (32u - n));
}

static void sha1_block(struct sha1 *s, const uint8_t block[64]) {
  uint32_t w[80];
  for (int i = 0; i < 16; ++i) {
    w[i] = ((uint32_t)block[i * 4] << 24) | ((uint32_t)block[i * 4 + 1] << 16) | ((uint32_t)block[i * 4 + 2] << 8) |
           (uint32_t)block[i * 4 + 3];
  }
  for (int i = 16; i < 80; ++i) {
    w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
  }
  uint32_t a = s->h[0];
  uint32_t b = s->h[1];
  uint32_t c = s->h[2];
  uint32_t d = s->h[3];
  uint32_t e = s->h[4];
  for (int i = 0; i < 80; ++i) {
    uint32_t f;
    uint32_t k;
    if (i < 20) {
      f = (b & c) | (~b & d);
      k = 0x5a827999u;
    } else if (i < 40) {
      f = b ^ c ^ d;
      k = 0x6ed9eba1u;
    } else if (i < 60) {
      f = (b & c) | (b & d) | (c & d);
      k = 0x8f1bbcdcu;
    } else {
      f = b ^ c ^ d;
      k = 0xca62c1d6u;
    }
    uint32_t temp = rol(a, 5) + f + e + k + w[i];
    e = d;
    d = c;
    c = rol(b, 30);
    b = a;
    a = temp;
  }
  s->h[0] += a;
  s->h[1] += b;
  s->h[2] += c;
  s->h[3] += d;
  s->h[4] += e;
}

static void sha1_init(struct sha1 *s) {
  *s = (struct sha1){.h = {0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u, 0xc3d2e1f0u}};
}

static void sha1_update(struct sha1 *s, const uint8_t *data, size_t len) {
  s->len += len;
  while (len > 0) {
    size_t n = 64 - s->used;
    if (n > len) { n = len; }
    for (size_t i = 0; i < n; ++i) {
      s->buf[s->used + i] = data[i];
    }
    s->used += n;
    data += n;
    len -= n;
    if (s->used == 64) {
      sha1_block(s, s->buf);
      s->used = 0;
    }
  }
}

static void sha1_final(struct sha1 *s, uint8_t out[20]) {
  uint64_t bit_len = s->len * 8;
  uint8_t one = 0x80;
  sha1_update(s, &one, 1);
  uint8_t zero = 0;
  while (s->used != 56) {
    sha1_update(s, &zero, 1);
  }
  uint8_t lenbuf[8];
  for (int i = 0; i < 8; ++i) {
    lenbuf[7 - i] = (uint8_t)(bit_len >> (i * 8));
  }
  sha1_update(s, lenbuf, sizeof(lenbuf));
  for (int i = 0; i < 5; ++i) {
    out[i * 4] = (uint8_t)(s->h[i] >> 24);
    out[i * 4 + 1] = (uint8_t)(s->h[i] >> 16);
    out[i * 4 + 2] = (uint8_t)(s->h[i] >> 8);
    out[i * 4 + 3] = (uint8_t)s->h[i];
  }
}

static int sum_stream(FILE *f, const char *name) {
  struct sha1 s;
  sha1_init(&s);
  uint8_t buf[1024];
  for (;;) {
    size_t n = fread(buf, 1, sizeof(buf), f);
    if (n > 0) { sha1_update(&s, buf, n); }
    if (n < sizeof(buf)) { break; }
  }
  if (ferror(f)) { return EXIT_FAILURE; }
  uint8_t digest[20];
  sha1_final(&s, digest);
  for (size_t i = 0; i < sizeof(digest); ++i) {
    printf("%02x", digest[i]);
  }
  if (name != NULL) { printf("  %s", name); }
  putchar('\n');
  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  if (argc == 1) { return sum_stream(stdin, NULL); }
  int rc = EXIT_SUCCESS;
  for (int i = 1; i < argc; ++i) {
    FILE *f = fopen(argv[i], "rb");
    if (f == NULL) {
      perror(argv[i]);
      rc = EXIT_FAILURE;
      continue;
    }
    if (sum_stream(f, argv[i]) != EXIT_SUCCESS) { rc = EXIT_FAILURE; }
    fclose(f);
  }
  return rc;
}
