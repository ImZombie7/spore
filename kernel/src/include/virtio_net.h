#pragma once

#include <stdbool.h>
#include <stdint.h>

bool virtio_net_init(uint64_t hhdm_offset);
bool virtio_net_smoke_tx(void);
bool virtio_net_send_frame(const void *frame, uint32_t len);
void virtio_net_poll(void);
