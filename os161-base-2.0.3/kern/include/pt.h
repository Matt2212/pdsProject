#ifndef PT_H
#define PT_H

#include <types.h>

typedef struct entry
{
    unsigned int frame_no:20;
    unsigned int valid:1;
    unsigned int swp:1;
    unsigned int read_only:1;
};

typedef struct pt
{
    entry* table[1024];
};


static unsigned int get_victim(pt* table);

void init_row(pt* table, unsigned int index); //inizializza un blocco di 1024 entries

unsigned int pt_get_frame_from_page(pt* table, vaddr_t addr); //restituisce l'indice del frame associato alla pagina, il frame potrebbe trovarsi in memoira o im swap

void pt_load_frame_from_swap(pt* table, vaddr_t frame); //effettua lo swap tra un frame in memoria (swap out) e un frame nel file di swap (swap-in)

void pt_load_frame_from_file(pt* table, vaddr_t userptr,vaddr_t frame); //carica un frame da disco e lo scrive nel frame di con logico del kernel addr

void pt_destroy(pt* table);

static int choose_victim();

#endif