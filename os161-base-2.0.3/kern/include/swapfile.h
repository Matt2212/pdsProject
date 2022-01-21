#ifndef _SWAP_FILE_H_
#define _SWAP_FILE_H_

#include <bitmap.h>
#include <vm.h>

#include <pt.h>

#define SWAP_MAX 9 * 1024 * 1024 / PAGE_SIZE

typedef struct {
    struct vnode *file;
    struct bitmap *bitmap;
} swap_file;
// bit table che indica se un blocco sia occupato (1) o meno (0)

/**
 *
 * Functions:
 *     swap_init  -  Apre lo swap_file e lo alloca se non esiste, restituisce il vnode ad esso associato.
 * 
 *     swap_get  -  Legge il blocco allocato nel file in posizione index, pone il corrispondente bit della bitmap a 1 e scrive il blocco in memoria all'indirizzo logico del kernel address.
 *     
 *     swap_set  -  Legge il blocco di memoria all'indirizzo address e lo scrive nel file utilizzando la prima posizione libera, ritorna la posizione nel file.
 *     
 *     swap_close - Chiude il file di swap e distrugge la bitmap di gestione.
 *     
 *     load_from_swap - Permette di effettuare lo swap-in della pagina descritta nella struct pt_entry.
 */


int swap_init(void);
int swap_get(vaddr_t address, unsigned int index);
int swap_set(vaddr_t address, unsigned int* index);
void swap_close(void);

int load_from_swap(pt_entry* entry, struct lock* pt_lock);

#endif