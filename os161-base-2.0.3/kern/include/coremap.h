#ifndef _COREMAP_H_
#define _COREMAP_H_

#include <types.h>
#include <pt.h>
/**
 * 
 * Lista dei frame liberi, ogni elemento punta al successivo, per ottenere il numero di frame bisogna 
 * efferttuare un right-shift di 12 posizioni
 * 
 */

struct cm_entry;

#define MAX_CONT_PAGES (unsigned int)64

/**
 *
 * Functions:
 *     coremap_bootstrap  - inizializza la lista dei frame liberi
 *
 *     get_frame  -  restituisce l'indirizzo fisico del primo frame libero della lista
 *
 *     free_frame  -  libera un frame e lo aggiunge in testa alla lista
 *
 */


void coremap_create(unsigned int npages);

int coremap_bootstrap(paddr_t firstpaddr);

paddr_t get_swappable_frame(pt_entry* entry);

paddr_t get_kernel_frame(unsigned int num);

void free_frame(paddr_t addr);

void coremap_destroy(void);

#endif