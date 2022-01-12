#include <coremap.h>
#include <pt.h>
#include <lib.h>
#include <machine/vm.h>
#include <swapfile.h>


#include <addrspace.h>

// TODO add spinlock

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
void pt_destroy(pt* table){
    int i = 0;
    if (table == NULL) return; 
    for(;i < TABLE_SIZE; i++) {
        if (table->table[i] != NULL) {
            int j = 0;
            for(; j < TABLE_SIZE; j++) {
                if (table->table[i][j].valid && !table->table[i][j].swp)
                    free_frame(table->table[i][j].frame_no << 12); // passo l'indirizzo fisico del frame in quanto la funzione si aspetta questo
                /*else if (table->table[i][j].valid && table->table[i][j].swp)
                    swap_get((vaddr_t) NULL, table->table[i][j].frame_no);*/
            }
            kfree(table->table[i]); // dealloco i blocchi utilizzati per contenere e entry
        }
    }
}

int init_rows(pt* table, unsigned int index) {
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


int pt_get_frame_from_page(pt* table, vaddr_t fault_addr, paddr_t* frame) {
    unsigned int exte, inte;
    int err;
    exte = GET_EXT_INDEX(fault_addr);
    inte = GET_INT_INDEX(fault_addr);
    
    if(table->table[exte] == NULL)
        err = init_rows();
    
    if(table->table[exte][inte].valid == false ){
        // carico il frame da file
        table->table[exte][inte].frame_no = get_swappable_frame(table->table[exte][inte]) >> 12;
        if( frame == 0 )
            return ENOMEM; 
        table->table[exte][inte].valid = true;
        err = load_page(proc_getas(), fault_addr, table->table[exte][inte]);
        
    }
    
     

    if(err)
        return err;

    
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
int pt_load_free_frame(pt* table, vaddr_t userptr) {
    unsigned int exte, inte;
    exte = GET_EXT_INDEX(userptr);
    inte = GET_INT_INDEX(userptr);
    if (!(table->table[exte][inte].frame_no = get_swappable_frame(&table->table[exte][inte])))
        return ENOMEM;
    table->table[exte][inte].valid = true;
    return 0;
}