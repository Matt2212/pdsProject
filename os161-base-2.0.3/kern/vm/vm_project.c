/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <types.h>

#include <machine/vm.h>
#include <addrspace.h>

#include <coremap.h>
#include <cpu.h>
#include <current.h>
#include <kern/errno.h>
#include <lib.h>
#include <proc.h>
#include <spinlock.h>
#include <spl.h>
#include <vm_tlb.h>
#include <vm.h>

#include <syscall.h>
#include <swapfile.h>

#include <vm_stats.h>
/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 *
 * NOTE: it's been found over the years that students often begin on
 * the VM assignment by copying dumbvm.c and trying to improve it.
 * This is not recommended. dumbvm is (more or less intentionally) not
 * a good design reference. The first recommendation would be: do not
 * look at dumbvm at all. The second recommendation would be: if you
 * do, be sure to review it from the perspective of comparing it to
 * what a VM system is supposed to do, and understanding what corners
 * it's cutting (there are many) and why, and more importantly, how.
 */

/* under dumbvm, always have 72k of user stack */
/* (this must be > 64K so argument blocks of size ARG_MAX will fit) */

/*
 * Wrap ram_stealmem in a spinlock.
 */
static struct spinlock vm_lock = SPINLOCK_INITIALIZER;
static bool init = 0;


void vm_bootstrap(void) {
    int err = 0;
    spinlock_acquire(&vm_lock);
    unsigned int npages = ram_getsize() / PAGE_SIZE;
    spinlock_release(&vm_lock);
    coremap_create(npages);
    spinlock_acquire(&vm_lock);
    if (!coremap_bootstrap(ram_getfirstfree())) {
        panic("vm_bootstrap: No space left on the device to allocate the coremap table\n");
        return;
    }
    init = true;
    spinlock_release(&vm_lock);
    err = swap_init();
    if (err) {
        panic("vm_bootstrap: Error during swap init: %s\n", strerror(err));
        return;
    }
}

static paddr_t
getppages(unsigned long npages) {
    paddr_t addr;
    spinlock_acquire(&vm_lock);
    if (!init) { // dovremmo essere al bootstrap quindi possiamo allocare in maniera contigua
        addr = ram_stealmem(npages);
        spinlock_release(&vm_lock);
    }
    else {
        spinlock_release(&vm_lock);
        addr = get_kernel_frame(npages);
    }
    return addr;
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t
alloc_kpages(unsigned npages) {
    paddr_t pa;

    pa = getppages(npages);
    if (pa == 0) {
        return 0;
    }
    return PADDR_TO_KVADDR(pa);
}

void free_kpages(vaddr_t addr) {
    if (addr == 0) return;
        
    spinlock_acquire(&vm_lock);
    if (init) {
        spinlock_release(&vm_lock);
        free_frame((paddr_t)addr - MIPS_KSEG0);
    }
    else
        spinlock_release(&vm_lock);

}

void vm_tlbshootdown(const struct tlbshootdown *ts) {
    (void)ts;
    panic("vm_project tried to do tlb shootdown?!\n");
}


int vm_fault(int faulttype, vaddr_t faultaddress) {
    paddr_t paddr;
    uint32_t ehi, elo;
    int i, spl, e_fault = 1;
    uint8_t read_only = 0;
    struct addrspace *as;

    spinlock_acquire(&vm_lock);
    if(!init){                                                  
        spinlock_release(&vm_lock);
        thread_exit();
        panic("thread_exit returned (should not happen)\n");
    }
    spinlock_release(&vm_lock);
    if( faultaddress == (vaddr_t) NULL || faultaddress >= (vaddr_t) MIPS_KSEG0){
        kprintf("\nvm_fault: %s\n", strerror(EFAULT));
        sys__exit(EFAULT);
    }
    

    if (curproc == NULL) {
        /*
         * No process. This is probably a kernel fault early
         * in boot. Return EFAULT so as to panic instead of
         * getting into an infinite faulting loop.
         */
        return EFAULT;
    }

    as = proc_getas();
    
    if (as == NULL || !as->active) {
        /*
         * No address space set up. This is probably also a
         * kernel fault early in boot.
         */
        thread_exit();
        panic("vm_fault: thread_exit returned (should not happen)\n");
        return EFAULT;
    }
    
    for(i=0; i<N_SEGMENTS && e_fault; i++){
        if (faultaddress >= as->segments[i].p_vaddr && faultaddress < as->segments[i].p_vaddr + as->segments[i].p_memsz) {
            e_fault = 0;
            read_only = !(as->segments[i].writable);
        }
    }

    if ( e_fault && faultaddress < PROJECT_STACK_MIN_ADDRESS ) {    // outside stack
        kprintf("\nvm_fault: %s\nThe address: %p, is out of the defined memory segments\n", strerror(EFAULT), (void *)faultaddress);
        sys__exit(EFAULT);
    }
    

    DEBUG(DB_VM, "vm_project: fault: 0x%x\n", faultaddress);

    switch (faulttype) {
        case VM_FAULT_READONLY:
            kprintf("\nvm_fault: %s\nAttempt to write into a read-only memory segment: %p\n", strerror(EFAULT), (void *)faultaddress);
            sys__exit(EFAULT);
        case VM_FAULT_READ:
        case VM_FAULT_WRITE:
            break;
        default:
            return EINVAL;
    }
    
    int err = pt_get_frame_from_page(as->page_table, faultaddress, &paddr);
    if (err != 0) {
        kprintf("\nvm_fault: get_frame failed %s\n", strerror(err));
        sys__exit(err);
    }
    faultaddress &= PAGE_FRAME;

    /* make sure it's page-aligned */
    KASSERT((paddr & PAGE_FRAME) == paddr);

    /* Disable interrupts on this CPU while frobbing the TLB. */
    spl = splhigh();

    for (i = 0; i < NUM_TLB; i++) {
        tlb_read(&ehi, &elo, i);
        if (ehi == faultaddress && ((elo & PAGE_FRAME) == paddr)) { // se ho effettuato una load questa entry potrebbe già esiste
            inc_counter(tlb_faults_with_replace);
            goto write;
        }
        if (elo & TLBLO_VALID) {
            continue;
        }
        inc_counter(tlb_faults_with_free);
        goto write;
    }

    i = tlb_get_rr_victim();
    inc_counter(tlb_faults_with_replace);
write:
    inc_counter(tlb_faults);
    ehi = faultaddress;
    if (read_only && !as->ignore_permissions)
        elo = paddr | TLBLO_VALID;
    else
        elo = paddr | TLBLO_DIRTY | TLBLO_VALID; 
    DEBUG(DB_VM, "vm_project: 0x%x -> 0x%x\n", faultaddress, paddr);
    tlb_write(ehi, elo, i);
    if (!as->ignore_permissions) // se sono in fase di load il frame non è swappable
        coremap_set_unfixed(paddr >> 12);
    splx(spl);
    return 0;
}

void vm_shutdown(void){

    swap_close(); 
    spinlock_acquire(&vm_lock);

    if(init){
        coremap_shutdown();
        init = false;

        print_stats();                  // print statistics
    }

    spinlock_release(&vm_lock);
    
}
