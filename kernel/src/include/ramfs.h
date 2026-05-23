#pragma once

#include <limine.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct ramfs_file {
    const char *path;
    const void *data;
    uint64_t size;
};

struct ramfs_node {
    const char *path;
    const char *name;
    const void *data;
    uint64_t size;
    uint64_t ino;
    bool is_dir;
};

struct ramfs_dirent {
    const char *name;
    uint64_t ino;
    bool is_dir;
};

struct ramfs {
    const struct limine_module_response *modules;
};

void ramfs_init(struct ramfs *fs, const struct limine_module_response *modules);
bool ramfs_lookup(const struct ramfs *fs, const char *path, struct ramfs_file *out);
bool ramfs_lookup_node(const struct ramfs *fs, const char *path, struct ramfs_node *out);
bool ramfs_root_dirent(size_t index, struct ramfs_dirent *out);
