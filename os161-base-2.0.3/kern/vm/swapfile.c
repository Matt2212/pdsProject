#include <swapfile.h>
#include <kern/fcntl.h>
#include <vfs.h>
#include <lib.h>
#include <vnode.h>
#include <errno.h>
#include <uio.h>

static struct spinlock bitmap_lock = SPINLOCK_INITIALIZER;

void swap_init() {
    swap = kmalloc(sizeof(swap_file));
    if (swap == NULL) {
        panic("OUT OF MEMORY");
        return;
    }
    swap->bitmap = bitmap_create(SWAP_MAX);
    if (swap->bitmap == NULL) {
        panic("OUT OF MEMORY");
        kfree(swap);
        return NULL;
    }
    vfs_open("SWAP_FILE", O_CREAT | O_RDWR, 0,&swap->file);
}

// FORSE NON VA BENE VADDR_T CONTROLLA SE NEL BUFFER DEVI METTEER UN INDIRIZZO FISICO O LOGICO DEL KERNEL (PROBABILMENTE LA SECONDA)

void swap_get(vaddr_t address, unsigned int index) {
    struct iovec iov;
	struct uio ku;
    spinlock_acquire(&bitmap_lock);
    bitmap_unmark(swap->bitmap, index);
    spinlock_release(&bitmap_lock);
    uio_kinit(&iov, &ku, address, PAGE_SIZE, index*PAGE_SIZE, UIO_READ);
    VOP_READ(swap->file, &ku);
}

int swap_set(vaddr_t address) {
    struct iovec iov;
	struct uio ku;
    int index;
    spinlock_acquire(&bitmap_lock);
    if (bitmap_alloc(swap->bitmap, &index) == ENOSPC)
        panic("OUT OF MEMORY");
    spinlock_release(&bitmap_lock);
    uio_kinit(&iov, &ku, address, PAGE_SIZE, index*PAGE_SIZE, UIO_WRITE);
    VOP_WRITE(swap->file, &ku);
    return index;
}

void swap_close() {
    bitmap_destroy(swap->bitmap);
    vfs_close(swap->file);
    kfree(swap);
    swap = NULL;
}