#include "swap.h"

#include <kern/errno.h>
#include <spinlock.h>

#include <copyinout.h>
#include <current.h>
#include <vnode.h>
#include <vfs.h>
#include <limits.h>
#include <uio.h>
#include <proc.h>
#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <clock.h>
#include <syscall.h>
#include <lib.h>
#include <kern/fcntl.h>
#include <addrspace.h>
#include <synch.h>
#include <mips/tlb.h>
#include <spl.h>
 

static struct vnode *v;
//vaddr_t entrypoint, stackptr;

#if OPT_SWAP

int swap_in(struct addrspace *as,pagetable *new,int index){  //leggi da file
	kprintf("\nSWAP_IN ");	
	struct iovec iov;
        struct uio u;
        int result;
	pagetable *tmp=(pagetable *)as->as_swap;	
	int indice_swap=index;
	off_t offset=indice_swap*PAGE_SIZE;

	kprintf("OFFSET:%d ",(int)offset);
	iov.iov_ubase = (userptr_t)new->pt_vaddr;
        iov.iov_len = PAGE_SIZE;       // length of the memory space
        u.uio_iov = &iov;
        u.uio_iovcnt = 1;
        u.uio_resid = PAGE_SIZE;          // amount to read from the file
        u.uio_offset = offset;
        u.uio_segflg = UIO_SYSSPACE;
        u.uio_rw = UIO_READ;
        u.uio_space = NULL;

	lock_release(as->lk);
        result = VOP_READ(v, &u);
	lock_acquire(as->lk);
        if (result) {
           return result;
        }
	for(;tmp->swap_next!=NULL;tmp=(pagetable*)tmp->swap_next){
		indice_swap++;
	}
	kprintf("nv:0x%08x np:0x%08x\n",new->pt_vaddr,new->pt_paddr);
	return 0;
}
#endif

#if OPT_SWAP
int swap_out(struct addrspace *as, pagetable *old){ //scrivi su file

	kprintf("\nSWAP_OUT ");
	struct iovec iov;
        struct uio u;
        int result;
	pagetable *tmp=(pagetable *)as->as_swap;	
	int indice_swap=0;
	if(as->as_swap!=NULL){ 
		indice_swap=1; //dal secondo swap_out in poi c'è già una pagina 
		for(;tmp->swap_next!=NULL;tmp=(pagetable*)tmp->swap_next){
			indice_swap++;
		}
		tmp->swap_next=(struct pagetable *)old;	
	}
	else{
		as->as_swap=old;	
	}
	off_t offset=indice_swap*PAGE_SIZE;
	kprintf("OFFSET:%d\n",(int)offset);

	iov.iov_ubase = (userptr_t)PADDR_TO_KVADDR(old->pt_paddr);
        iov.iov_len = PAGE_SIZE;       // length of the memory space
        u.uio_iov = &iov;
        u.uio_iovcnt = 1;
        u.uio_resid = PAGE_SIZE;          // amount to read from the file
        u.uio_offset = offset;
        u.uio_segflg = UIO_SYSSPACE;
        u.uio_rw = UIO_WRITE;
        u.uio_space = NULL;

	lock_release(as->lk);
        result = VOP_WRITE(v, &u);
	lock_acquire(as->lk);
        if (result) {
		kprintf("prima spero di no\n");
           return result;
        }
	tlb_invalidate(old->pt_paddr);	
	return 0;
	//vop_write
}
#endif


#if OPT_SWAP
int swap_init_create(void){
	int result;	
	kprintf("PRIMA %d \n",SWAPFILE_SIZE);
	result = vfs_open((char*)"emu0:SWAPFILE", O_CREAT | O_RDWR | O_APPEND, 0664, &v);
	kprintf("DOPO %d \n",SWAPFILE_NPAGE);
	if (result) {
		kprintf("MOLTO MALE %d\n", result);
		return result;
	}else{
		
		vfs_close(v);
		return result;
	}
	
}
#endif

int* swap_create(void){
	int* swap;
	kprintf("swap_create\n");
	swap=kmalloc((SWAPFILE_NPAGE+max_pages)*sizeof(int));
	for(int i=0;i<SWAPFILE_NPAGE+max_pages;i++)
		swap[i]=2;
	return swap;
}
