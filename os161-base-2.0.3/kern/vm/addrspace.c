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
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>

#if OPT_PAGING
#include <spl.h>
#include <mips/tlb.h>
#include <vfs.h>
#include <vmstats.h>
#endif
/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct addrspace *
as_create(void) {
    struct addrspace *as;

    as = kmalloc(sizeof(struct addrspace));
    if (as == NULL) {
        return NULL;
    }
#if OPT_PAGING

    /*
     * Initialize as needed.
     */
    as->index = 0;
#endif
    return as;
}

int as_copy(struct addrspace *old, struct addrspace **ret) {
    struct addrspace *newas;

    newas = as_create();
    if (newas == NULL) {
        return ENOMEM;
    }

    /*
     * Write this.
     */

    (void)old;

    *ret = newas;
    return 0;
}

void as_destroy(struct addrspace *as) {
    /*
     * Clean up as needed.
     */
#if OPT_PAGING
    if (as == NULL)
         return;
    vfs_close(as->file);
#endif
    kfree(as);
}

void as_activate(void) {
#if OPT_PAGING
    int i, spl;
    struct addrspace *as;

    as = proc_getas();
    if (as == NULL) {
        return;
    }

    /* Disable interrupts on this CPU while frobbing the TLB. */
    spl = splhigh();
    inc_counter(tlb_invalidations);

    for (i = 0; i < NUM_TLB; i++) {
        tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
    }

    splx(spl);
#else
    struct addrspace *as;

    as = proc_getas();
    if (as == NULL) {
        /*
         * Kernel thread without an address space; leave the
         * prior address space in place.
         */
        return;
    }

    /*
     * Write this.
     */
#endif
}

void as_deactivate(void) {
    /*
     * Write this. For many designs it won't need to actually do
     * anything. See proc.c for an explanation of why it (might)
     * be needed.
     */
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
                     int readable, int writable, int executable
                     ) {

#if OPT_PAGING
    if (as->index > N_SEGMENTS) {
        panic("Too many regions!\n");
        return ENOSYS;
    }

    (as->segments[as->index]).p_vaddr = vaddr;
    (as->segments[as->index]).p_memsz = memsize;
    (as->segments[as->index]).readable = readable || 0;
    (as->segments[as->index]).writable = writable || 0;
    (as->segments[as->index]).executable = executable || 0;

    

    return 0;
#else
    /*
	 * Write this.
	 */

	(void)as;
	(void)vaddr;
	(void)memsize;
	(void)readable;
	(void)writeable;
	(void)executable;
	return ENOSYS;
#endif
}

int as_prepare_load(struct addrspace *as) {
    /*
     * Write this.
     */
#if OPT_PAGING
    as->ignore_permissions = 1;
#else
    (void)as;
#endif
    return 0;
}

int as_complete_load(struct addrspace *as) {
    /*
     * Write this.
     */

#if OPT_PAGING
    as->ignore_permissions = 0;
#else
    (void)as;
#endif
    return 0;
}

int as_define_stack(struct addrspace *as, vaddr_t *stackptr) {
    /*
     * Write this.
     */

    (void)as;

    /* Initial user-level stack pointer */
    *stackptr = USERSTACK;

    return 0;
}

#if OPT_PAGING
int load_page(struct addrspace *as, vaddr_t vaddr) {
    int i = 0, err = 0;
    unsigned int first_offset = 0, f_size, m_size;
    for (i = 0; i < N_SEGMENTS; i++) {
        if (vaddr >= as->segments[i].p_vaddr && vaddr < as->segments[i].p_vaddr + as->segments[i].p_memsz)
            break;
    }

    vaddr = vaddr & PAGE_FRAME;


    if( vaddr < as->segments[i].p_vaddr )  // se l'inizio di pagina non appartiene al segmento
        vaddr = as->segments[i].p_vaddr;
    
        
    //  quanta memoria scrivere (al più 4096)
    m_size = PAGE_SIZE - (~PAGE_FRAME & vaddr);


    // il limite del segmento appartiene alla pagina da caricare? 
    if( as->segments[i].p_vaddr + as->segments[i].p_memsz > vaddr && as->segments[i].p_vaddr + as->segments[i].p_memsz < vaddr + m_size ) 
        m_size = (as->segments[i].p_vaddr + as->segments[i].p_memsz) - vaddr;
   
    
    // calcolo offset file

    first_offset = as->segments[i].p_file_start + (vaddr - as->segments[i].p_vaddr);
    
    if ( first_offset >= as->segments[i].p_file_end )
        return 0;

    // calcolo quantità da leggere da file
    f_size = (m_size > as->segments[i].p_file_end - first_offset) ? as->segments[i].p_file_end - first_offset : m_size;
    
    as_prepare_load(as);
    err = load_segment(as, as->file, first_offset, vaddr, m_size, f_size, as->segments[i].executable);
    as_complete_load(as);
    if(err)
        return err;    
    
    return 0;
}
#endif
