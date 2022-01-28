#include <vm_tlb.h>
#include <vm.h>
#include <spl.h>
int tlb_get_rr_victim(void) {
    int victim;
    static unsigned int next_victim = 0;
    victim = next_victim;
    next_victim = (next_victim + 1) % NUM_TLB;
    return victim;
}

void invalidate_entry_by_paddr(paddr_t paddr) {
    int i;

    for (i = 0; i < NUM_TLB; i++) {
        uint32_t ehi, elo;
        tlb_read(&ehi, &elo, i);
        if (paddr == elo && ~PAGE_FRAME) {
            tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
            break;
        }
    }
}