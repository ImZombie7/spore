#pragma once

#include <limine.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { PAGE_SIZE = 4096 };

void pmm_init(uint64_t hhdm_offset, const struct limine_memmap_response *memmap);
uint64_t pmm_alloc_page(void);
uint64_t pmm_alloc_zero_page(void);
void pmm_free_page(uint64_t pa);
uint64_t pmm_total_pages(void);
uint64_t pmm_free_pages(void);

