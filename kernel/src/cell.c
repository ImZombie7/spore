#include "cell.h"

#include "kprintf.h"
#include "mem.h"
#include "mm/pmm.h"
#include "pl011.h"

static struct cell cells[MAX_CELLS];
static struct snapshot snapshots[MAX_SNAPSHOTS];
static struct open_file open_files[MAX_OPEN_FILES];
static struct cell *current_cell;
static uint64_t kernel_hhdm;
static int next_pid = 1;
static int next_snapshot_id;

static void poweroff(void) {
    __asm__ volatile(
        "mov x0, #0x0008\n"
        "movk x0, #0x8400, lsl #16\n"
        "hvc #0\n"
        :
        :
        : "x0", "memory");
}

static struct cell *find_cell(int pid) {
    for (size_t i = 0; i < MAX_CELLS; ++i) {
        if (cells[i].state != CELL_UNUSED && cells[i].pid == pid) {
            return &cells[i];
        }
    }
    return NULL;
}

static struct cell *alloc_cell(void) {
    for (size_t i = 0; i < MAX_CELLS; ++i) {
        if (cells[i].state == CELL_UNUSED) {
            kmemset(&cells[i], 0, sizeof(cells[i]));
            cells[i].pid = next_pid++;
            cells[i].wait_target = -1;
            return &cells[i];
        }
    }
    return NULL;
}

static struct open_file *alloc_open_file(void) {
    for (size_t i = 0; i < MAX_OPEN_FILES; ++i) {
        if (!open_files[i].used) {
            kmemset(&open_files[i], 0, sizeof(open_files[i]));
            open_files[i].used = true;
            open_files[i].refcount = 1;
            return &open_files[i];
        }
    }
    return NULL;
}

static void retain_open_file(struct open_file *file) {
    if (file != NULL) {
        ++file->refcount;
    }
}

static void release_open_file(struct open_file *file) {
    if (file == NULL || file->refcount == 0) {
        return;
    }
    --file->refcount;
    if (file->refcount == 0) {
        file->used = false;
    }
}

static void close_all_fds(struct cell *cell) {
    for (size_t i = 0; i < MAX_FDS; ++i) {
        release_open_file(cell->fds[i]);
        cell->fds[i] = NULL;
    }
}

static bool init_stdio(struct cell *cell) {
    struct open_file *in = alloc_open_file();
    struct open_file *out = alloc_open_file();
    struct open_file *err = alloc_open_file();
    if (in == NULL || out == NULL || err == NULL) {
        release_open_file(in);
        release_open_file(out);
        release_open_file(err);
        return false;
    }
    in->type = OPEN_STDIN;
    out->type = OPEN_STDOUT;
    err->type = OPEN_STDOUT;
    cell->fds[0] = in;
    cell->fds[1] = out;
    cell->fds[2] = err;
    return true;
}

static void copy_fd_table(struct cell *dst, const struct cell *src) {
    for (size_t i = 0; i < MAX_FDS; ++i) {
        dst->fds[i] = src->fds[i];
        retain_open_file(dst->fds[i]);
    }
}

static struct snapshot *find_snapshot(int id) {
    for (size_t i = 0; i < MAX_SNAPSHOTS; ++i) {
        if (snapshots[i].used && snapshots[i].id == id) {
            return &snapshots[i];
        }
    }
    return NULL;
}

static struct snapshot *alloc_snapshot(void) {
    for (size_t i = 0; i < MAX_SNAPSHOTS; ++i) {
        if (!snapshots[i].used) {
            kmemset(&snapshots[i], 0, sizeof(snapshots[i]));
            snapshots[i].used = true;
            snapshots[i].id = next_snapshot_id++;
            return &snapshots[i];
        }
    }
    return NULL;
}

