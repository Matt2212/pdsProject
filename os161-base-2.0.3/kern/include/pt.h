#ifndef PT_H
#define PT_H

#include <vm.h>

#define TABLE_SIZE 1024

#define GET_EXT_INDEX(addr) ( addr >> 22)
#define GET_INT_INDEX(addr) (( addr & 0x3f000) >> 12)

#define MACRO_PAGE_SIZE 1 << 22
typedef struct
{
    unsigned int frame_no:20;
    unsigned int valid:1;
    unsigned int swp:1;
    unsigned int read_only:1;
} entry;

typedef struct
{
    entry* table[TABLE_SIZE];
} pt;


static vaddr_t get_victim(pt* table); //indirizzo logico nello user space di una pagina indicata come vittima

void init_rows(pt* table, unsigned int index); //creazione e inizializzazione di un blocco di 1024 entries

paddr_t pt_get_frame_from_page(pt* table, vaddr_t addr, uint8_t read_only); // da richiamre solo su frame correttamente caricati o in swap restituisce l'indice del frame associato alla pagina, il frame potrebbe trovarsi in memoria o im swap

void pt_load_frame_from_swap(pt* table, vaddr_t requested); //effettua lo swap tra un frame in memoria (swap out) e un frame nel file di swap (swap-in)

void pt_load_frame_from_file(pt* table, vaddr_t userptr, vaddr_t frame); //carica un frame da disco e lo scrive nel frame di con logico del kernel addr

bool pt_load_free_frame(pt* table, vaddr_t userptr); //preleva un frame libero dalla lista dei frame liberi. Ritorna true se tutto va bene, false altrimenti

void pt_destroy(pt* table); // distrugge la pagetable



#endif