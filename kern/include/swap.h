#ifndef _SWAP_H_
#define _SWAP_H_

#include "opt-swap.h"
#include "types.h"
#include <lib.h>
#include "pt.h"


#define SWAPFILE_SIZE 1024*1024*9

#define SWAPFILE_NPAGE SWAPFILE_SIZE/PAGE_SIZE

#if OPT_SWAP
int *swap_map;

int swap_init_create(void);

int swap_create(struct addrspace *);

int swap_in(struct addrspace *as, pagetable *,int);

int swap_out(struct addrspace* , pagetable *);//int fd,userptr_t p,size_t s);

void swap_destroy(pagetable_swap *);

void vfs_close_swap(void);

#endif

#endif

