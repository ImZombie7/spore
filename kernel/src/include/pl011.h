#pragma once

#include <stdbool.h>
#include <stdint.h>

void pl011_init(uint64_t hhdm_offset);
void pl011_putc(char c);
void pl011_enable_rx_irq(void);
bool pl011_handle_irq(void);
bool pl011_getc(char *out);
