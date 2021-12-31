
#include <types.h>
#include <machine/vm.h>
#include <kern/fcntl.h>
#include <swapfile.h>
#include <lib.h>
#include <uio.h>
#include <vfs.h>
#include <vnode.h>
#include <kern/errno.h>

static struct spinlock swap_lock = SPINLOCK_INITIALIZER;
static swap_file* swap;
static bool init = false;

int swap_init() {
    char name[] = "emu0:/.SWAP_FILE";
    swap = kmalloc(sizeof(swap_file));
    if (swap == NULL) {
        panic("OUT OF MEMORY");
        return ENOMEM;
    }
    swap->bitmap = bitmap_create(SWAP_MAX);
    if (swap->bitmap == NULL) {
        kfree(swap);
        panic("OUT OF MEMORY");
        return ENOMEM;
    }
    spinlock_acquire(&swap_lock);
    init = true;
    spinlock_release(&swap_lock);
    vfs_open(name, O_CREAT | O_RDWR | O_TRUNC, 0664, &swap->file);
}

int swap_get(vaddr_t address, unsigned int index) {
    struct iovec iov;
	struct uio ku;
    spinlock_acquire(&swap_lock);
    if (!init){
        spinlock_release(&swap_lock);
        return EPERM;
    }
    bitmap_unmark(swap->bitmap, index);
    spinlock_release(&swap_lock);
    if ((void *)address == NULL) return ENOSPC;
    uio_kinit(&iov, &ku, (void *)address, PAGE_SIZE, index*PAGE_SIZE, UIO_READ);
    return VOP_READ(swap->file, &ku);
}

int swap_set(vaddr_t address, unsigned int index) {
    struct iovec iov;
	struct uio ku;
    unsigned int index;
    spinlock_acquire(&swap_lock);
    if (!init) {
        spinlock_release(&swap_lock);
        return EPERM;
    }
    if (bitmap_alloc(swap->bitmap, &index) == ENOSPC){
        spinlock_release(&swap_lock);
        panic("OUT OF MEMORY");
        return ENOSPC;
    }

    spinlock_release(&swap_lock);
    uio_kinit(&iov, &ku, (void *)address, PAGE_SIZE, index*PAGE_SIZE, UIO_WRITE);
    VOP_WRITE(swap->file, &ku);
    return index;
}

void swap_close() {
    spinlock_acquire(&swap_lock);
    bitmap_destroy(swap->bitmap);
    init = false;
    spinlock_release(&swap_lock);
    vfs_close(swap->file);
    kfree(swap);
    swap = NULL;
}