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
	//kprintf("SWAP_IN ");	
	struct iovec iov;
        struct uio u;
        int result;
	off_t offset=index*PAGE_SIZE;

	//kprintf("OFFSET:%d ",(int)offset);
	iov.iov_ubase = (userptr_t)PADDR_TO_KVADDR(new->pt_paddr);
        iov.iov_len = PAGE_SIZE;       // length of the memory space
        u.uio_iov = &iov;
        u.uio_iovcnt = 1;
        u.uio_resid = PAGE_SIZE;          // amount to read from the file
        u.uio_offset = offset;
        u.uio_segflg = UIO_SYSSPACE;
        u.uio_rw = UIO_READ;
        u.uio_space = NULL;

	//lock_release(as->lk);
        result = VOP_READ(v, &u);
	//lock_acquire(as->lk);
        if (result) {
           return result;
        }
	/*
	for(;tmp->swap_next!=NULL;tmp=(pagetable*)tmp->swap_next){
		indice_swap++;
	}
	*/
	//kprintf("nv:0x%08x np:0x%08x\n",new->pt_vaddr,new->pt_paddr);
	//liberiamo pagina nello swap
	as->pts[index].sw_paddr=0;
	as->pts[index].sw_vaddr=0;
	as->count_swap--;
	return 0;
}
#endif

#if OPT_SWAP
int swap_out(struct addrspace *as, pagetable *old){ //scrivi su file

	struct iovec iov;
        struct uio u;
        int result,spl;
	int i=0;
	if(as->count_swap>=SWAPFILE_NPAGE){
		panic("ERRORE SWAPFILE PIENO");
	}

	//ricerca pagina libera nello swapfile
	for(i=0;i<SWAPFILE_NPAGE;i++){
		if(as->pts[i].sw_paddr==0)
			break;
	}
	
	off_t offset=i*PAGE_SIZE;
	//kprintf("OFFSET:%d\n",(int)offset);

	iov.iov_ubase = (userptr_t)PADDR_TO_KVADDR(old->pt_paddr);
        iov.iov_len = PAGE_SIZE;       // length of the memory space
        u.uio_iov = &iov;
        u.uio_iovcnt = 1;
        u.uio_resid = PAGE_SIZE;          // amount to read from the file
        u.uio_offset = offset;
        u.uio_segflg = UIO_SYSSPACE;
        u.uio_rw = UIO_WRITE;
        u.uio_space = NULL;

	//lock_release(as->lk);
        result = VOP_WRITE(v, &u);
	//lock_acquire(as->lk);
        if (result) {
		kprintf("prima spero di no\n");
           return result;
        }

	//inserimento nella swap
	as->pts[i].sw_paddr=old->pt_paddr;
	as->pts[i].sw_vaddr=old->pt_vaddr;
	as->count_swap++;

	//invalido la riga nella tlb
	spl = splhigh();
	tlb_invalidate(old->pt_paddr);	
	splx(spl);
	swap_write++;
	return 0;
}
#endif


#if OPT_SWAP
int swap_init_create(void){
	int result;	
	kprintf("PRIMA %d \n",SWAPFILE_SIZE);
	result = vfs_open((char*)"emu0:SWAPFILE", O_CREAT | O_RDWR | O_APPEND, 0, &v);
	kprintf("DOPO %d \n",SWAPFILE_NPAGE);
	if (result) {
		kprintf("MOLTO MALE %d\n", result);
		return result;
	}else{
		
		//vfs_close(v);
		return result;
	}
	
}
#endif

int swap_create(struct addrspace *as){
	//int* swap;
	//kprintf("swap_create\n");
	as->count_swap=0;
	/*swap=kmalloc((SWAPFILE_NPAGE+max_pages)*sizeof(int));
	if(swap==NULL)
		return 1;
	for(int i=0;i<SWAPFILE_NPAGE+max_pages;i++)
		swap[i]=2;
	*/
	as->pts=kmalloc((SWAPFILE_NPAGE)*sizeof(pagetable_swap));
	if(as->pts==NULL)
		return 1;
	for(int i=0;i<SWAPFILE_NPAGE;i++){
		as->pts[i].sw_paddr=0;
		as->pts[i].sw_vaddr=0;
	}
	//as->as_in_swap=swap;
	return 0;
}

void swap_destroy(pagetable_swap *swap){
	kfree(swap);
	return;
}










