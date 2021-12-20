#ifndef PT_H
#define PT_H

#include <types.h>
#define TABLE_SIZE 1024

#define GET_EXT_INDEX(vaddr_t addr) (addr >> 22)
#define GET_INT_INDEX(vaddr_t addr) ((addr & 0x3f000) >> 12)
typedef struct
{
    unsigned int frame_no:20;
    unsigned int valid:1;
    unsigned int swp:1;
    unsigned int read_only:1;
} entry;

typedef struct
{
    entry* table[TABLE_SIZE];
} pt;


static unsigned int get_victim(pt* table);

void init_rows(pt* table, unsigned int index); //creazione e inizializza un blocco di 1024 entries

unsigned int pt_get_frame_from_page(pt* table, vaddr_t addr); //restituisce l'indice del frame associato alla pagina, il frame potrebbe trovarsi in memoira o im swap

void pt_load_frame_from_swap(pt* table, vaddr_t frame); //effettua lo swap tra un frame in memoria (swap out) e un frame nel file di swap (swap-in)

void pt_load_frame_from_file(pt* table, vaddr_t userptr,vaddr_t frame); //carica un frame da disco e lo scrive nel frame di con logico del kernel addr

void pt_load_free_frame(pt* table, vaddr_t userptr)

void pt_destroy(pt* table); // distrugge la pagetable



#endif