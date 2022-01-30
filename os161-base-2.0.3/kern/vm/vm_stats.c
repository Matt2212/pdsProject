#include <types.h>
#include <vm_stats.h>
#include <lib.h>


static unsigned long long counters[10] = {0};

static const char* messages[10] = {
    "tlb_faults                :",
    "tlb_faults_with_free      :",
    "tlb_faults_with_replace   :",
    "tlb_invalidations         :",
    "tlb_reloads               :",
    "page_faults_zeroed        :",
    "page_faults_disk          :",
    "page_faults_from_elf      :",
    "page_faults_from_swap     :",
    "swap_file_writes          :"
};


void inc_counter(unsigned int position){
    KASSERT( position < 10 );
    counters[position]++;

}

void print_stats(void){


    kprintf("\nStatistics:\n\n");


    kprintf("%s %lld\n", messages[tlb_faults],               counters[tlb_faults]);                 /* tlb_faults = tlb_faults_with_free + tlb_faults_with_replace */
    kprintf("%s %lld\n", messages[tlb_faults_with_free],     counters[tlb_faults_with_free]);
    kprintf("%s %lld\n", messages[tlb_faults_with_replace],  counters[tlb_faults_with_replace]);
    kprintf("%s %lld\n", messages[tlb_invalidations],        counters[tlb_invalidations]);
    kprintf("%s %lld\n", messages[tlb_reloads],              counters[tlb_reloads]);                /* tlb_faults = tlb_reloads + page_faults_zeroed + page_faults_disk */
    kprintf("%s %lld\n", messages[page_faults_zeroed],       counters[page_faults_zeroed]);
    kprintf("%s %lld\n", messages[page_faults_disk],         counters[page_faults_disk]);           /* page_faults_disk = page_faults_from_elf + page_faults_from_swap */
    kprintf("%s %lld\n", messages[page_faults_from_elf],     counters[page_faults_from_elf]);
    kprintf("%s %lld\n", messages[page_faults_from_swap],    counters[page_faults_from_swap]);
    kprintf("%s %lld\n", messages[swap_file_writes],         counters[swap_file_writes]);


    if(counters[tlb_faults] != counters[tlb_faults_with_free] + counters[tlb_faults_with_replace]){
        kprintf("Warning the property 'tlb_faults = tlb_faults_with_free + tlb_faults_with_replace' does not hold\n");
    }
    if(counters[tlb_faults] != counters[tlb_reloads] + counters[page_faults_zeroed] + counters[page_faults_disk]){
        kprintf("Warning the property 'tlb_faults = tlb_reloads + page_faults_zeroed + page_faults_disk' does not hold\n");
    }
    if(counters[page_faults_disk] != counters[page_faults_from_elf] + counters[page_faults_from_swap]){
        kprintf("Warning the property 'page_faults_disk = page_faults_from_elf + page_faults_from_swap' does not hold\n");
    }

    kprintf("\n");


}