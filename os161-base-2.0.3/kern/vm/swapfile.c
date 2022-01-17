
#include <types.h>
#include <machine/vm.h>
#include <kern/fcntl.h>
#include <swapfile.h>
#include <lib.h>
#include <uio.h>
#include <vfs.h>
#include <vnode.h>
#include <synch.h>
#include <kern/errno.h>

static struct lock* swap_lock;
static swap_file* swap;
static bool init = false;

//ritorna 0 se non ci sono stati errori
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
    swap_lock = lock_create("swap_lock");
    if (swap_lock == NULL) {
        kfree(swap);
        kfree(swap->bitmap);
        panic("OUT OF MEMORY");
        return ENOMEM;
    }
    lock_acquire(swap_lock);
    init = true;
    lock_release(swap_lock);
    vfs_open(name, O_CREAT | O_RDWR | O_TRUNC, 0664, &swap->file);
    return 0;
}

// ritorna 0 se non ci sono stati errori
int swap_get(vaddr_t address, unsigned int index) {
    struct iovec iov;
	struct uio ku;
        int err = 0;
        lock_acquire(swap_lock);
        if (!init) {
            lock_release(swap_lock);
            return EPERM;
    }
    bitmap_unmark(swap->bitmap, index);
    
    if ((void *)address == NULL) {
        lock_release(swap_lock);
        return ENOSPC;
}
    uio_kinit(&iov, &ku, (void *)address, PAGE_SIZE, index*PAGE_SIZE, UIO_READ);
    err = VOP_READ(swap->file, &ku);
    lock_release(swap_lock);
    return err;
}

// ritorna 0 se non ci sono stati errori
int swap_set(vaddr_t address, unsigned int* ret_index) {
    struct iovec iov;
	struct uio ku;
    unsigned int index;
    int err = 0;
    lock_acquire(swap_lock);
    if (!init) {
        lock_release(swap_lock);
        return EPERM;
    }
    if (bitmap_alloc(swap->bitmap, &index) == ENOSPC){
        lock_release(swap_lock);
        panic("Out of swap space");
        return ENOSPC;
    }

    
    uio_kinit(&iov, &ku, (void *)address, PAGE_SIZE, index*PAGE_SIZE, UIO_WRITE);
    err = VOP_WRITE(swap->file, &ku);
    *ret_index = index;
    lock_release(swap_lock);
    return err;
}

void swap_close() {
    lock_acquire(swap_lock);
    bitmap_destroy(swap->bitmap);
    init = false;
    lock_release(swap_lock);
    lock_destroy(swap_lock);
    vfs_close(swap->file);
    kfree(swap);
    swap = NULL;
}