#include <coremap.h>
#include <vm.h>
#include <spinlock.h>

static struct spinlock free_list_lock = SPINLOCK_INITIALIZER;
static struct free_list* frames = NULL;

void coremap_init(void) {
    struct free_list* last_addr = PADDR_TO_KVADDR(((struct free_list*)ram_getsize()));
    struct free_list* addr = PADDR_TO_KVADDR(((struct free_list*)ram_getfirstfree()));
    for (; addr < last_addr; addr += PAGE_SIZE) {
        addr->next = frames;
        frames = addr;
    }
}

paddr_t get_frame(void) {
    vaddr_t ret;
    spinlock_acquire(&free_list_lock);
    if (frames == NULL) {
        spinlock_release(&free_list_lock);
        return ((paddr_t) NULL);
    }
    else {
        ret = (paddr_t)frames;
        frames = frames->next;
    }
    spinlock_release(&free_list_lock);
    //controlla che effettivamente tutta la pagina sia azzerata
    //memset((void*) ret, '\0', PAGE_SIZE);
    return ret - MIPS_KSEG0;
}

void free_frame(paddr_t addr) {
    struct free_list* target = ((struct free_list*)PADDR_TO_KVADDR(addr));

    spinlock_acquire(&free_list_lock);

    target->next = frames;
    frames->next = target;

    spinlock_release(&free_list_lock);
}
