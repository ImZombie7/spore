#include "ramfs.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

int main(void) {
    const char init_data[] = "init";
    struct limine_file init = {
        .address = (void *)init_data,
        .size = sizeof(init_data),
        .path = "boot():/boot/init",
        .string = "/init",
    };
    struct limine_file other = {
        .address = (void *)"x",
        .size = 1,
        .path = "boot():/boot/other",
        .string = "/other",
    };
    struct limine_file *files[] = {&other, &init};
    struct limine_module_response modules = {
        .module_count = 2,
        .modules = files,
    };

    struct ramfs fs;
    struct ramfs_file file;
    ramfs_init(&fs, &modules);

    assert(ramfs_lookup(&fs, "/init", &file));
    assert(file.data == init_data);
    assert(file.size == sizeof(init_data));
    assert(strcmp(file.path, "/init") == 0);
    assert(!ramfs_lookup(&fs, "/missing", &file));

    struct ramfs_node node;
    assert(ramfs_lookup_node(&fs, "/", &node));
    assert(node.is_dir);
    assert(ramfs_lookup_node(&fs, "/etc/motd", &node));
    assert(!node.is_dir);
    assert(node.size == 17);
    assert(ramfs_lookup_node(&fs, "/init", &node));
    assert(!node.is_dir);
    assert(node.data == init_data);

    struct ramfs_dirent ent;
    assert(ramfs_root_dirent(0, &ent));
    assert(strcmp(ent.name, "bin") == 0 && ent.is_dir);
    assert(ramfs_root_dirent(1, &ent));
    assert(strcmp(ent.name, "etc") == 0 && ent.is_dir);
    assert(ramfs_root_dirent(2, &ent));
    assert(strcmp(ent.name, "init") == 0 && !ent.is_dir);
    assert(!ramfs_root_dirent(3, &ent));
    return 0;
}
