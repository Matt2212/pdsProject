#include <coremap.h>
#include <pt.h>
#include <swapfile.h>
#include <lib.h>
#include <proc.h>
#include <addrspace.h>
#include <kern/errno.h>

#if 0
vaddr_t get_victim(pt* table ) {
    static vaddr_t victim = 0;
    vaddr_t ret = victim;
    int count = 0;

    while ( table->table[GET_EXT_INDEX(ret)] != NULL && count < 1024 ) {
        ret += MACRO_PAGE_SIZE;
    }

    if (count >= 1024) return PAGE_NOT_FOUND; // ritorno un indirizzo che non può rappresentare una pagina

    while ( !table->table[GET_EXT_INDEX(ret)][GET_INT_INDEX(ret)].valid || 
            table->table[GET_EXT_INDEX(ret)][GET_INT_INDEX(ret)].swp ) {
        ret += PAGE_SIZE;
        if ( ret == victim ) return PAGE_NOT_FOUND;
        while ( table->table[GET_EXT_INDEX(ret)] != NULL )
            ret += MACRO_PAGE_SIZE;
    }

    victim = ret;
    return victim;
}
#endif

pt* pt_create(){
    pt* ret;
    ret = kmalloc(sizeof(pt));
    if(ret == NULL)
        return NULL;
    ret->table = kmalloc(sizeof(pt_entry*) * TABLE_SIZE);
    if (ret->table == NULL){
        kfree(ret);
        return NULL;
    }
    ret->pt_lock = lock_create("pt_lock");
    if (ret->pt_lock == NULL) {
        kfree(ret->table);
        kfree(ret);
        return NULL;
    }
    return ret;
}

void pt_destroy(pt* table){
    int i = 0;
    if (table == NULL) return;
    lock_destroy(table->pt_lock);
    for (; i < TABLE_SIZE; i++) {
        if (table->table[i] != NULL) {
            int j = 0;
            for(; j < TABLE_SIZE; j++) {
                if (table->table[i][j].valid && !table->table[i][j].swp)
                    free_frame(table->table[i][j].frame_no << 12); // passo l'indirizzo fisico del frame in quanto la funzione si aspetta questo
                else if (table->table[i][j].valid && table->table[i][j].swp)
                    swap_get((vaddr_t) NULL, table->table[i][j].frame_no);
            }
            kfree(table->table[i]); // dealloco i blocchi utilizzati per contenere e entry
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

static int load_from_file(pt* table, unsigned int exte, unsigned int inte, vaddr_t fault_addr) {
    int err = 0;
    table->table[exte][inte].frame_no = get_swappable_frame(&table->table[exte][inte]) >> 12;
    if (table->table[exte][inte].frame_no == 0)
        return ENOMEM;
    table->table[exte][inte].valid = true;
    if (fault_addr < PROJECT_STACK_MIN_ADDRESS)     //l'indirizzo si trova al di fuori dello stack
        err = load_page(proc_getas(), fault_addr); 
    return err;
}

int pt_get_frame_from_page(pt* table, vaddr_t fault_addr, paddr_t* frame) {
    unsigned int exte, inte;
    int err = 0;
    exte = GET_EXT_INDEX(fault_addr);
    inte = GET_INT_INDEX(fault_addr);

    //se posseggo il lock significa che sto effettuando o una load o una swap-in, quindi l'entry nella page_table esiste già, quindi goto end

    if(lock_do_i_hold(table->pt_lock)){
        KASSERT(table->table[exte] != NULL);
        KASSERT(table->table[exte][inte].valid);
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
    
    if (table->table[exte][inte].valid == false)
        err = load_from_file(table, exte, inte, fault_addr);
    // else if (table->table[exte][inte].swp) // swap
    if (err) {
        lock_release(table->pt_lock);
        return err;
    }
    lock_release(table->pt_lock);

end:
    *frame = table->table[exte][inte].frame_no << 12;
    return 0;
}

#if 0
void pt_load_frame_from_swap(pt* table, vaddr_t requested) {
    unsigned int exte, inte, vic_int, vic_ext;
    exte = GET_EXT_INDEX(requested);
    inte = GET_INT_INDEX(requested);
    vaddr_t victim = get_victim(table);
    if (victim == PAGE_NOT_FOUND)
        panic("no space ");
    vic_int = GET_INT_INDEX(victim);  
    vic_ext = GET_EXT_INDEX(victim);
    paddr_t frame_victim = table->table[vic_ext][vic_int].frame_no * PAGE_SIZE; //indirizzo fisco del frame vittima
    table->table[vic_ext][vic_int].frame_no = swap_set(victim);
    table->table[vic_ext][vic_int].swp = true;
    swap_get(PADDR_TO_KVADDR(frame_victim), table->table[exte][inte].frame_no); // scrivo sul frame victim il contenuto del frame in posizione frame_no presente nel file di swap
    table->table[exte][inte].frame_no = frame_victim / PAGE_SIZE;
    table->table[exte][inte].swp = false;
}
#endif
