
#include <types.h>
#include <coremap.h>
#include <lib.h>
#include <synch.h>
#include <vm.h>
#include <swapfile.h>


struct cm_entry {
    uint32_t occ : 1;
    uint32_t fixed : 1;
    uint32_t nframes : 20;  // quanti frame contigui a questo sono stati allocati o sono liberi
    pt_entry *pt_entry; 
    struct lock *pt_lock;
};

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
    return victim;
}

void coremap_create(unsigned int n_pages) {
    npages = n_pages;
    coremap = kmalloc(npages * sizeof(struct cm_entry));
}

int coremap_bootstrap(paddr_t firstpaddr) {
    unsigned int i = 0;
    if (!coremap) 
        return false;
    coremap_lock = lock_create("coremap_lock");
    if (!coremap_lock){
        kfree(coremap);
        return false;
    }
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

static paddr_t get_n_frames(unsigned int num, bool fixed, pt_entry* entry, struct lock* lock) {
    
    paddr_t addr = 0;
    uint32_t i = first_page, residual, page;
    bool found = false;
    lock_acquire(coremap_lock);
    if(coremap == NULL){
        lock_release(coremap_lock);
        return 0;
    }
    while (i < npages && !found) {
        if (!coremap[i].occ && coremap[i].nframes >= num)
            found = true;
        else
            i += coremap[i].nframes;
    }

    if (!found && num != 1) {
        lock_release(coremap_lock);
        return 0;
    } else if (!found) {
        int err = 0;
        unsigned int swp_index;
        i = get_victim();
        if((int)i == -1){
            lock_release(coremap_lock);
            return 0;
        }

        // gestisti la TLB in caso di swap out (invalidation??)
        // (MULTICORE PROB)

    

        // swappa
        if(!lock_do_i_hold(coremap[i].pt_lock)) { //se il frame non e' mappato nella page_table del thread che sta eseguendo il codice
            // devo settare l'entry come in swap, quindi devo acquisire il lock della pt che possiede la entry
            lock_acquire(coremap[i].pt_lock);
            err = swap_set(PADDR_TO_KVADDR(i * PAGE_SIZE), &swp_index);
            if(!err){
                coremap[i].pt_entry->swp = true;
                coremap[i].pt_entry->frame_no = swp_index;
            }
            lock_release(coremap[i].pt_lock);
        } else {
            // eseguo lo swap-out
            err = swap_set(PADDR_TO_KVADDR(i * PAGE_SIZE), &swp_index);
            if(!err){
                coremap[i].pt_entry->swp = true;
                coremap[i].pt_entry->frame_no = swp_index;
            }
        }
        
        if(err) {
            kprintf("%s", strerror(err));
            lock_release(coremap_lock);
            return 0;
        }
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
        coremap[i].fixed = fixed;
        coremap[i].pt_entry = entry;
        coremap[i].pt_lock = lock;
    }
    addr = (paddr_t)(page * PAGE_SIZE);
    bzero((void*)PADDR_TO_KVADDR(addr), PAGE_SIZE);
    lock_release(coremap_lock);
    return addr;    

}

paddr_t get_swappable_frame(pt_entry* entry, struct lock* lock) {
    return get_n_frames(1, false, entry, lock);
}

paddr_t get_kernel_frame(unsigned int num) {
    return get_n_frames(num, true, NULL, NULL);
}

void free_frame(paddr_t addr) {

    uint32_t i, next, page = addr / PAGE_SIZE, mysize;

    //se distruggo la page table già posseggo la coremap_lock
    bool lock_hold = lock_do_i_hold(coremap_lock);
    if(!lock_hold){
        lock_acquire(coremap_lock);
        if(coremap == NULL){
            lock_release(coremap_lock);
            return ;
        }
    }
    
    if(coremap == NULL){
        return;
    }

    mysize = coremap[page].nframes;
    KASSERT(mysize>0);

    for (i = 0; i < mysize; i++) {
        coremap[page + i].occ = false;
        coremap[page + i].fixed = false;
        coremap[page + i].pt_entry = NULL;
        coremap[page + i].pt_lock = NULL;
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
    if(!lock_hold)
        lock_release(coremap_lock);
}


void coremap_destroy() {
    lock_acquire(coremap_lock);
    npages = 0;
    //kfree(coremap); da problemi
    coremap = NULL;    
    first_page = 0;
    lock_release(coremap_lock);
                                    // leak coremap_lock, because the system now will be shutdown
    
}
