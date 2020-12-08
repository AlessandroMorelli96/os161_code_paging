#ifndef _PT_H_
#define _PT_H_

#include "opt-pt.h"
#include <types.h>
#include <lib.h>
#include <addrspace.h>

void pt_create(struct addrspace *);

void pt_destroy(struct addrspace *);

int pt_define_region(struct addrspace *, vaddr_t ,size_t, size_t , int , int , int );

int pt_add(paddr_t paddr, struct addrspace *as, vaddr_t vaddr);

#endif /* _PT_H_ */
