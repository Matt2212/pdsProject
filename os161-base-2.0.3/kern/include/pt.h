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
    unsigned int frame_no : 20;
    bool valid : 1;
    bool swp : 1; //è in swap
    bool swapping : 1; //è stato scelto come vittima
} pt_entry;

typedef struct
{
    pt_entry **table; 
    struct lock* pt_lock; //per garantire accesso concorrente alla struttura dati da parte dei thread del processo

} pt;

/**
 * Partizionamento indirizzo logico user
 *  
 *  +---------------------------------+-----------------------------------+---------------------+
 *  |                                 |                                   |                     |
 *  | indice page table primo livello | indice page table secondo livello | offset nella pagina |
 *  |                                 |                                   |                     |
 *  +---------------------------------+-----------------------------------+---------------------+
 *  31                                22                                  12                    0
 */

/**
 *
 * Funzioni:
 *     pt_create - Crea la Page Table e la restituisce.
 *
 *     pt_get_frame_from_page  - Trova, mediante la Page Table table, l’indirizzo del frame corrispondente alla pagina che ha come indirizzo logico fault_addr e lo scrive nel parametro frame_addr; se il frame non è presente in memoria lo carica da memoria secondaria tramite la funzione load_frame; ritorna 0 se non vi sono stati errori durante questo processo.
 *
 *     pt_copy - Crea una copia profonda della Page Table old in un'altra già creata e passata tramite il parametro new; ritorna 0 se non vi sono stati errori durante la copia.
 *
 *     pt_destroy  -  Distrugge la page table.
 *
 */

pt* pt_create(void);

int pt_get_frame_from_page(pt* table, vaddr_t addr, paddr_t* frame_addr);

int pt_copy(pt* old, pt* new);

void pt_destroy(pt* table);


#endif
