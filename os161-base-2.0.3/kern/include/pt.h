#ifndef PT_H_
#define PT_H_

#include <types.h>
#include <spinlock.h>
#define TABLE_SIZE 1024

#define GET_EXT_INDEX(addr) ( addr >> 22 )
#define GET_INT_INDEX(addr) (( addr & 0x3ff000) >> 12)

#define MACRO_PAGE_SIZE 1 << 22

#define PAGE_NOT_FOUND 1



typedef struct
{
    unsigned int frame_no:20;
    unsigned int valid:1;
    unsigned int swp:1;
    //struct spinlock lock;
} pt_entry;

typedef struct
{
    pt_entry *table[TABLE_SIZE];
    //spinlock per l'intera table o non serve?
} pt;

// fai i metodi thread safe

//int init_rows(pt* table, unsigned int index); //creazione e inizializzazione di un blocco di 1024 entries

int pt_get_frame_from_page(pt* table, vaddr_t addr, paddr_t* frame); // da richiamre per risolvere mediante la Page Table la corrispondenza tra frame e page
#if 0
void pt_load_frame_from_swap(pt* table, vaddr_t requested); //effettua lo swap tra un frame in memoria (swap out) e un frame nel file di swap (swap-in)

void pt_load_frame_from_file(pt* table, vaddr_t userptr, vaddr_t frame); //carica un frame da disco e lo scrive nel frame di con logico del kernel addr
#endif
//int pt_load_free_frame(pt* table, vaddr_t userptr); //preleva un frame libero dalla lista dei frame liberi. Ritorna true se tutto va bene, false altrimenti

void pt_destroy(pt* table); // distrugge la pagetable


#endif
