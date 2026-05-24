#include <spore.h>

#include <stdio.h>
#include <stdlib.h>

static const char *emit_escape(const char *p) {
  ++p;
  if (*p == 'n') {
    putchar('\n');
  } else if (*p == 't') {
    putchar('\t');
  } else if (*p == 'r') {
    putchar('\r');
  } else if (*p == '\\') {
    putchar('\\');
  } else if (*p != '\0') {
    putchar(*p);
  } else {
    return p - 1;
  }
  return p;
}

int main(int argc, char **argv) {
  if (argc < 2) { return usage("printf", "FORMAT [ARGUMENT]..."); }
  const char *fmt = argv[1];
  int arg = 2;
  for (const char *p = fmt; *p != '\0'; ++p) {
    if (*p == '\\') {
      p = emit_escape(p);
    } else if (*p == '%' && p[1] != '\0') {
      ++p;
      if (*p == '%') {
        putchar('%');
      } else if (*p == 's') {
        fputs(arg < argc ? argv[arg++] : "", stdout);
      } else if (*p == 'd' || *p == 'i') {
        printf("%d", arg < argc ? atoi(argv[arg++]) : 0);
      } else {
        putchar('%');
        putchar(*p);
      }
    } else {
      putchar(*p);
    }
  }
  return ferror(stdout) ? EXIT_FAILURE : EXIT_SUCCESS;
}
