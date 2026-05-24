#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef IPPROTO_ICMP
#define IPPROTO_ICMP 1
#endif

static uint16_t be16(uint16_t x) {
  return (uint16_t)((x << 8) | (x >> 8));
}

static int parse_ipv4(const char *s, uint32_t *out) {
  unsigned a;
  unsigned b;
  unsigned c;
  unsigned d;
  char tail;
  if (sscanf(s, "%u.%u.%u.%u%c", &a, &b, &c, &d, &tail) != 4 || a > 255 || b > 255 || c > 255 || d > 255) { return -1; }
  *out = a | (b << 8) | (c << 16) | (d << 24);
  return 0;
}

int main(int argc, char **argv) {
  const char *target = argc > 1 ? argv[1] : "10.0.2.2";
  uint32_t ip;
  if (parse_ipv4(target, &ip) != 0) {
    fprintf(stderr, "ping: bad address: %s\n", target);
    return 1;
  }
  int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
  if (fd < 0) {
    perror("socket");
    return 1;
  }
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = be16(0);
  sa.sin_addr.s_addr = ip;
  const char payload[] = "spore-ping";
  if (sendto(fd, payload, sizeof(payload) - 1, 0, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
    perror("sendto");
    close(fd);
    return 1;
  }
  char buf[128];
  ssize_t n = recvfrom(fd, buf, sizeof(buf), 0, NULL, NULL);
  if (n < 0) {
    perror("recvfrom");
    close(fd);
    return 1;
  }
  printf("64 bytes from %s: icmp_seq=1\n", target);
  close(fd);
  return 0;
}
