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
        table->table[index][i].valid = 0; // basta invalidare l' entry per far perdere di significato tutto il resto
    }
}
