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
#include <addrspace.h>
#include <lib.h>
#include <proc.h>
#include <vm.h>

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

    as->page_table = kmalloc(sizeof(pt));
    if (as == NULL) {
        kfree(as);
        return NULL;
    }

    /*
     * Initialize as needed.
     */
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
    kfree(as->segments);
    pt_destroy(as->page_table);

    kfree(as);
}

void as_activate(void) {
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
                     int readable, int writeable, int executable
                     ) {
    

    static int index = 0;
    if(index > N_SEGMENTS)
        {
            panic("Too many regions!\n");
            return ENOSYS;
        }

    as->segments[index].p_vaddr = vaddr;
    as->segments[index].p_memsz = memsize;
    as->segments[index].readable = readable;
    as->segments[index].writable = writeable;
    as->segments[index].executable = executable;

    



    if(init_rows(as->page_table, (unsigned int) as->segments[index].p_vaddr)) 
        return ENOMEM;

    if  (pt_load_free_frame(as->page_table, (unsigned int) as->segments[index].p_vaddr))
        return ENOMEM;

    

    index++;

    

    return 0;
}

int as_prepare_load(struct addrspace *as) {
    /*
     * Write this.
     */

    (void)as;
    return 0;
}

int as_complete_load(struct addrspace *as) {
    /*
     * Write this.
     */

    (void)as;
    return 0;
}

int as_define_stack(struct addrspace *as, vaddr_t *stackptr) {
    /*
     * Write this.
     */

    as->stack_limit = USERSTACK - PAGE_SIZE;   
    // metti init_rows in un if (per ora torna void)
    if(init_rows(as->page_table, (unsigned int) as->stack_limit))  
        return ENOMEM;              /* vedi se farlo fare direttamente alla risoluzione dell'indirizzo */

    /* Initial user-level stack pointer */
    *stackptr = USERSTACK;

    return 0;
}

#if OPT_PROJECT
int load_page(struct addrspace *as, vaddr_t vaddr){

    int i=0;
    for(i=0; i<N_SEGMENTS ; i++){
         if ( vaddr > as->segments[i].p_vaddr && vaddr < as->segments[i].p_memsz)
            break;
    }
    if( as->segments[i].p_offset < as->segments[i].p_file_end ){
        int amount_to_read = PAGE_SIZE;
        if( PAGE_SIZE > as->segments[i].p_file_end - as->segments[i].p_offset)
            amount_to_read = as->segments[i].p_file_end - as->segments[i].p_offset;

        load_segment(as, as->file , as->segments[i].p_offset, vaddr, PAGE_SIZE,  amount_to_read, as->segments[i].executable);
        as->segments[i].p_offset += amount_to_read;
    }

}
#endif