static void wake_parent_of(const struct cell *child) {
    struct cell *parent = find_cell(child->parent_pid);
    if (parent != NULL && parent->state == CELL_BLOCKED &&
        (parent->wait_target < 0 || parent->wait_target == child->pid)) {
        int status = child->exit_status << 8;
        uint64_t status_addr = parent->tf.x[1];
        if (status_addr != 0) {
            (void)vmm_copy_to_user(&parent->as, status_addr, &status, sizeof(status));
        }
        parent->tf.x[0] = (uint64_t)child->pid;
        close_all_fds((struct cell *)child);
        vmm_destroy((struct user_address_space *)&child->as);
        ((struct cell *)child)->state = CELL_UNUSED;
        parent->state = CELL_RUNNABLE;
        parent->wait_target = -1;
    }
}

void cell_system_init(uint64_t hhdm_offset) {
    kernel_hhdm = hhdm_offset;
    kmemset(cells, 0, sizeof(cells));
    kmemset(snapshots, 0, sizeof(snapshots));
    kmemset(open_files, 0, sizeof(open_files));
    current_cell = NULL;
    next_pid = 1;
    next_snapshot_id = 0;
    // v1 is cooperative and UP: cell table state has no locks. v2 preemption/SMP
    // must add synchronization here.
}

bool cell_create_init(struct user_address_space *as, uint64_t entry, uint64_t sp) {
    struct cell *cell = alloc_cell();
    if (cell == NULL) {
        return false;
    }
    cell->parent_pid = 0;
    cell->state = CELL_RUNNABLE;
    cell->as = *as;
    cell->as.asid = 0;
    vma_list_init(&cell->vmas);
    if (!init_stdio(cell)) {
        cell->state = CELL_UNUSED;
        return false;
    }
    cell->tf.elr_el1 = entry;
    cell->tf.sp_el0 = sp;
    cell->tf.spsr_el1 = 0x3c0;
    current_cell = cell;
    return true;
}

struct user_address_space *cell_current_as(void) {
    return current_cell == NULL ? NULL : &current_cell->as;
}

int cell_current_pid(void) {
    return current_cell == NULL ? 0 : current_cell->pid;
}

int cell_current_ppid(void) {
    return current_cell == NULL ? 0 : current_cell->parent_pid;
}

void cell_save_current(const struct trap_frame *frame) {
    if (current_cell == NULL) {
        return;
    }
    current_cell->tf = *frame;
    __asm__ volatile("mrs %0, tpidr_el0" : "=r"(current_cell->tpidr_el0));
}

void cell_restore_current(struct trap_frame *frame) {
    if (current_cell == NULL) {
        return;
    }
    vmm_install_user(&current_cell->as);
    __asm__ volatile("msr tpidr_el0, %0" : : "r"(current_cell->tpidr_el0));
    *frame = current_cell->tf;
}

void cell_schedule(struct trap_frame *frame) {
    cell_save_current(frame);
    if (current_cell != NULL && current_cell->state == CELL_RUNNABLE) {
        size_t start = (size_t)(current_cell - cells + 1);
        for (size_t n = 0; n < MAX_CELLS; ++n) {
            struct cell *candidate = &cells[(start + n) % MAX_CELLS];
            if (candidate->state == CELL_RUNNABLE) {
                current_cell = candidate;
                cell_restore_current(frame);
                return;
            }
        }
    }
    for (size_t i = 0; i < MAX_CELLS; ++i) {
        if (cells[i].state == CELL_RUNNABLE) {
            current_cell = &cells[i];
            cell_restore_current(frame);
            return;
        }
    }
    kprintf("[kernel] no runnable cells\n");
    poweroff();
    for (;;) {
        __asm__ volatile("wfe");
    }
}

void cell_exit_current(int status, struct trap_frame *frame) {
    if (current_cell == NULL) {
        return;
    }
    current_cell->exit_status = status;
    current_cell->state = CELL_ZOMBIE;
    wake_parent_of(current_cell);
    cell_schedule(frame);
}

