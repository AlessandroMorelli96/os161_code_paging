#include "pt.h"
#include "swap.h"
#include "vm_tlb.h"
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

struct spinlock pt_splock;

#if OPT_PT
int
pt_victim(void)
{
	int victim;
   	static unsigned int next_victim = 0;
    	victim = next_victim;
     	next_victim = (next_victim + 1) % (max_pages+SWAPFILE_NPAGE);
     	return victim;
}
#endif

#if OPT_PT
pagetable *
pt_create(void){
	kprintf("pt_create\n");
	pagetable * new = kmalloc(sizeof(pagetable));
	spinlock_init(&pt_splock);
	if(new == NULL){
		return NULL;
	}
	//new->pt_paddr=0;
	//new->pt_vaddr=0;
	return new;
}
#endif

#if OPT_PT
void
pt_destroy(pagetable *pt){
	//kprintf("\tpt_destroy\n");
	spinlock_acquire(&pt_splock);	
	pagetable *tmp = pt;
	pagetable *old;
	for(;tmp->next!=NULL;){
		old = tmp;
		//kprintf("dentro %d\n", (int)(old->pt_paddr)/4096);
		freeppages(old->pt_paddr,1);
		old->pt_vaddr=0;
		old->pt_paddr=0;
		tmp = (pagetable*) tmp->next;
		//kfree(old);
		//old=NULL;
	}
	kfree(tmp);
	spinlock_release(&pt_splock);
}
#endif

#if OPT_PT
int
pt_add(paddr_t paddr, struct addrspace *as, vaddr_t vaddr){
	//spinlock_acquire(&pt_splock);
	//kprintf("pt_add %d \tpaddr: 0x%08x \tvaddr:0x%08x\n",max_pages,paddr,vaddr);	
        //kprintf("[*] pt_add %d\n",as->as_pt_npages+1);
	if(as->as_pt_npages==0){
		//as->as_pagetable= kmalloc(sizeof(pagetable));
		if(as->as_pagetable == NULL){
			//spinlock_release(&pt_splock);
			return 1;
		}

		as->as_in_swapfile[as->as_pt_npages]=0;

		as->as_pt_npages++;
		as->as_pagetable->pt_vaddr = vaddr;
		as->as_pagetable->pt_paddr = paddr;
		as->as_pagetable->next = NULL;
		//spinlock_release(&pt_splock);
		return 0;
	}
	else{
		int i=0;
		pagetable *tmp;
		for (tmp = as->as_pagetable;
			tmp->next != NULL; //&& tmp->pt_paddr == 0;
			tmp = (pagetable *) tmp->next, i++) {}
			//kprintf("%d %d\n",i,max_pages); //////////////////////////////////////////
		if(paddr!=0){ 

			pagetable *new = kmalloc(sizeof(pagetable));
			if(new == NULL){
				//spinlock_release(&pt_splock);
				return 1;
			}

			as->as_in_swapfile[as->as_pt_npages]=0;

			new->pt_vaddr = vaddr;
			new->pt_paddr = paddr;
			new->next = NULL;
			as->as_pt_npages++;
			tmp->next = (struct pagetable *) new;
			//kfree(new);
			//spinlock_release(&pt_splock);
			return 0;
		} else if(i<max_pages+SWAPFILE_NPAGE) //controllo da verificare
			{
			kprintf("\nSWAPPPPPPPPPPPPPPPP v:0x%08x ",vaddr);
			//ricerca vittima e SWAP_OUT 
			//ricerca vittima
			int vittima;			
			do{
				vittima=pt_victim();
			}
			while(as->as_in_swapfile[vittima]!=0);
			pagetable *old=as->as_pagetable;
			for(int i=0;i<vittima;i++){				
				old=(pagetable *)old->next;
			}
			swap_out(as,old);
			if(!freeppages(old->pt_paddr,1)){
				kprintf("Errore freeppages\n");
				return 1;
			}
			paddr=getppages(1);
			if(paddr==0){
				kprintf("Errore getppages\n");
				return 1;
			}			
			kprintf("p:0x%08x ",paddr);
			kprintf("ov:0x%08x ",old->pt_vaddr);
			kprintf("op:0x%08x\n",old->pt_paddr);
			as->as_in_swapfile[vittima]=1;
			//assegnazione nuovo elemento in ram aggiunto in coda
			pagetable *new = kmalloc(sizeof(pagetable));
			as->as_in_swapfile[as->as_pt_npages]=0;
			as->as_pt_npages++;
			new->pt_vaddr = vaddr;
			new->pt_paddr = paddr;
			new->next = NULL;
			tmp->next = (struct pagetable *) new;
			

			//spinlock_release(&pt_splock);
			return 0;
		}
	}
	//spinlock_release(&pt_splock);
	return 1;
}
#endif

