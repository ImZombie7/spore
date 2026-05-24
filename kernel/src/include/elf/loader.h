#pragma once

#include "mm/vmm.h"

#include <stddef.h>
#include <stdint.h>

struct loaded_elf {
  uint64_t entry;
  uint64_t runtime_entry;
  uint64_t load_base;
  uint64_t at_base;
  uint64_t phdr;
  uint16_t phent;
  uint16_t phnum;
  uint64_t brk_base;
};

bool elf_load_static_aarch64(struct user_address_space *as, const void *image, size_t image_size,
                             struct loaded_elf *out);
bool elf_load_aarch64(struct user_address_space *as, const void *image, size_t image_size, uint64_t load_base,
                      struct loaded_elf *out);
bool elf_find_interp_aarch64(const void *image, size_t image_size, char *out, size_t out_cap);
