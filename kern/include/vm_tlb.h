#ifndef _VM_TLB_H_
#define _VM_TLB_H_

#include "types.h"
#include "lib.h"

#include "opt-virtual_memory_mng.h"
#include "opt-tlb.h"

//int tlb_get_rr_victim(void);

//int occupate;


int tlb_fault,tlb_fault_free,tlb_fault_replace,tlb_invalidation,tlb_reload; //invalidation,reload da fare
int page_fault_zero,page_fault_disk,page_fault_elf,page_fault_swap;
int swap_write;




int occupati(void);

void vm_bootstrap(void);


/* Allocate/free some kernel-space virtual pages */
vaddr_t alloc_kpages(unsigned npages);

void free_kpages(vaddr_t addr);

int vm_fault(int faulttype, vaddr_t faultaddress);

void dumbvm_can_sleep(void);

void vm_tlbshootdown(const struct tlbshootdown *ts);

paddr_t getppages(unsigned long npages);

int freeppages(paddr_t addr, unsigned long npages);

void tlb_invalidate(paddr_t paddr);
#endif
