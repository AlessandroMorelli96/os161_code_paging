#ifndef _VM_TLB_H_
#define _VM_TLB_H_

#include "types.h"
#include "lib.h"

#include "opt-virtual_memory_mng.h"
//#include "opt-tlb.h" 	
#include "opt-code.h"


void vm_bootstrap(void);

/* Allocate/free some kernel-space virtual pages */
vaddr_t alloc_kpages(unsigned npages);

void free_kpages(vaddr_t addr);

int vm_fault(int faulttype, vaddr_t faultaddress);

void dumbvm_can_sleep(void);

void vm_tlbshootdown(const struct tlbshootdown *ts);

paddr_t getppages(unsigned long npages);

int freeppages(paddr_t addr, unsigned long npages);

#if OPT_CODE
void tlb_invalidate(paddr_t paddr);

void vfs_close_swap_wrap(void);
#endif

#endif
