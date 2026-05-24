#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
  NET_IP_ICMP = 1,
  NET_IP_UDP = 17,
};

uint32_t net_ipv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
void net_init(void);
void net_poll(void);
void net_receive_ethernet(const uint8_t *frame, size_t len);
bool net_udp_send(uint16_t src_port, uint32_t dst_ip, uint16_t dst_port, const void *payload, size_t len);
bool net_icmp_send_echo(uint32_t dst_ip, const void *payload, size_t len);
