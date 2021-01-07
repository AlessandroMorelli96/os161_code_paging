#ifndef _SWAP_H_
#define _SWAP_H_

#include "opt-swap.h"
#include "types.h"
#include <lib.h>
#include <vfs.h>
#include <kern/fcntl.h>
#include <addrspace.h>
#include "pt.h"


#define SWAPFILE_SIZE 1024*1024*9 

#define SWAPFILE_NPAGE SWAPFILE_SIZE/PAGE_SIZE

#if OPT_SWAP
int *swap_map;

int swap_init_create(void);

int* swap_create(void);
/*
int swap_in(int fd, userptr_t buf_ptr, size_t size);//int fd,userptr_t p,size_t s);

int swap_out(int fd, userptr_t buf_ptr, size_t size);//int fd,userptr_t p,size_t s);
*/
#endif

#endif

