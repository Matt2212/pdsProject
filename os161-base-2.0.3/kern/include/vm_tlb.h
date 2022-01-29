#ifndef _VM_TLB_H_
#define _VM_TLB_H_

#include <types.h>
#include <machine/tlb.h>

/**
 *
 * Funzioni:
 *
 *     tlb_get_rr_victim - Sceglie una entry della tlb da sostituire mediante una politica Round-Robin e ne restituisce l'indice.
 *
 *     invalidate_entry_by_paddr - Segna come invalida, se esiste, la entry della tlb riferita al frame che contiene l'indirizzo paddr.
 *
 */

int tlb_get_rr_victim(void);

void invalidate_entry_by_paddr(paddr_t paddr);

#endif /* _VM_TLB_H_ */