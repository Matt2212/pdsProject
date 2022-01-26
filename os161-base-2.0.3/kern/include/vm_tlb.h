#ifndef _VM_TLB_H_
#define _VM_TLB_H_

#include <types.h>
#include <machine/tlb.h>


int tlb_get_rr_victim(void);

void invalidate_entry_by_paddr(paddr_t paddr);

#endif /* _VM_TLB_H_ */