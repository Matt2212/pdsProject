#ifndef PT_H_
#define PT_H_

#include <types.h>
#include <synch.h>
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
} pt_entry;

typedef struct
{
    pt_entry *table[TABLE_SIZE]; // allocala dinamicamente
    struct semaphore* load_sem;

    //spinlock per l'intera table o non serve?
} pt;

// fai i metodi thread safe

//crea la page tabòe
pt* pt_create(void);

int pt_get_frame_from_page(pt* table, vaddr_t addr, paddr_t* frame);  // da richiamre per risolvere mediante la Page Table la corrispondenza tra frame e page

void pt_destroy(pt* table); // distrugge la page table


#endif
