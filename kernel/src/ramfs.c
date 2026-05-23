#include "ramfs.h"

#include "mem.h"

static const char motd[] = "welcome to spore\n";

static bool streq(const char *a, const char *b) {
    while (*a != '\0' && *b != '\0') {
        if (*a++ != *b++) {
            return false;
        }
    }
    return *a == '\0' && *b == '\0';
}

static bool path_matches(const struct limine_file *file, const char *path) {
    if (file->string != NULL && streq(file->string, path)) {
        return true;
    }
    if (file->path == NULL) {
        return false;
    }

    const char *module_path = file->path;
    size_t module_len = kstrlen(module_path);
    size_t target_len = kstrlen(path);
    return module_len >= target_len &&
           streq(module_path + module_len - target_len, path);
}

void ramfs_init(struct ramfs *fs, const struct limine_module_response *modules) {
    fs->modules = modules;
}

bool ramfs_lookup(const struct ramfs *fs, const char *path, struct ramfs_file *out) {
    if (fs->modules == NULL) {
        return false;
    }

    for (uint64_t i = 0; i < fs->modules->module_count; ++i) {
        const struct limine_file *file = fs->modules->modules[i];
        if (!path_matches(file, path)) {
            continue;
        }
        out->path = path;
        out->data = file->address;
        out->size = file->size;
        return true;
    }
    return false;
}

bool ramfs_lookup_node(const struct ramfs *fs, const char *path, struct ramfs_node *out) {
    if (streq(path, "/") || streq(path, ".")) {
        *out = (struct ramfs_node) {
            .path = "/",
            .name = "/",
            .ino = 1,
            .is_dir = true,
        };
        return true;
    }
    if (streq(path, "/etc")) {
        *out = (struct ramfs_node) {
            .path = "/etc",
            .name = "etc",
            .ino = 2,
            .is_dir = true,
        };
        return true;
    }
    if (streq(path, "/bin")) {
        *out = (struct ramfs_node) {
            .path = "/bin",
            .name = "bin",
            .ino = 3,
            .is_dir = true,
        };
        return true;
    }
    if (streq(path, "/etc/motd")) {
        *out = (struct ramfs_node) {
            .path = "/etc/motd",
            .name = "motd",
            .data = motd,
            .size = sizeof(motd) - 1,
            .ino = 4,
            .is_dir = false,
        };
        return true;
    }

    struct ramfs_file file;
    if (ramfs_lookup(fs, path, &file)) {
        *out = (struct ramfs_node) {
            .path = "/init",
            .name = "init",
            .data = file.data,
            .size = file.size,
            .ino = 5,
            .is_dir = false,
        };
        return true;
    }
    return false;
}

bool ramfs_root_dirent(size_t index, struct ramfs_dirent *out) {
    static const struct ramfs_dirent entries[] = {
        {.name = "bin", .ino = 3, .is_dir = true},
        {.name = "etc", .ino = 2, .is_dir = true},
        {.name = "init", .ino = 5, .is_dir = false},
    };
    if (index >= sizeof(entries) / sizeof(entries[0])) {
        return false;
    }
    *out = entries[index];
    return true;
}
