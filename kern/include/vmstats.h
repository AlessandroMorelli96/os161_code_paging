#ifndef _VMSTATS_H_
#define _VMSTATS_H_

#include "types.h"
#include "lib.h"

//#include "opt-vmstats.h" 	
#include "opt-code.h"

#if OPT_CODE
int tlb_fault,tlb_fault_free,tlb_fault_replace,tlb_invalidation,tlb_reload; //invalidation,reload da fare
int page_fault_zero,page_fault_disk,page_fault_elf,page_fault_swap;
int swap_write;
#endif

#if OPT_CODE
void stampa_statistiche(void);

void fault(void);
void free(void);
void replace(void);
void invalidation(void);
void reload(void);
void zero(void);
void disk(void);
void elf(void);
void swap(void);
void swapwrite(void);
#endif

#endif
