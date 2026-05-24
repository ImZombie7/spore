#include <spore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int decode_char(int c) {
  if (c >= 'A' && c <= 'Z') { return c - 'A'; }
  if (c >= 'a' && c <= 'z') { return c - 'a' + 26; }
  if (c >= '0' && c <= '9') { return c - '0' + 52; }
  if (c == '+') { return 62; }
  if (c == '/') { return 63; }
  if (c == '=') { return -2; }
  return -1;
}

static int encode(FILE *in, FILE *out) {
  unsigned char b[3];
  for (;;) {
    size_t n = fread(b, 1, sizeof(b), in);
    if (n == 0) { break; }
    unsigned v = ((unsigned)b[0] << 16) | ((n > 1 ? b[1] : 0u) << 8) | (n > 2 ? b[2] : 0u);
    fputc(alphabet[(v >> 18) & 63u], out);
    fputc(alphabet[(v >> 12) & 63u], out);
    fputc(n > 1 ? alphabet[(v >> 6) & 63u] : '=', out);
    fputc(n > 2 ? alphabet[v & 63u] : '=', out);
  }
  fputc('\n', out);
  return ferror(in) || ferror(out) ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int decode(FILE *in, FILE *out) {
  int q[4];
  int qn = 0;
  for (;;) {
    int c = fgetc(in);
    if (c == EOF) { break; }
    int v = decode_char(c);
    if (v < 0 && v != -2) { continue; }
    q[qn++] = v;
    if (qn != 4) { continue; }
    unsigned n = ((q[0] & 63u) << 18) | ((q[1] & 63u) << 12) | ((q[2] < 0 ? 0u : (unsigned)q[2]) << 6) |
                 (q[3] < 0 ? 0u : (unsigned)q[3]);
    fputc((n >> 16) & 0xffu, out);
    if (q[2] >= 0) { fputc((n >> 8) & 0xffu, out); }
    if (q[3] >= 0) { fputc(n & 0xffu, out); }
    qn = 0;
  }
  return ferror(in) || ferror(out) ? EXIT_FAILURE : EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  int dec = 0;
  const char *output = NULL;
  const char *input = NULL;
  for (int i = 1; i < argc; ++i) {
    if (streq(argv[i], "-d")) {
      dec = 1;
    } else if (streq(argv[i], "-o") && i + 1 < argc) {
      output = argv[++i];
    } else if (input == NULL) {
      input = argv[i];
    } else {
      return usage("base64", "[-d] [-o OUT] [FILE]");
    }
  }
  FILE *in = input == NULL ? stdin : fopen(input, "rb");
  if (in == NULL) {
    perror(input);
    return EXIT_FAILURE;
  }
  FILE *out = output == NULL ? stdout : fopen(output, "wb");
  if (out == NULL) {
    perror(output);
    if (in != stdin) { fclose(in); }
    return EXIT_FAILURE;
  }
  int rc = dec ? decode(in, out) : encode(in, out);
  if (in != stdin) { fclose(in); }
  if (out != stdout) { fclose(out); }
  return rc;
}
