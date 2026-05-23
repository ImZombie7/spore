#include "mm/pmm.h"

#include <assert.h>
#include <stdint.h>

int main(void) {
    struct limine_memmap_entry usable = {
        .base = 0x100000,
        .length = 16 * PAGE_SIZE,
        .type = LIMINE_MEMMAP_USABLE,
    };
    struct limine_memmap_entry *entries[] = {&usable};
    struct limine_memmap_response memmap = {
        .revision = 0,
        .entry_count = 1,
        .entries = entries,
    };

    pmm_init(0, &memmap);
    assert(pmm_free_pages() == 16);

    uint64_t first = pmm_alloc_page();
    uint64_t second = pmm_alloc_page();
    assert(first == 0x100000);
    assert(second == 0x101000);
    assert(pmm_refcount(first) == 1);
    assert(pmm_refcount(second) == 1);
    assert(pmm_is_last_ref(first));
    assert(pmm_free_pages() == 14);

    assert(pmm_share_page(first));
    assert(pmm_refcount(first) == 2);
    assert(!pmm_is_last_ref(first));
    assert(pmm_free_pages() == 14);

    pmm_free_page(first);
    assert(pmm_refcount(first) == 1);
    assert(pmm_is_last_ref(first));
    assert(pmm_free_pages() == 14);

    pmm_free_page(first);
    assert(pmm_refcount(first) == 0);
    assert(pmm_free_pages() == 15);
    assert(pmm_alloc_page() == first);
    assert(pmm_refcount(first) == 1);

    pmm_free_page(0x123);
    pmm_free_page(0x90000000);
    assert(!pmm_share_page(0x123));
    assert(!pmm_share_page(0x90000000));

    return 0;
}
