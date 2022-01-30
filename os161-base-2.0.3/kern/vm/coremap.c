
#include <types.h>
#include <lib.h>
#include <vm.h>
#include <spl.h>
#include <swapfile.h>
#include <coremap.h>
#include <kern/errno.h>
#include <thread.h>
#include <vm_tlb.h>
#include <pt.h>
#include <current.h>

#define MAX_ATTEMPTS 5

static struct cm_entry* coremap = NULL;
static unsigned int npages = 0;
static unsigned int first_page = 0;


static int get_victim() {
    static int prev_victim = 0;
    int victim = (prev_victim + coremap[prev_victim].nframes) % (npages);
    bool loop = false;
    while (!(coremap[victim].occ && !coremap[victim].fixed && coremap[victim].pt_entry != NULL)) {
        if (loop) return -1;
        victim = (victim + coremap[victim].nframes) % (npages);
        if (victim == prev_victim)
            loop = true;
    }
    prev_victim = victim;
    coremap[victim].fixed = true;
    coremap[victim].pt_entry->swapping = true;
    return victim;
}

void coremap_create(unsigned int n_pages) {
    npages = n_pages;
    coremap = kmalloc(npages * sizeof(struct cm_entry));
}

bool coremap_bootstrap(paddr_t firstpaddr) {
    unsigned int i = 0;
    if (!coremap)
        return false;
    
    first_page = firstpaddr / PAGE_SIZE;  // firstpaddr è l'indirizzo fisico del primo frame libero

    KASSERT(npages > first_page);

    for (i = 0; i < first_page; i++) {
        coremap[i].occ = true;
        coremap[i].fixed = true;
        coremap[i].nframes = 0;
        coremap[i].pt_entry = NULL;
    }
    for (; i < npages; i++) {
        coremap[i].occ = false;
        coremap[i].fixed = false;
        coremap[i].nframes = 0;
        coremap[i].pt_entry = NULL;
    }

    coremap[0].nframes = first_page;
    coremap[first_page].nframes = npages - first_page;
    return true;
}

static paddr_t get_n_frames(unsigned int num, struct pt_entry* entry) {
    
    paddr_t addr = 0;
    uint32_t i = first_page, residual, page;
    bool found = false;
    int spl;
    spl = splhigh();
    if (coremap == NULL) {
        splx(spl);
        return 0;
    }
    while (i < npages && !found) {
        if (!coremap[i].occ && coremap[i].nframes >= num)
            found = true;
        else
            i += coremap[i].nframes;
    }

    if (!found && num != 1) {
        splx(spl);
        return 0;
    } else if (!found) {
        int err = 0;
        unsigned int swap_index;
        i = get_victim();
        splx(spl);

        if ((int)i == -1) {
            return 0;
        }

        err = swap_set(PADDR_TO_KVADDR(i * PAGE_SIZE), &swap_index);
        if (err) {
            spl = splhigh();
            coremap[i].fixed = false;
            coremap[i].pt_entry->swapping = false;
            splx(spl);
            return 0;
        }
        spl = splhigh();
        //mentre effettuo la swap un processo in fase di disrtuzione potrebbe aver liberato il frame vittima
        if (coremap[i].pt_entry != NULL) {
            invalidate_entry_by_paddr(coremap[i].pt_entry->frame_no << 12);
            coremap[i].pt_entry->frame_no = swap_index;
            coremap[i].pt_entry->swp = true;
            coremap[i].pt_entry->swapping = false;
        }
        coremap[i].pt_entry = entry;
        splx(spl);
        addr = (paddr_t)(i * PAGE_SIZE);
        bzero((void*)PADDR_TO_KVADDR(addr), PAGE_SIZE);
        return addr;
    }
    page = i;
    
    // page adesso conterrà il primo frame libero
    // aggiorno la tabella
    residual = coremap[page].nframes - num;

    if (residual && page + num < npages)
        coremap[page + num].nframes = residual;
    coremap[page].nframes = num;


    for (i = page; i < page + num; i++){
        coremap[i].occ = true;
        coremap[i].fixed = true;
        coremap[i].pt_entry = entry;
    }
    splx(spl);
    addr = (paddr_t)(page * PAGE_SIZE);
    bzero((void*)PADDR_TO_KVADDR(addr), PAGE_SIZE*num);
    return addr;    
}

paddr_t get_user_frame(struct pt_entry* entry) {
    paddr_t ret;
    for (int i = 0; i < MAX_ATTEMPTS; i++) {
        ret = get_n_frames(1, entry);
        if (ret)
            return ret;
        thread_yield();
    }
    kprintf("get_user_frame: Not enough space \n");
    return 0;
}

paddr_t get_kernel_frame(unsigned int num) {
    paddr_t ret;
    for (int i = 0; i < MAX_ATTEMPTS; i++){
        ret = get_n_frames(num, NULL);
        if (ret)
            return ret;
        thread_yield();
    }
    kprintf("get_user_frame: Not enough space \n");
    return 0;
}

void free_frame(paddr_t addr) {

    uint32_t i, next, page = addr / PAGE_SIZE, mysize;

    int spl = splhigh();

    if(coremap == NULL){
        return;
    }
    mysize = coremap[page].nframes;
    KASSERT(mysize>0);
    if (coremap[page].pt_entry != NULL && coremap[page].pt_entry->swapping) {  // se si sta effettuando lo swap-out della pagina contenuta nel frame questi campi devono rimanere invariati
        KASSERT(coremap[page].nframes == 1);
        coremap[page].fixed = true;
        coremap[page].pt_entry = NULL;
        splx(spl);
        return;
    }

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
    splx(spl);
}


void coremap_shutdown() {
    npages = 0;
    coremap = NULL;    
    first_page = 0;
}

void coremap_set_fixed(unsigned int index) {
    KASSERT(curthread->t_iplhigh_count > 0);
    coremap[index].fixed = true;
}

void coremap_set_unfixed(unsigned int index) {
    KASSERT(curthread->t_iplhigh_count > 0);
    coremap[index].fixed = false;
}