int cell_fork_current(struct trap_frame *frame) {
    struct cell *child = alloc_cell();
    if (child == NULL) {
        return -12;
    }
    cell_save_current(frame);
    if (!vmm_clone_cow(&child->as, &current_cell->as, 0)) {
        child->state = CELL_UNUSED;
        return -12;
    }
    (void)vma_clone(&child->vmas, &current_cell->vmas);
    copy_fd_table(child, current_cell);
    child->parent_pid = current_cell->pid;
    child->state = CELL_RUNNABLE;
    child->tf = current_cell->tf;
    child->tf.x[0] = 0;
    child->tpidr_el0 = current_cell->tpidr_el0;
    current_cell->tf.x[0] = (uint64_t)child->pid;
    return child->pid;
}

static struct cell *find_waitable_child(int parent_pid, int pid) {
    for (size_t i = 0; i < MAX_CELLS; ++i) {
        if (cells[i].state == CELL_ZOMBIE && cells[i].parent_pid == parent_pid &&
            (pid <= 0 || cells[i].pid == pid)) {
            return &cells[i];
        }
    }
    return NULL;
}

static bool has_child(int parent_pid, int pid) {
    for (size_t i = 0; i < MAX_CELLS; ++i) {
        if (cells[i].state != CELL_UNUSED && cells[i].parent_pid == parent_pid &&
            (pid <= 0 || cells[i].pid == pid)) {
            return true;
        }
    }
    return false;
}

int cell_wait4(int pid, uint64_t status_addr, struct trap_frame *frame) {
    struct cell *child = find_waitable_child(current_cell->pid, pid);
    if (child == NULL) {
        if (!has_child(current_cell->pid, pid)) {
            return -10;
        }
        cell_save_current(frame);
        current_cell->state = CELL_BLOCKED;
        current_cell->wait_target = pid;
        cell_schedule(frame);
        return CELL_SWITCHED;
    }

    int status = child->exit_status << 8;
    if (status_addr != 0 && !vmm_copy_to_user(&current_cell->as, status_addr, &status, sizeof(status))) {
        return -14;
    }
    int child_pid = child->pid;
    close_all_fds(child);
    vmm_destroy(&child->as);
    child->state = CELL_UNUSED;
    current_cell->wait_target = -1;
    return child_pid;
}

int cell_kill(int pid, int signal) {
    (void)signal;
    struct cell *cell = find_cell(pid);
    if (cell == NULL || cell->state == CELL_UNUSED || cell->state == CELL_ZOMBIE) {
        return -3;
    }
    cell->exit_status = 128 + signal;
    cell->state = CELL_ZOMBIE;
    wake_parent_of(cell);
    return 0;
}

bool cell_handle_cow_fault(uint64_t far) {
    return current_cell != NULL && vmm_handle_cow_fault(&current_cell->as, far);
}

int64_t cell_fd_write(int fd, uint64_t buf, uint64_t len) {
    if (current_cell == NULL || fd < 0 || fd >= MAX_FDS ||
        current_cell->fds[fd] == NULL) {
        return -9;
    }
    struct open_file *file = current_cell->fds[fd];
    if (file->type != OPEN_STDOUT) {
        return -22;
    }
    for (uint64_t i = 0; i < len; ++i) {
        char c;
        if (!vmm_copy_from_user(&current_cell->as, &c, buf + i, 1)) {
            return -14;
        }
        pl011_putc(c);
    }
    return (int64_t)len;
}

int64_t cell_fd_read(int fd, uint64_t buf, uint64_t len) {
    if (current_cell == NULL || fd < 0 || fd >= MAX_FDS ||
        current_cell->fds[fd] == NULL) {
        return -9;
    }
    struct open_file *file = current_cell->fds[fd];
    if (file->type == OPEN_STDIN) {
        return 0;
    }
    if (file->type != OPEN_RAMFS || file->node.is_dir) {
        return -22;
    }
    uint64_t remaining = file->node.size > file->offset ? file->node.size - file->offset : 0;
    uint64_t n = remaining < len ? remaining : len;
    if (!vmm_copy_to_user(&current_cell->as,
                          buf,
                          (const uint8_t *)file->node.data + file->offset,
                          (size_t)n)) {
        return -14;
    }
    file->offset += n;
    return (int64_t)n;
}

