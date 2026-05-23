#pragma once

#include <stdint.h>

void pl011_init(uint64_t hhdm_offset);
void pl011_putc(char c);

