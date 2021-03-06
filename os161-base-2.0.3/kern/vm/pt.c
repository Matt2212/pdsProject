#include <coremap.h>
#include <pt.h>
#include <swapfile.h>
#include <lib.h>
#include <wchan.h>
#include <proc.h>
#include <addrspace.h>
#include <kern/errno.h>
#include <spl.h>
#include <thread.h>
#include <vm_stats.h>


static struct spinlock spinlock_faults_from_disk = SPINLOCK_INITIALIZER;


struct pt* pt_create(){
    struct pt* ret;
    static int id = 0;
    ret = kmalloc(sizeof(struct pt));
    if(ret == NULL)
        return NULL;
    ret->table = kmalloc(sizeof(struct pt_entry*) * TABLE_SIZE);
    if (ret->table == NULL){
        kfree(ret);
        return NULL;
    }
    char name[16] = "pt_lock";
    snprintf(name, 16, "pt_lock_%d", id++);
    ret->pt_lock = lock_create(name);
    if (ret->pt_lock == NULL) {
        kfree(ret->table);
        kfree(ret);
        return NULL;
    }
    return ret;
}

void pt_destroy(struct pt* table){ 
    int i = 0;
    if (table == NULL) return;

    lock_acquire(swap_lock);
    for (; i < TABLE_SIZE; i++) {
        if (table->table[i] != NULL) {
            int j = 0;
            for(; j < TABLE_SIZE; j++) {
                if (table->table[i][j].valid && !table->table[i][j].swp)
                    free_frame(table->table[i][j].frame_no << 12); 
                if (table->table[i][j].valid && table->table[i][j].swp)
                    swap_get((vaddr_t)NULL, table->table[i][j].frame_no);  // libera l'entry relativa a tale pagina nello swap
            }
            kfree(table->table[i]);  // dealloco i blocchi utilizzati per contenere e entry
        }
    }
    kfree(table->table);
    lock_destroy(table->pt_lock);
    kfree(table);
    lock_release(swap_lock);
}

static int init_rows(struct pt* table, unsigned int index) {
    index = GET_EXT_INDEX(index);
    table->table[index] = kmalloc(sizeof(struct pt_entry)*TABLE_SIZE);
    if (table->table[index] == NULL) {
        kprintf("init_rows: No space left for pt entry creation \n");
        return ENOMEM;
    }
    return 0;
}

static int load_frame(struct pt* table, unsigned int exte, unsigned int inte, vaddr_t fault_addr) {
    static struct spinlock spinlock_zeroed_stats = SPINLOCK_INITIALIZER;
    int err = 0;
    table->table[exte][inte].frame_no = get_user_frame(&table->table[exte][inte]) >> 12;
    if (table->table[exte][inte].frame_no == 0)
        return ENOMEM;
    table->table[exte][inte].valid = true;
    if (fault_addr < PROJECT_STACK_MIN_ADDRESS){ //l'indirizzo si trova al di fuori dello stack ma dentro un segmento valido
        err = load_page(proc_getas(), fault_addr); 
        if (!err) {
        spinlock_acquire(&spinlock_faults_from_disk);
        inc_counter(page_faults_from_elf);
        inc_counter(page_faults_disk);
        spinlock_release(&spinlock_faults_from_disk);
        }
    }
    else{
        spinlock_acquire(&spinlock_zeroed_stats);
        inc_counter(page_faults_zeroed);
        spinlock_release(&spinlock_zeroed_stats);
    }

    return err;
}

int pt_get_frame_from_page(struct pt* table, vaddr_t fault_addr, paddr_t* frame_addr) {
    unsigned int exte, inte;
    int err = 0;
    exte = GET_EXT_INDEX(fault_addr);
    inte = GET_INT_INDEX(fault_addr);
    bool lock_hold = false;


    KASSERT(fault_addr < MIPS_KSEG0);

    if(lock_do_i_hold(table->pt_lock)){ //sono in fase di load
        KASSERT(table->table[exte] != NULL); // ho gi?? una page table di secondo livello
        KASSERT(table->table[exte][inte].valid); //ho gi?? ottenuto un frame
        KASSERT(table->table[exte][inte].frame_no != 0);
        lock_hold = true;
    }

    if (!lock_hold)lock_acquire(table->pt_lock);
    // inizializzazione riga di secondo livello
    if (table->table[exte] == NULL)
        err = init_rows(table, fault_addr);
    if (err) {
        if (!lock_hold) lock_release(table->pt_lock);
        return err;
    }
    
    if (table->table[exte][inte].valid == false) {
        err = load_frame(table, exte, inte, fault_addr);
        if (err) {
            lock_release(table->pt_lock);
            return err;
        }
        //il frame ?? fixed in quanto appena uscito da una load quindi sono sicuro che nessuno abbia effettuato swap-out
        *frame_addr = table->table[exte][inte].frame_no << 12;
        lock_release(table->pt_lock);
        return 0;
    }
    int spl = splhigh();
    while(table->table[exte][inte].swapping) { //busy wait finch?? non termina la procedura di swap-out della pagina
        splx(spl);
        thread_yield();
        spl = splhigh();
    }
    if (table->table[exte][inte].swp) {  // swap-in
        splx(spl);
        err = load_from_swap(&table->table[exte][inte]);
        if (!err) {
            spinlock_acquire(&spinlock_faults_from_disk);
            inc_counter(page_faults_from_swap);
            inc_counter(page_faults_disk);
            spinlock_release(&spinlock_faults_from_disk);
        }
    } else { //imposto il frame come fixed
        coremap_set_fixed(table->table[exte][inte].frame_no);  // da questo momento in poi sino alla scrittura in tlb il frame non ?? swappable
        inc_counter(tlb_reloads);
        splx(spl);
    }

    if (err) {
        if (!lock_hold) lock_release(table->pt_lock);
        return err;
    }
    *frame_addr = table->table[exte][inte].frame_no << 12;
    if (!lock_hold) lock_release(table->pt_lock);
    return 0;
}

int pt_copy(struct pt* old, struct pt* new) {
    int i = 0;
    lock_acquire(old->pt_lock);
    lock_acquire(swap_lock);  
    for (; i < TABLE_SIZE; i++) {
        if (old->table[i] != NULL) {
            if (init_rows(new, i << 22)) {
                lock_release(swap_lock);
                return ENOMEM;
            }
            int j = 0;
            for(; j < TABLE_SIZE; j++) 
                if (old->table[i][j].valid) {
                    new->table[i][j].swp = old->table[i][j].swp;
                    new->table[i][j].valid = old->table[i][j].valid;
                    if (new->table[i][j].swp) {
                        new->table[i][j].frame_no = old->table[i][j].frame_no; 
                        swap_inc_ref(new->table[i][j].frame_no);
                    }
                    else {
                        int spl;
                        new->table[i][j].frame_no = get_user_frame(&new->table[i][j]) >> 12;
                        if (new->table[i][j].frame_no == 0) {
                            lock_release(swap_lock);
                            return ENOMEM;
                        }
                        spl = splhigh();
                        memcpy((void *) PADDR_TO_KVADDR(new->table[i][j].frame_no << 12), 
                            (void *) PADDR_TO_KVADDR(old->table[i][j].frame_no << 12), 4096);
                        coremap_set_unfixed(new->table[i][j].frame_no);
                        splx(spl);
                    }
                } 
        }
    }
    lock_release(swap_lock);
    lock_release(old->pt_lock);
    return 0;
}