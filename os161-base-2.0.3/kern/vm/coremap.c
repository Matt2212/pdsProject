
#include <types.h>
#include <coremap.h>
#include <lib.h>
#include <pt.h>
#include <spinlock.h>
#include <vm.h>


struct cm_entry {
    uint32_t occ : 1;
    uint32_t fixed : 1;
    uint32_t nframes : 20;  // quanti frame contigui a questo sono stati allocati o sono liberi
    pt_entry *pt_entry;
};


static struct cm_entry* coremap = NULL;
static unsigned int npages = 0;
static unsigned int first_page = 0;

void coremap_create(unsigned int n_pages) {
    npages = n_pages;
    coremap = kmalloc(npages * sizeof(struct cm_entry));
}
int coremap_bootstrap(paddr_t firstpaddr) {
    unsigned int i = 0;
    if (!coremap) return false;
    first_page = firstpaddr / PAGE_SIZE;  // firstpaddr è l'indirizzo fisico del primo frame libero

    KASSERT(npages > first_page);

    for (i = 0; i < npages; i++) {
        coremap[i].fixed = 0;
        coremap[i].fixed = false;
        coremap[i].nframes = 0;
        coremap[i].pt_entry = NULL;
    }
    coremap[0].nframes = first_page;
    coremap[first_page].nframes = npages - first_page;
    return true;
}

paddr_t get_n_frames(unsigned int num) {  // per il momento sono tutti fixed
    #if 0
    unsigned int i, j, count = 0;
    if (num > MAX_CONT_PAGES || num == 0) return 0;
    spinlock_acquire(&free_list_lock);
    for (i = first_page; i < npages; i++) {
        if (!coremap[i].occ)
            count++;
        else
            count = 0;
        if (count == num) {
            for (j = 0; j < num; j++) {
                coremap[i - j].occ = true;
                coremap[i - j].fixed = true;
            }
            coremap[i - j + 1].nframes = num;
            spinlock_release(&free_list_lock);
            return (i - j + 1) * PAGE_SIZE;
        }
    }
    
    spinlock_release(&free_list_lock);
    return 0;
#else
    paddr_t addr = 0;
    uint32_t i = first_page, residual, page;
    bool found = false;
    if (num == 0) return 0;

    while (i < npages && !found) {
        if (!coremap[i].occ && coremap[i].nframes >= num)
            found = true;
        else
            i += coremap[i].nframes;
    }

    if (!found){
        return 0;
    }

    page = i;
    // page adesso conterrà il primo frame libero
    // aggiorno la tabella
    residual = coremap[page].nframes - num;

    if (residual && page + num < npages)
        coremap[page + num].nframes = residual;
    KASSERT(coremap[page + num].nframes > 0);
    coremap[page].nframes = num;

    for (i = page; i < page + num; i++){
        coremap[i].occ = true;
        coremap[i].fixed = true;
    }
    addr = (paddr_t)(page * PAGE_SIZE);
    return addr;
#endif

}

void free_frame(paddr_t addr) {
#if 0
    unsigned int index, i;
    KASSERT((addr & PAGE_FRAME) == addr);
    index = addr / PAGE_SIZE;
    spinlock_acquire(&free_list_lock);
    KASSERT(coremap[index].occ);
    for (i = 0; i < coremap[index].nframes; i++) {
        coremap[index + i].occ = false;
        coremap[index + i].fixed = false;
    }
    coremap[index].nframes = 0;
    spinlock_release(&free_list_lock);
    return;
#else
    uint32_t i, next, page = addr / PAGE_SIZE, mysize;

    mysize = coremap[page].nframes;

    for (i = 0; i < mysize; i++) {
        coremap[page + i].occ = false;
        coremap[page + i].fixed = false;
        coremap[page + i].pt_entry = NULL;
    }

    i = 0;
    next = coremap[i].nframes;
    while (next < npages) {
        if (!coremap[i].occ) {
            while (next < npages && !coremap[next].occ) {
                uint32_t size = coremap[next].nframes;
                coremap[i].nframes += size;
                coremap[next].nframes = 0;
                next += size;
            }
            if (next >= npages) break;
        }
        i = next;
        next += coremap[next].nframes;
    }

#endif
}