int64_t cell_fd_lseek(int fd, int64_t off, int whence) {
    if (current_cell == NULL || fd < 0 || fd >= MAX_FDS ||
        current_cell->fds[fd] == NULL) {
        return -9;
    }
    struct open_file *file = current_cell->fds[fd];
    int64_t base = 0;
    if (whence == 0) {
        base = 0;
    } else if (whence == 1) {
        base = (int64_t)file->offset;
    } else if (whence == 2) {
        base = (int64_t)file->node.size;
    } else {
        return -22;
    }
    int64_t next = base + off;
    if (next < 0) {
        return -22;
    }
    file->offset = (uint64_t)next;
    return next;
}

int cell_fd_open_node(const struct ramfs_node *node, uint32_t flags) {
    if (current_cell == NULL) {
        return -12;
    }
    int fd = -1;
    for (int i = 0; i < MAX_FDS; ++i) {
        if (current_cell->fds[i] == NULL) {
            fd = i;
            break;
        }
    }
    if (fd < 0) {
        return -24;
    }
    struct open_file *file = alloc_open_file();
    if (file == NULL) {
        return -12;
    }
    file->type = OPEN_RAMFS;
    file->flags = flags;
    file->node = *node;
    current_cell->fds[fd] = file;
    return fd;
}

int cell_fd_dup(int oldfd, int minfd) {
    if (current_cell == NULL || oldfd < 0 || oldfd >= MAX_FDS ||
        minfd < 0 || minfd >= MAX_FDS || current_cell->fds[oldfd] == NULL) {
        return -9;
    }
    for (int fd = minfd; fd < MAX_FDS; ++fd) {
        if (current_cell->fds[fd] == NULL) {
            current_cell->fds[fd] = current_cell->fds[oldfd];
            retain_open_file(current_cell->fds[fd]);
            return fd;
        }
    }
    return -24;
}

int cell_fd_close(int fd) {
    if (current_cell == NULL || fd < 0 || fd >= MAX_FDS ||
        current_cell->fds[fd] == NULL) {
        return -9;
    }
    release_open_file(current_cell->fds[fd]);
    current_cell->fds[fd] = NULL;
    return 0;
}

bool cell_fd_stat(int fd, struct ramfs_node *out) {
    if (current_cell == NULL || fd < 0 || fd >= MAX_FDS ||
        current_cell->fds[fd] == NULL) {
        return false;
    }
    struct open_file *file = current_cell->fds[fd];
    if (file->type == OPEN_STDOUT || file->type == OPEN_STDIN) {
        *out = (struct ramfs_node) {.ino = 10, .is_dir = false};
        return true;
    }
    *out = file->node;
    return true;
}

bool cell_fd_next_dirent(int fd, struct ramfs_dirent *out) {
    if (current_cell == NULL || fd < 0 || fd >= MAX_FDS ||
        current_cell->fds[fd] == NULL) {
        return false;
    }
    struct open_file *file = current_cell->fds[fd];
    if (file->type != OPEN_RAMFS || !file->node.is_dir) {
        return false;
    }
    if (!ramfs_root_dirent((size_t)file->offset, out)) {
        return false;
    }
    ++file->offset;
    return true;
}

void cell_fd_rewind_one_dirent(int fd) {
    if (current_cell != NULL && fd >= 0 && fd < MAX_FDS &&
        current_cell->fds[fd] != NULL && current_cell->fds[fd]->offset > 0) {
        --current_cell->fds[fd]->offset;
    }
}

uint64_t cell_fd_dir_offset(int fd) {
    if (current_cell == NULL || fd < 0 || fd >= MAX_FDS ||
        current_cell->fds[fd] == NULL) {
        return 0;
    }
    return current_cell->fds[fd]->offset;
}

static bool access_allowed(const struct vma *vma, enum vmm_access access) {
    switch (access) {
    case VMM_ACCESS_READ:
        return (vma->prot & VMM_USER_READ) != 0;
    case VMM_ACCESS_WRITE:
        return (vma->prot & VMM_USER_WRITE) != 0;
    case VMM_ACCESS_EXEC:
        return (vma->prot & VMM_USER_EXEC) != 0;
    }
    return false;
}

