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
    pt_entry **table; 
    struct lock* pt_lock;

} pt;

/**
 *
 * Functions:
 *     pt_create  -  Crea la page table.
 *
 *     pt_get_frame_from_page  -  Risolvere mediante la Page Table la corrispondenza tra frame e page, ritorna 0 se non vi sono stati errori durante il processo.
 *
 *     pt_destroy  -  Distrugge la page table
 *
 */

pt* pt_create(void);

int pt_get_frame_from_page(pt* table, vaddr_t addr, paddr_t* frame);  

void pt_destroy(pt* table);


#endif
