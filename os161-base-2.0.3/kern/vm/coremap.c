#include <coremap.h>
#include <errno.h>
#include <spinlock.h>
#include <string.h>
#include <vm.h>

static struct spinlock free_list_lock = SPINLOCK_INITIALIZER;

void coremap_init(int n_pages) {
    vaddr_t last_addr = PADDR_TO_KVADDR(ram_getsize());
    free_list* addr = PADDR_TO_KVADDR(ram_getfirstfree());
    for (; addr < last_addr; addr += PAGE_SIZE) {
        addr->next = frames;
        frames = addr;
    }
}

paddr_t get_frame() {
    vaddr_t ret;
    spinlock_acquire(&free_list_lock);
    if (frames == NULL) {
        spinlock_release(&free_list_lock);
        return NULL;
    } else {
        ret = frames;
        frames = frames->next;
    }
    spinlock_release(&free_list_lock);
    // controlla che effettivamente tutta la pagina sia azzerata
    memset((void*)ret, '\0', PAGE_SIZE);
    return ret - MIPS_KSEG0;
}

void free_frame(paddr_t addr) {
    free_list* target = PADDR_TO_KVADDR(addr);

    spinlock_acquire(&free_list_lock);

    target->next = frames;
    frames->next = target;

    spinlock_release(&free_list_lock);
}
