#ifndef _COREMAP_H_
#define _COREMAP_H_

#include <types.h>
struct pt_entry;
/**
 *
 * Array di cm_entry cioè una struttura dati contenente informazioni riguardanti il relativo frame. Ogni elemento dell'array rappresentra lo stato del corrispettivo frame.
 * ogni frame può essere o libero oppure occupato. Inoltre, solo se il frame è occupato, viene indicato se su esso possa essere effettuato uno swap-out.
 * Nel caso in cui un processo kernel richieda di allocare un blocco contiguo di frame, il primo elemento del blocco contiguo conterrà la dimensione del blocco.
 * Lo stesso ragionamento viene applicato per i blocchi contigui di frame liberi.
 *
 */

struct cm_entry {
    uint32_t occ : 1;      /* indica se il frame sia occupato o meno */
    uint32_t fixed : 1;    /* indica se si possa effettuare swap-out del frame */
    uint32_t nframes : 20; /* quanti frame contigui a questo sono stati allocati o sono liberi */
    struct pt_entry* pt_entry;    /* entry della Page Table che contiene questo frame, tale campo è diverso da NULL se il frame corrispondente appartiene a un address space */
};

/**
 *
 * Funzioni:
 *
 *     coremap_create - Crea una coremap (array di struct cm_entry), di dimensione npages.
 *
 *     coremap_bootstrap - Inizializza la struttura, il parametro firstpaddr rappresenta l'indirizzo fisico dell'inizio del primo frame libero.
 *
 *     get_user_frame -  Restituisce l'indirizzo fisico dell'inizio di un frame libero.
 *                       Nel caso nessun frame sia libero, effettua lo swap-out di un frame vittima. Una volta trovato, il corrispettivo elemento nell’array coremap conterrà il valore descritto dal parametro entry.
 *
 *     get_kernel_frame - Restituisce l'indirizzo fisico dell'inizio del blocco di frame liberi contigui di dimensione num.
 *                        Nel caso in cui il parametro num valga 1, e non vi siano frame liberi, viene effettuato lo swap-out di un frame vittima.
 *
 *     free_frame - Marca il frame o la sequenza di frame contigui, che iniziano dall'indirizzo fisico addr, come liberi. Durante questa fase gli interrupt vengono disabilitati per garantire l’atomicità dell’operazione.
 *
 *     coremap_shutdown - Termina il funzionamento della coremap.
 *
 *     coremap_set_fixed - Imposta il frame rappresentato dall'elemento in posizione index come adatto allo swap-out.
 *
 *     coremap_set_fixed - Imposta il frame rappresentato dall'elemento in posizione index come non adatto allo-swap out.
 *
 */

void coremap_create(unsigned int npages);

bool coremap_bootstrap(paddr_t firstpaddr);

paddr_t get_user_frame(struct pt_entry* entry);

paddr_t get_kernel_frame(unsigned int num);

void free_frame(paddr_t addr);

void coremap_shutdown(void);

void coremap_set_fixed(unsigned int index);

void coremap_set_unfixed(unsigned int index);
#endif