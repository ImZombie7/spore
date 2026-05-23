#include "pl011.h"

#include <stddef.h>
#include <stdint.h>

enum {
    PL011_PHYS = 0x09000000,
    PL011_DR = 0x00,
    PL011_FR = 0x18,
    PL011_FR_TXFF = 1u << 5,
};

static volatile uint32_t *uart_base;

static inline uint32_t mmio_read32(uint64_t offset) {
    return *(volatile uint32_t *)((uintptr_t)uart_base + offset);
}

static inline void mmio_write32(uint64_t offset, uint32_t value) {
    *(volatile uint32_t *)((uintptr_t)uart_base + offset) = value;
}

void pl011_init(uint64_t hhdm_offset) {
    uart_base = (volatile uint32_t *)(hhdm_offset + PL011_PHYS);
}

void pl011_putc(char c) {
    if (uart_base == NULL) {
        return;
    }
    if (c == '\n') {
        pl011_putc('\r');
    }
    while ((mmio_read32(PL011_FR) & PL011_FR_TXFF) != 0) {
        __asm__ volatile("yield");
    }
    mmio_write32(PL011_DR, (uint32_t)(uint8_t)c);
}

