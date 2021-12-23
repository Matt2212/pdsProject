#include <pt.h>
#include <coremap.h>
#include <swapfile.h>

void pt_destroy(pt* table){
    int i = 0;
    for(;i < TABLE_SIZE; i++) {
        if (table->table[i] != NULL) {
            int j = 0;
            for(; j < TABLE_SIZE; j++) {
                if (table->table[i][j].valid && !table->table[i][j].swp)
                    free_frame(table->table[i][j].frame_no << 12); // passo l'indirizzo fisico del frame in quanto la funzione si aspetta questo
                else if (table->table[i][j].valid && table->table[i][j].swp)
                    swap_get(NULL, table->table[i][j].frame_no);
            }
            kfree(table[i]); // dealloco i blocchi utilizzati per contenere e entry
        }
    }
    kfree(table);
}

void init_rows(pt* table, unsigned int index) {
    int i = 0;
    table->table[index] = kmalloc(sizeof(entry)*TABLE_SIZE);
    if (table->table[index] == NULL) {
        panic("No space left on the device");
    }
    for(; i < TABLE_SIZE; i++){ // forse inutile
        table->table[index][i].valid = false; // basta invalidare l' entry per far perdere di significato tutto il resto
    }
}

// ritorna 0 in caso di errori
unsigned int pt_get_frame_from_page(pt* table, vaddr_t addr) {
    unsigned int exte, inte;
    exte = GET_EXT_INDEX(addr);
    inte = GET_INT_INDEX(addr);
    if (table->table[exte] == NULL || !table->table[exte][inte].valid) return 0;
    if (table->table[exte][inte].swp){
        // TODO chied se meglio prendere le pagine libere per questo swapout
        pt_load_frame_from_swap(table, addr);
    }

    return table->table[exte][inte].frame_no;
}

void pt_load_frame_from_swap(pt* table, vaddr_t requested) {
    unsigned int exte, inte, vic_int, vic_ext;
    exte = GET_EXT_INDEX(requested);
    inte = GET_INT_INDEX(requested);
    vaddr_t victim = get_victim(table);
    vic_int = GET_INT_INDEX(victim);  
    vic_ext = GET_EXT_INDEX(victim);
    paddr_t frame_victim = table->table[vic_ext][vic_int].frame_no * PAGE_SIZE; //indirizzo fisco del frame vittima
    table->table[vic_ext][vic_int].frame_no = swap_set(victim);
    table->table[vic_ext][vic_int].swp = true;
    swap_get(PADDR_TO_KVADDR(frame_victim), table->table[exte][inte].frame_no); // scrivo sul frame victim il contenuto del frame in posizione frame_no presente nel file di swap
    table->table[exte][inte].frame_no = frame_victim / PAGE_SIZE;
    table->table[exte][inte].swp = false;
}