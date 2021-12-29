#ifndef _COREMAP_H_
#define _COREMAP_H_

#include <types.h>
/**
 * 
 * Lista dei frame liberi, ogni elemento punta al successivo, per ottenere il numero di frame bisogna 
 * efferttuare un right-shift di 12 posizioni
 * 
 */

struct cm_entry {
    uint8_t occ : 1;
    uint8_t fixed : 1;
    uint8_t nframes : 6; //quanti frame contigui a questo sono stati allocati (max 63)
    //entry *pt_entry;
};

#define MAX_CONT_PAGES 64

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

int coremap_bootstrap(paddr_t lastpaddr, paddr_t firstpaddr);

paddr_t get_n_frames(int num);

void free_frame(paddr_t addr);

#endif