bool cell_handle_translation_fault(uint64_t far, enum vmm_access access) {
    if (current_cell == NULL) {
        return false;
    }
    uint64_t va = far & ~(uint64_t)(PAGE_SIZE - 1);
    const struct vma *vma = vma_lookup(&current_cell->vmas, va);
    if (vma == NULL || vma->type != VMA_ANON || !access_allowed(vma, access)) {
        return false;
    }
    return vmm_alloc_page(&current_cell->as, va, vma->prot);
}

bool cell_ensure_user_range(uint64_t va, size_t len, enum vmm_access access) {
    if (current_cell == NULL || len == 0) {
        return true;
    }
    uint64_t end = va + len - 1;
    if (end < va) {
        return false;
    }
    uint64_t page = va & ~(uint64_t)(PAGE_SIZE - 1);
    uint64_t last = end & ~(uint64_t)(PAGE_SIZE - 1);
    for (;;) {
        if (!vmm_is_mapped(&current_cell->as, page) &&
            !cell_handle_translation_fault(page, access)) {
            return false;
        }
        if (access == VMM_ACCESS_WRITE &&
            !vmm_user_range_accessible(&current_cell->as, page, 1, VMM_ACCESS_WRITE) &&
            !vmm_handle_cow_fault(&current_cell->as, page)) {
            return false;
        }
        if (!vmm_user_range_accessible(&current_cell->as, page, 1, access)) {
            return false;
        }
        if (page == last) {
            return true;
        }
        page += PAGE_SIZE;
    }
}

bool cell_add_vma(uint64_t start, uint64_t end, uint32_t prot, uint32_t flags) {
    return current_cell != NULL && vma_insert(&current_cell->vmas, start, end, prot, flags, VMA_ANON);
}

bool cell_remove_vma(uint64_t start, uint64_t end) {
    if (current_cell == NULL) {
        return false;
    }
    vmm_unmap_range(&current_cell->as, start, end);
    return vma_remove(&current_cell->vmas, start, end);
}

bool cell_protect_vma(uint64_t start, uint64_t end, uint32_t prot) {
    if (current_cell == NULL) {
        return false;
    }
    if (!vma_protect(&current_cell->vmas, start, end, prot)) {
        return false;
    }
    vmm_protect_range(&current_cell->as, start, end, prot);
    return true;
}

size_t cell_resident_pages(uint64_t start, uint64_t end) {
    if (current_cell == NULL) {
        return 0;
    }
    return vmm_mapped_pages_in_range(&current_cell->as, start, end);
}

int snapshot_create_current(void) {
    struct snapshot *snap = alloc_snapshot();
    if (snap == NULL) {
        return -12;
    }
    if (!vmm_clone_cow(&snap->as, &current_cell->as, 0)) {
        snap->used = false;
        return -12;
    }
    (void)vma_clone(&snap->vmas, &current_cell->vmas);
    return snap->id;
}

int snapshot_spawn(int snap_id, uint64_t entry, uint64_t arg) {
    struct snapshot *snap = find_snapshot(snap_id);
    struct cell *child = alloc_cell();
    if (snap == NULL || child == NULL) {
        return -12;
    }
    if (!vmm_clone_cow(&child->as, &snap->as, 0)) {
        child->state = CELL_UNUSED;
        return -12;
    }
    (void)vma_clone(&child->vmas, &snap->vmas);
    copy_fd_table(child, current_cell);
    child->parent_pid = current_cell->pid;
    child->state = CELL_RUNNABLE;
    child->tf = current_cell->tf;
    child->tf.elr_el1 = entry;
    child->tf.x[0] = arg;
    child->tf.x[1] = (uint64_t)child->pid;
    child->tpidr_el0 = current_cell->tpidr_el0;
    return child->pid;
}

int snapshot_reap(int pid, uint64_t status_addr, struct trap_frame *frame) {
    return cell_wait4(pid, status_addr, frame);
}
