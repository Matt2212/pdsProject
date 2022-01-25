#include <coremap.h>
#include <pt.h>
#include <swapfile.h>
#include <lib.h>
#include <wchan.h>
#include <proc.h>
#include <addrspace.h>
#include <kern/errno.h>

#include <vm_stats.h>


static struct spinlock spinlock_faults_from_disk = SPINLOCK_INITIALIZER;


pt* pt_create(){
    pt* ret;
    static int id = 0;
    ret = kmalloc(sizeof(pt));
    if(ret == NULL)
        return NULL;
    ret->table = kmalloc(sizeof(pt_entry*) * TABLE_SIZE);
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

void pt_destroy(pt* table){ // puo' essere eseguita una sola volta
    int i = 0;
    if (table == NULL) return;
    spinlock_acquire(&coremap_lock);
    
    while (is_swapping) {
        wchan_sleep(pt_destroy_queue, &coremap_lock);
    }

    for (; i < TABLE_SIZE; i++) {
        if (table->table[i] != NULL) {
            int j = 0;
            for(; j < TABLE_SIZE; j++) 
                if (table->table[i][j].valid && !table->table[i][j].swp)
                    free_frame(table->table[i][j].frame_no << 12); // passo l'indirizzo fisico del frame in quanto la funzione si aspetta questo
        }
    }
    lock_destroy(table->pt_lock); //safe perchè sono l'ultimo thread del processo
    spinlock_release(&coremap_lock);

    for (i = 0; i < TABLE_SIZE; i++) {
        if (table->table[i] != NULL) {
            int j = 0;
            for (; j < TABLE_SIZE; j++) 
                if (table->table[i][j].valid && table->table[i][j].swp)
                    swap_get((vaddr_t)NULL, table->table[i][j].frame_no);  // libera l'entry di tale pagina nello swap
            kfree(table->table[i]);  // dealloco i blocchi utilizzati per contenere e entry
        }
    }

    kfree(table->table);
    kfree(table);
}

static int init_rows(pt* table, unsigned int index) {
    int i = 0;
    index = GET_EXT_INDEX(index);
    

    table->table[index] = kmalloc(sizeof(pt_entry)*TABLE_SIZE);
    if (table->table[index] == NULL) {
        panic("No space left on the device");
        return ENOMEM;
    }
    for(; i < TABLE_SIZE; i++){ // forse inutile
        table->table[index][i].valid = false; // basta invalidare l' entry per far perdere di significato tutto il resto
    }
    return 0;
}

static int load_frame(pt* table, unsigned int exte, unsigned int inte, vaddr_t fault_addr) {
    static struct spinlock spinlock_zeroed_stats = SPINLOCK_INITIALIZER;
    int err = 0;
    table->table[exte][inte].frame_no = get_swappable_frame(&table->table[exte][inte],table->pt_lock) >> 12;
    if (table->table[exte][inte].frame_no == 0)
        return ENOMEM;
    table->table[exte][inte].valid = true;
    if (fault_addr < PROJECT_STACK_MIN_ADDRESS){     //l'indirizzo si trova al di fuori dello stack
        err = load_page(proc_getas(), fault_addr); 
        
        spinlock_acquire(&spinlock_faults_from_disk);
        inc_counter(page_faults_from_elf);
        inc_counter(page_faults_disk);
        spinlock_release(&spinlock_faults_from_disk);
    }
    else{
        spinlock_acquire(&spinlock_zeroed_stats);
        inc_counter(page_faults_zeroed);
        spinlock_release(&spinlock_zeroed_stats);
    }

    return err;
}

int pt_get_frame_from_page(pt* table, vaddr_t fault_addr, paddr_t* frame) {
    unsigned int exte, inte;
    int err = 0;
    exte = GET_EXT_INDEX(fault_addr);
    inte = GET_INT_INDEX(fault_addr);

    static struct spinlock spinlock_reload = SPINLOCK_INITIALIZER;

    //se posseggo il lock significa che sto effettuando o una load o una swap-in, quindi l'entry nella page_table esiste già, quindi goto end

    KASSERT(fault_addr < MIPS_KSEG0);

    if(lock_do_i_hold(table->pt_lock)){
        KASSERT(table->table[exte] != NULL); // ho già una entry di secondo livello
        KASSERT(table->table[exte][inte].valid); //ho già ottenuto un frame
        KASSERT(table->table[exte][inte].frame_no != 0);
        goto end;
    }

    lock_acquire(table->pt_lock);
    // inizializzazione riga di secondo livello
    if (table->table[exte] == NULL)
        err = init_rows(table, fault_addr);
    if (err) {
        lock_release(table->pt_lock);
        return err;
    }
    
    if (table->table[exte][inte].valid == false){
        err = load_frame(table, exte, inte, fault_addr);
    }
    else{
        if (table->table[exte][inte].swp){           // swap
            spinlock_acquire(&spinlock_faults_from_disk);
            inc_counter(page_faults_from_swap);
            inc_counter(page_faults_disk);
            spinlock_release(&spinlock_faults_from_disk);
            err = load_from_swap(&table->table[exte][inte], table->pt_lock);
        }
        else{
            spinlock_acquire(&spinlock_reload);
            inc_counter(tlb_reloads);
            spinlock_release(&spinlock_reload);
        }
    }


    if (err) {
        lock_release(table->pt_lock);
        return err;
    }
    lock_release(table->pt_lock);

end:
    *frame = table->table[exte][inte].frame_no << 12;
    return 0;
}

int pt_copy(pt* old, pt* new) {
    int i = 0;

    //prima della copia rendi i frames non swappabili

    //lock_acquire(old->pt_lock);
    lock_acquire(new->pt_lock);
    for (; i < TABLE_SIZE; i++) {
        if (old->table[i] != NULL) {
            if (init_rows(new, i << 22)) {
                lock_release(old->pt_lock);
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
                        new->table[i][j].frame_no = get_swappable_frame(&new->table[i][j], new->pt_lock) >> 12;
                        if (new->table[i][j].frame_no == 0) {
                            lock_release(old->pt_lock);
                            return ENOMEM;
                        }
                        memcpy((void *) PADDR_TO_KVADDR(new->table[i][j].frame_no << 12), 
                            (void *) PADDR_TO_KVADDR(old->table[i][j].frame_no << 12), 4096);
                    }
                    
                } 
        }
    }
    lock_release(new->pt_lock);
    //lock_release(old->pt_lock);
    return 0;
}