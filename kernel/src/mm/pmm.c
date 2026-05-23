#include "mm/pmm.h"

#include "mem.h"

#define PMM_MAX_PHYS (2ull * 1024 * 1024 * 1024)
#define PMM_MAX_PAGES (PMM_MAX_PHYS / PAGE_SIZE)
#define BITS_PER_WORD 64
#define PMM_BITMAP_WORDS (PMM_MAX_PAGES / BITS_PER_WORD)

static uint64_t *hhdm_base;
static uint64_t bitmap[PMM_BITMAP_WORDS];
static uint64_t total_page_count;
static uint64_t free_page_count;

static uint64_t align_down(uint64_t value, uint64_t align) {
    return value & ~(align - 1);
}

static uint64_t align_up(uint64_t value, uint64_t align) {
    return (value + align - 1) & ~(align - 1);
}

static void set_used(uint64_t page) {
    uint64_t mask = 1ull << (page % BITS_PER_WORD);
    uint64_t *word = &bitmap[page / BITS_PER_WORD];
    if ((*word & mask) == 0) {
        *word |= mask;
        if (free_page_count > 0) {
            --free_page_count;
        }
    }
}

static void set_free(uint64_t page) {
    uint64_t mask = 1ull << (page % BITS_PER_WORD);
    uint64_t *word = &bitmap[page / BITS_PER_WORD];
    if ((*word & mask) != 0) {
        *word &= ~mask;
        ++free_page_count;
    }
}

static bool is_free(uint64_t page) {
    return (bitmap[page / BITS_PER_WORD] & (1ull << (page % BITS_PER_WORD))) == 0;
}

void pmm_init(uint64_t hhdm_offset, const struct limine_memmap_response *memmap) {
    hhdm_base = (uint64_t *)(uintptr_t)hhdm_offset;
    for (size_t i = 0; i < PMM_BITMAP_WORDS; ++i) {
        bitmap[i] = UINT64_MAX;
    }
    total_page_count = PMM_MAX_PAGES;
    free_page_count = 0;

    for (uint64_t i = 0; i < memmap->entry_count; ++i) {
        const struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        uint64_t start = align_up(entry->base, PAGE_SIZE);
        uint64_t end = align_down(entry->base + entry->length, PAGE_SIZE);
        if (end > PMM_MAX_PHYS) {
            end = PMM_MAX_PHYS;
        }
        for (uint64_t pa = start; pa < end; pa += PAGE_SIZE) {
            set_free(pa / PAGE_SIZE);
        }
    }

    for (uint64_t pa = 0; pa < 0x100000; pa += PAGE_SIZE) {
        set_used(pa / PAGE_SIZE);
    }
}

uint64_t pmm_alloc_page(void) {
    for (uint64_t page = 0x100000 / PAGE_SIZE; page < PMM_MAX_PAGES; ++page) {
        if (is_free(page)) {
            set_used(page);
            return page * PAGE_SIZE;
        }
    }
    return 0;
}

uint64_t pmm_alloc_zero_page(void) {
    uint64_t pa = pmm_alloc_page();
    if (pa != 0) {
        kmemset((void *)((uintptr_t)hhdm_base + pa), 0, PAGE_SIZE);
    }
    return pa;
}

void pmm_free_page(uint64_t pa) {
    if ((pa % PAGE_SIZE) == 0 && pa < PMM_MAX_PHYS) {
        set_free(pa / PAGE_SIZE);
    }
}

uint64_t pmm_total_pages(void) {
    return total_page_count;
}

uint64_t pmm_free_pages(void) {
    return free_page_count;
}
