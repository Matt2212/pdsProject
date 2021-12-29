
#include <types.h>
#include <coremap.h>
#include <lib.h>
#include <spinlock.h>
#include <vm.h>

static struct spinlock free_list_lock = SPINLOCK_INITIALIZER;

static struct cm_entry* coremap = NULL;
static int npages = 0;
static int first_page = 0;
// rifare coremap con una bitmap
int coremap_bootstrap(paddr_t lastpaddr, paddr_t firstpaddr) {
    int kernpages, i, no_fit = 1;
    npages = lastpaddr / PAGE_SIZE;
    coremap = kmalloc(npages*sizeof(struct cm_entry));
    if (!coremap) return false;
    if (!(firstpaddr % PAGE_SIZE))
        no_fit = 0;
    kernpages = (firstpaddr & PAGE_FRAME) / PAGE_SIZE + no_fit; // forse è la prima pagina non kernel attento all'allineamento. se l'indirizzo non risulta allineato la pagina appartiene al kernel altrimenti no
    KASSERT(npages > kernpages);
    for (i = 0; i <= kernpages; i++) {
        coremap[i].occ = true;
        coremap[i].fixed = true;
    }
    first_page = kernpages + 1;
    return true;
}

paddr_t get_n_frames(int num) { //per il momento sono tutti fixed
    int i,j, count = 0;
    if (num > MAX_CONT_PAGES || num == 0) return 0;
    spinlock_acquire(&free_list_lock);
    for (i = first_page; i < npages; i++) {
        if (!coremap[i].occ)
            count++;
        else
            count = 0;
        if (count == num) {
            for (j = i; j > i - num; j--) {
                coremap[j].occ = true;
                coremap[j].fixed = true;
            }
            coremap[j].nframes = num;
            spinlock_release(&free_list_lock);
            return j * PAGE_SIZE;
        }
    }
    spinlock_release(&free_list_lock);
    return 0;
}

void free_frame(paddr_t addr) {
    int index, i;
    KASSERT((addr & PAGE_FRAME) == addr);
    index = addr / PAGE_SIZE;
    spinlock_acquire(&free_list_lock);
    KASSERT(coremap[index].occ);
    for (i = 0; i < coremap[index].nframes; i++) {
        coremap[index].occ = false;
        coremap[index].fixed = false;
    }
    coremap[index].nframes = 0;
    spinlock_release(&free_list_lock);
}
