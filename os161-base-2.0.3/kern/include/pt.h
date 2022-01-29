#ifndef PT_H_
#define PT_H_

#include <types.h>
#include <synch.h>
#define TABLE_SIZE 1024

#define GET_EXT_INDEX(addr) ( addr >> 22 )
#define GET_INT_INDEX(addr) (( addr & 0x3ff000) >> 12)

#define MACRO_PAGE_SIZE 1 << 22

#define PAGE_NOT_FOUND 1

typedef struct /* secondo livello */
{
    unsigned int frame_no : 20; /* indica di quale numero di frame si tratta oppure se swp = 1 indica l’indice (offset nel file swap / 4096) nel quale il frame si trovi all’interno dello swap*/
    unsigned int valid : 1;     /* indica se questa entry è valida (il frame corrispondente è utilizzabile) */
    unsigned int swp : 1;       /* indica se la pagina si trovi nello swap file */
    bool swapping : 1;          /* indica se la pagina sia stata scelta come vittima per lo swap-out */
} pt_entry;

typedef struct /* primo livello */
{
    pt_entry** table;     /* vettore di puntatori a Page Table di secondo livello */
    struct lock* pt_lock; /* permette ad un thread del processo di avere un accesso esclusivo alla Page Table */
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