#if OPT_PT
int
pt_define_region(pagetable *pt, vaddr_t vaddr, size_t sz, size_t npages, int readable, int writeable, int executable){
	//kprintf("pt_define_region\n");
	//spinlock_acquire(&pt_splock);
	pagetable* tmp = pt;
	int i;
	for(i = 0; i < (int) npages; i++){
		tmp->pt_paddr = 0;
		tmp->pt_vaddr = vaddr + i * PAGE_SIZE;
		//spinlock_release(&pt_splock);
		//Controllo per non allocare una pagina in piu
		if(i!=(int)npages-1)
			tmp->next = kmalloc(sizeof(pagetable));
		else
			tmp->next = NULL;
		 tmp=(pagetable*)tmp->next;
		//spinlock_acquire(&pt_splock);
	}
	
	//kprintf("[*] npages: %d\n", (int)npages);
	//tmp = (pagetable *) pt;
	//while(tmp){
	//	kprintf("[*] tmp vaddr: %08x\n", tmp->pt_vaddr);
	//	tmp = (pagetable *) tmp->next;
	//}
	/*
	(void) pt;
	(void) vaddr;
	(void) sz;
	(void)npages;*/
	(void) readable;
	(void) writeable;
	(void) executable;
	(void) sz;
	//spinlock_release(&pt_splock);
	return 0;
}
#endif

#if OPT_PT
int
pt_copy(pagetable *old, pagetable **ret){
	
	pagetable *new;
	kprintf("pt_copy\n");	
	new = pt_create();
	if(new == NULL){
		return ENOMEM;
	}

	//new->as_pagetable = (pagetable *) kmalloc(sizeof(pagetable));
	pagetable * tmp = new;
	pagetable * old_tmp = old;
	for(; old_tmp !=NULL; old_tmp = (pagetable *) old_tmp->next, tmp = (pagetable *) tmp->next){
		//memmove((void *)PADDR_TO_KVADDR(new->pt_paddr),
		//(const void *)PADDR_TO_KVADDR(old->pt_paddr),
		//PAGE_SIZE);
		tmp->pt_vaddr = old_tmp->pt_vaddr;
		tmp->pt_paddr = old_tmp->pt_paddr;
		if(old_tmp->next == NULL)
			tmp->next = NULL;
		else	{
			
				tmp->next = kmalloc(sizeof(pagetable));

			}
	}
	
	//tmp = (pagetable *) new->as_pagetable;
	//old_tmp = (pagetable *) old->as_pagetable;
	//while(tmp){
		kprintf("[*] tmp vaddr\n");
	//	KASSERT(tmp->pt_paddr != 0);
	//	KASSERT(tmp->pt_vaddr != 0);
	//	tmp = (pagetable *) tmp->next;
	//	old_tmp = (pagetable *) old_tmp->next;
	//}

	*ret = new;
	return 0;
}
#endif

#if OPT_PT
int
pt_prepare_load(pagetable * pt, int npages){
	//kprintf("pt_prepare_load\n");
	spinlock_acquire(&pt_splock);
	pagetable * tmp = pt;
	for(; tmp != NULL; tmp = (pagetable *)tmp->next){
		//kprintf("+++++++++++++++++++++++\n");
		tmp->pt_paddr = getppages(1);
		//kprintf("---------------paddr: 0x%08x\n", tmp->pt_paddr);
	}
	if(pt->pt_paddr == 0){
		spinlock_release(&pt_splock);	
		return ENOMEM;
	}
	(void) npages;
	tmp=pt;
	//kprintf("\t\tSTAMPA PAGE TABLE+++++++++++++++++++++++\n");
	for(; tmp != NULL; tmp = (pagetable *)tmp->next){
		
		//kprintf("\t\tpaddr: 0x%08x vaddr:0x%08x\n", tmp->pt_paddr,tmp->pt_vaddr);
	}/*
	(void)npages;
	(void )pt;*/
	spinlock_release(&pt_splock);
	return 0;
}
#endif
/*
int swap_init(void){
	struct vnode *v;
        int result;	
	kprintf("PRIMA\n");
	result = vfs_open((char*)"emu0:swapfile", O_CREAT | O_RDONLY | O_RDWR , 0, &v);
	kprintf("DOPO\n");
	if (result) {
		kprintf("MOLTO MALE %d\n", result);
		return result;
	}else{
		kprintf("BENE\n");
		//vfs_close(v);
		return result;
	}
}
*/
