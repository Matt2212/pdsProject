#ifndef _VMSTATS_H_
#define _VMSTATS_H_


#define tlb_faults                  0               /* tlb_faults = tlb_faults_with_free + tlb_faults_with_replace */
#define tlb_faults_with_free        1
#define tlb_faults_with_replace     2
#define tlb_invalidations           3
#define tlb_reloads                 4               /* tlb_faults = tlb_reloads + page_faults_zeroed + page_faults_disk */
#define page_faults_zeroed          5
#define page_faults_disk            6               /* page_faults_disk = page_faults_from_elf + page_faults_from_swap */
#define page_faults_from_elf        7
#define page_faults_from_swap       8
#define swap_file_writes            9



void inc_counter(unsigned int position);

void print_stats(void);


#endif  /*  _VMSTATS_H_ */