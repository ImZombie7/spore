#pragma once

#include <stdbool.h>
#include <stdint.h>

bool virtio_net_init(uint64_t hhdm_offset);
bool virtio_net_smoke_tx(void);
