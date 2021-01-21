#ifndef _PT_H_
#define _PT_H_

//#include "opt-pt.h"
#include <types.h>
#include <lib.h>
#include <addrspace.h>

#if OPT_CODE

pagetable * pt_create(void);

int pt_victim(struct addrspace *as);

void pt_destroy(pagetable *);

int pt_add(paddr_t paddr, struct addrspace *as, vaddr_t vaddr);

int pt_copy(pagetable *, pagetable **);

#endif 
#endif /* _PT_H_ */
