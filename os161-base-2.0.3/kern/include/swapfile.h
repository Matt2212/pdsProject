#ifndef _SWAP_FILE_H_
#define _SWAP_FILE_H_

#include <bitmap.h>
#include <vm.h>

#define SWAP_MAX 9 * 1024 * 1024 / PAGE_SIZE

typedef struct {
    struct vnode *file;
    struct bitmap *bitmap;
} swap_file;
// bit table che indica se un blocco sia occupato (1) o meno (0)

/**
 *
 * Functions:
 *     swap_init  -  apre lo swap_file e lo alloca se non esiste, restituisce il vnode ad esso associato
 * 
 *     swap_get  -  legge il blocco allocato nel file in posizione index, pone il corrispondente bit della bitmap a 1 e scrive il blocco in memoria all'indirizzo logico del kernel address
 *     
 *     swap_set  -  legge il blocco di memoria all'indirizzo address e lo scrive nel file utilizzando la prima posizione libera, ritorna la posizione nel file
 *     
 *     swap_close - chiude il file di swap e distrugge la bitmap di gestione
 */

static swap_file* swap;

void swap_init();
void swap_get(vaddr_t address, unsigned int index);
int swap_set(vaddr_t address);
void swap_close();

#endif