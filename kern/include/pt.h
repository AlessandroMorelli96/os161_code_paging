#ifndef _PT_H_
#define _PT_H_

#include "opt-pt.h"
#include <types.h>
#include <lib.h>
#include <addrspace.h>

#if OPT_PT

pagetable * pt_create(void);

void pt_destroy(pagetable *);

int pt_define_region(pagetable *, vaddr_t ,size_t, size_t , int , int , int );

int pt_add(paddr_t paddr, struct addrspace *as, vaddr_t vaddr);

int pt_copy(pagetable *, pagetable **);

int pt_prepare_load(pagetable *, int);

//int swap_init(void);

#endif 
#endif /* _PT_H_ */
