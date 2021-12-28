#ifndef _COREMAP_H_
#define _COREMAP_H_

#include <types.h>

/**
 * 
 * Lista dei frame liberi, ogni elemento punta al successivo, per ottenere il numero di frame bisogna 
 * efferttuare un right-shift di 12 posizioni
 * 
 */

typedef struct {
    free_list* next;
} free_list;

static free_list* frames = NULL;

/**
 *
 * Functions:
 *     coremap_init  - inizializza la lista dei frame liberi
 * 
 *     get_frame  -  restituisce l'indirizzo fisico del primo frame libero della lista
 *     
 *     free_frame  -  libera un frame e lo aggiunge in testa alla lista
 *     
 */

void coremap_init(int n_pages);

paddr_t get_frame();

void free_frame(paddr_t addr);

#endif