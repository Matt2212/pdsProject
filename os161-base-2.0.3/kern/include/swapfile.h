#ifndef _SWAP_FILE_H_
#define _SWAP_FILE_H_

#include <types.h>
#include <vm.h>

struct pt_entry;

#define SWAP_MAX 9 * 1024 * 1024 / PAGE_SIZE

struct swap_file{
    struct vnode* file;     /* Rappresenta il file utilizzato per effettuare lo swap-in o lo swap-out delle pagine*/
    uint8_t refs[SWAP_MAX]; /* Arrai il cui indici rappresentano una porzione del file che può contenere una pagina e i cui elementi rappresentano il contatore di riferimenti a tale pagina. Se 0 la porzione è libera.  */
};

struct lock* swap_lock; /* Utilizzato per sincronizzare gli accessi alla struttura dati*/

/**
 *
 * Functions:
 *     swap_init  -  Inizializza il sistema di swap allocando la struct swap_file e il lock necessario per rendere gli accessi alla struttura sincronizzati. Apre il file di swap. Ritorna 0 se non si verificano errori.
 *
 *     swap_get  -  Legge il blocco allocato nel file in posizione index, decrementa il contatore dei riferimenti e scrive il blocco in memoria all'indirizzo logico del kernel address; se address è NULL il contatore dei riferimenti viene decrementato comunque, ma non avviene alcuna scrittura in memoria; restituisce 0 se non si verificano errori.
 *
 *     swap_set  -  Legge il blocco di memoria all'indirizzo address e lo scrive nel file utilizzando la prima posizione libera, imposta il relativo contatore a 1, ritorna la posizione nel file tramite il parametro index; restituisce 0 se non si verificano errori.
 *
 *     swap_inc_ref - Incrementa il contatore dei riferimenti al frame rappresentato dall'indice index dell'array refs della struct swap_file.
 *
 *     swap_close - Chiude il file di swap e dealloca la struct swap_file.
 *
 *     load_from_swap - Permette di effettuare lo swap-in della pagina descritta nella struct pt_entry.
 */

int swap_init(void);
int swap_get(vaddr_t address, unsigned int index);
int swap_set(vaddr_t address, unsigned int* index);
void swap_inc_ref(unsigned int index);
void swap_close(void);

int load_from_swap(struct pt_entry* entry);

#endif