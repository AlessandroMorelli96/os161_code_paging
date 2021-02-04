#include "swap.h"
#include "vmstats.h"
#include <vfs.h>
#include <vnode.h>
#include <uio.h>
#include <addrspace.h>
#include <spl.h>
#include <kern/fcntl.h>

static struct vnode *v;

#if OPT_CODE
int swap_in(struct addrspace *as,pagetable *new,int index){  //leggi da file

	struct iovec iov;
        struct uio u;
        int result;
	off_t offset=index*PAGE_SIZE;

	iov.iov_ubase = (userptr_t)PADDR_TO_KVADDR(new->pt_paddr);
        iov.iov_len = PAGE_SIZE;       // length of the memory space
        u.uio_iov = &iov;
        u.uio_iovcnt = 1;
        u.uio_resid = PAGE_SIZE;          // amount to read from the file
        u.uio_offset = offset;
        u.uio_segflg = UIO_SYSSPACE;
        u.uio_rw = UIO_READ;
        u.uio_space = NULL;

        result = VOP_READ(v, &u);

        if (result) {
		kprintf("ERRORE IN SWAP IN\n");
	        return result;
        }

	//liberiamo pagina nello swap
	as->pts[index].sw_paddr=0;
	as->pts[index].sw_vaddr=0;
	as->as_sw_npages--;

	swap();	//incremento page_fault_swap
	return 0;
}
#endif

#if OPT_CODE
int swap_out(struct addrspace *as, pagetable *old){ //scrivi su file

	struct iovec iov;
        struct uio u;
        int result,spl;
	int i=0;
	if(as->as_sw_npages>=SWAPFILE_NPAGE){
		panic("Out of swap space\n");
	}

	//ricerca pagina libera nello swapfile
	for(i=0;i<SWAPFILE_NPAGE;i++){
		if(as->pts[i].sw_paddr==0)
			break;
	}
	
	off_t offset=i*PAGE_SIZE;

	iov.iov_ubase = (userptr_t)PADDR_TO_KVADDR(old->pt_paddr);
        iov.iov_len = PAGE_SIZE;       // length of the memory space
        u.uio_iov = &iov;
        u.uio_iovcnt = 1;
        u.uio_resid = PAGE_SIZE;          // amount to read from the file
        u.uio_offset = offset;
        u.uio_segflg = UIO_SYSSPACE;
        u.uio_rw = UIO_WRITE;
        u.uio_space = NULL;

        result = VOP_WRITE(v, &u);

        if (result) {
		kprintf("ERRORE IN SWAP OUT\n");
           return result;
        }

	//inserimento nella swap
	as->pts[i].sw_paddr=old->pt_paddr;
	as->pts[i].sw_vaddr=old->pt_vaddr;
	as->as_sw_npages++;

	//invalido la riga nella tlb
	spl = splhigh();
	tlb_invalidate(old->pt_paddr);	
	splx(spl);

	swapwrite();	//incremento swap_write
	return 0;
}
#endif


#if OPT_CODE
int swap_init_create(void){
	int result;	

	result = vfs_open((char*)"emu0:SWAPFILE", O_CREAT | O_RDWR | O_APPEND, 0, &v);

	if (result) {
		kprintf("Errore apertura swapfile %d\n", result);
		return result;
	}

	return result;	
}
#endif

#if OPT_CODE
int swap_create(struct addrspace *as){

	as->as_sw_npages=0;	
	as->pts=kmalloc((SWAPFILE_NPAGE)*sizeof(pagetable_swap));

	if(as->pts==NULL)
		return 1;

	for(int i=0;i<SWAPFILE_NPAGE;i++){
		as->pts[i].sw_paddr=0;
		as->pts[i].sw_vaddr=0;
	}

	return 0;
}
#endif

#if OPT_CODE
void swap_destroy(pagetable_swap *swap){
	kfree(swap);
	return;
}
#endif

#if OPT_CODE
void vfs_close_swap(void){
	vfs_close(v);
	return;
}
#endif








