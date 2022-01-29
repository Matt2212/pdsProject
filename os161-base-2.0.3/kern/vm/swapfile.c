
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
#include <coremap.h>

#include <vm_stats.h>

static swap_file* swap;
static bool init = false;

//ritorna 0 se non ci sono stati errori
int swap_init() {
    char name[] = "emu0:/SWAPFILE";
    swap = kmalloc(sizeof(swap_file));
    if (swap == NULL) {
        panic("swap_init: OUT OF MEMORY");
        return ENOMEM;
    }
    swap_lock = lock_create("swap_lock");
    if (swap_lock == NULL) {
        kfree(swap);
        panic("swap_init: OUT OF MEMORY");
        return ENOMEM;
    }
    lock_acquire(swap_lock);
    init = true;
    vfs_open(name, O_CREAT | O_RDWR | O_TRUNC, 0664, &swap->file);
    lock_release(swap_lock);
    return 0;
}

// ritorna 0 se non ci sono stati errori
int swap_get(vaddr_t address, unsigned int index) {
    struct iovec iov;
	struct uio ku;
    int err = 0;
    
    bool lock_hold = lock_do_i_hold(swap_lock);
    if(!lock_hold) lock_acquire(swap_lock);

    if (!init) {
        if (!lock_hold) lock_release(swap_lock);
        return EPERM;
    }

    KASSERT(swap->refs[index] > 0);
    swap->refs[index]--;
    //se address è null significa che voglio liberare la pagina dello swap e non fare swap-in, e.g. durante una pt destroy
    
    if ((void *)address == NULL) {
        if (!lock_hold) lock_release(swap_lock);
        return 0;
    }
    

    uio_kinit(&iov, &ku, (void *)address, PAGE_SIZE, index*PAGE_SIZE, UIO_READ);
    err = VOP_READ(swap->file, &ku);

    if(!lock_hold)
        lock_release(swap_lock);
    
    return err;
}

// ritorna 0 se non ci sono stati errori
int swap_set(vaddr_t address, unsigned int* ret_index) {
    struct iovec iov;
	struct uio ku;
    unsigned int index = 0;
    int err = 0;
    
    bool lock_hold = lock_do_i_hold(swap_lock);
    if(!lock_hold) lock_acquire(swap_lock);
    if (!init) {
        if (!lock_hold) lock_release(swap_lock);
        return EPERM;
    }
    for(; index < SWAP_MAX && swap->refs[index] > 0; index++);   
    if (index == SWAP_MAX) {
        if (!lock_hold)  lock_release(swap_lock);
        panic("swap_init: Out of swap space");
        return ENOSPC;
    }
    swap->refs[index] = 1;
    
    uio_kinit(&iov, &ku, (void *)address, PAGE_SIZE, index*PAGE_SIZE, UIO_WRITE);
    err = VOP_WRITE(swap->file, &ku);
    *ret_index = index;
    if (!err) inc_counter(swap_file_writes);
    if(!lock_hold)
        lock_release(swap_lock); //magari rilascialo prima della scrittura
    return err;
}

void swap_close() {
    lock_acquire(swap_lock);
    if (init) {
    init = false;
    vfs_close(swap->file);
    kfree(swap);
    swap = NULL;
    }
    lock_release(swap_lock);
}

int load_from_swap(pt_entry* entry){

    int err;
    paddr_t frame;    
    

    KASSERT(entry->swp);
    //crea la get_swappable_fixed_frame
    frame = get_user_frame(entry);
    if(frame == 0){
        return ENOMEM;
    }

    err = swap_get(PADDR_TO_KVADDR(frame), entry->frame_no);
     
    if(!err){
        entry->frame_no = frame >> 12;
        entry->swp = false;
    }

    return err;
}

void swap_inc_ref(unsigned int index) {
    KASSERT(index < SWAP_MAX);
    if (!lock_do_i_hold(swap_lock)) { //mwttila in una kassert
        lock_acquire(swap_lock);
        swap->refs[index]++;
        lock_release(swap_lock);
    } else
        swap->refs[index]++;
}