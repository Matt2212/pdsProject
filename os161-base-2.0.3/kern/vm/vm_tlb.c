#include <cpu.h>
#include <current.h>
#include <kern/errno.h>
#include <mips/tlb.h>
#include <proc.h>
#include <spl.h>
#include <types.h>
#include <vm.h>

int vm_fault(int faulttype, vaddr_t faultaddress) {
    vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
    paddr_t paddr;
    int i;
    uint32_t ehi, elo;
    int spl;
    uint8_t read_only;
    struct addrspace *as;

    faultaddress &= PAGE_FRAME;

    DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

    if (curproc == NULL) {
        /*
         * No process. This is probably a kernel fault early
         * in boot. Return EFAULT so as to panic instead of
         * getting into an infinite faulting loop.
         */
        return EFAULT;
    }

    as = proc_getas();

    if (as == NULL) {
        /*
         * No address space set up. This is probably also a
         * kernel fault early in boot.
         */
        return EFAULT;
    }

    switch (faulttype) {
        case VM_FAULT_READONLY:
            as_destroy(as);
            /* thread exits. proc data structure will be lost */
            thread_exit();
            panic("thread_exit returned (should not happen)\n");
        case VM_FAULT_READ:
        case VM_FAULT_WRITE:
            break;
        default:
            return EINVAL;
    }

    paddr = pt_get_frame_from_page(curproc->p_table, faultaddress, &read_only);

    /* make sure it's page-aligned */
    KASSERT((paddr & PAGE_FRAME) == paddr);

    /* Disable interrupts on this CPU while frobbing the TLB. */
    spl = splhigh();

    for (i = 0; i < NUM_TLB; i++) {
        tlb_read(&ehi, &elo, i);
        if (elo & TLBLO_VALID) {
            continue;
        }
        ehi = faultaddress;
        elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
        DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
        tlb_write(ehi, elo, i);
        splx(spl);
        return 0;
    }

    i = tlb_get_rr_victim();
    ehi = faultaddress;
    if (read_only)
        elo = paddr | TLBLO_VALID;
    else
        elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
    DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
    tlb_write(ehi, elo, i);
    splx(spl);
    return 0;
}

int tlb_get_rr_victim(void) {
    int victim;
    static unsigned int next_victim = 0;
    victim = next_victim;
    next_victim = (next_victim + 1) % NUM_TLB;
    return victim;
}