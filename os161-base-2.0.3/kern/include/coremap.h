#ifndef _COREMAP_H_
#define _COREMAP_H_

#include <types.h>

/**
 * 
 * Lista dei frame liberi, ogni elemento punta al successivo, per ottenere il numero di frame bisogna 
 * efferttuare un right-shift di 12 posizioni
 * 
 */

struct free_list{
    struct free_list* next;
};

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

void coremap_init(void);

paddr_t get_frame(void);

void free_frame(paddr_t addr);

#endif