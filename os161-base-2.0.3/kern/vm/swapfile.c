//#include <kern/fcntl.h>
//#include <lib.h>
#include <machine/vm.h>
#include <swapfile.h>
#include <types.h>
#include <uio.h>
#include <vfs.h>
#include <vnode.h>

static struct spinlock bitmap_lock = SPINLOCK_INITIALIZER;
static swap_file* swap;

void swap_init() {
    swap = kmalloc(sizeof(swap_file));
    #if 0
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
    #endif
}

// FORSE NON VA BENE VADDR_T CONTROLLA SE NEL BUFFER DEVI METTEER UN INDIRIZZO FISICO O LOGICO DEL KERNEL (PROBABILMENTE LA SECONDA)

void swap_get(vaddr_t address, unsigned int index) {
    #if 0
    struct iovec iov;
	struct uio ku;
    spinlock_acquire(&bitmap_lock);
    bitmap_unmark(swap->bitmap, index);
    spinlock_release(&bitmap_lock);
    if (address == NULL) return;
    uio_kinit(&iov, &ku, address, PAGE_SIZE, index*PAGE_SIZE, UIO_READ);
    VOP_READ(swap->file, &ku);
    #else
    (void)address;
    (void)index;
#endif
}

int swap_set(vaddr_t address) {
#if 0
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
#else
    (void)address;
#endif
}

void swap_close() {
    bitmap_destroy(swap->bitmap);
    #if 0
    vfs_close(swap->file);
    #endif
    kfree(swap);
    swap = NULL;
}