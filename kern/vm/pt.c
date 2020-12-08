#include "pt.h"

#if OPT_PT
void pt_create(struct addrspace *as){
	kprintf("pt_create\n");
	as->as_pagetable=kmalloc(sizeof(pagetable));
	KASSERT(as->as_pagetable!=NULL);
	as->as_pt_npages=0;
}
#endif

#if OPT_PT
void pt_destroy(struct addrspace *as){
	kprintf("pt_destroy\n");
	(void)as;
/*	pagetable *tmp=as->as_pagetable;
	pagetable *old;
	for(;tmp->next!=NULL;){
		kprintf("dentro\n");
		old = tmp;
		old->pt_vaddr=0;
		old->pt_paddr=0;
		tmp = (pagetable*) tmp->next;
		kfree(old);
		old=NULL;
	}
	kfree(as->as_pagetable);
	as->as_pagetable=NULL;
	as->as_pt_npages=0;
*/
}
#endif

#if OPT_PT
int
pt_add(paddr_t paddr, struct addrspace *as, vaddr_t vaddr){
	pagetable *tmp;
	
	//kprintf("[*] pt_add\n");

	//kprintf("[*] paddr: 0x%8x\n", paddr);
	//kprintf("[*] vaddr: 0x%08x\n", vaddr);

	for (tmp = as->as_pagetable;
		tmp != NULL && tmp->pt_vaddr != vaddr; //&& tmp->pt_paddr == 0;
		tmp = (pagetable *) tmp->next) {

		//kprintf("[*] tmp->pt_vaddr: 0x%08x", tmp->pt_vaddr);
		//kprintf(" tmp->pt_paddr: 0x%08x\n", tmp->pt_paddr);
	}

	if(tmp != NULL){
		tmp->pt_paddr = paddr;
		return 1;
	} else {
		return 0;
	}
}
#endif

#if OPT_PT
int
pt_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz, size_t npages, int readable, int writeable, int executable){
	kprintf("pt_define_region\n");
	as->as_pt_npages=(int)npages;
	
	pagetable* tmp = as->as_pagetable;
	int i;
	for(i = 0; i < (int) npages; i++){
		tmp->pt_paddr = 0;
		tmp->pt_vaddr = vaddr + i * PAGE_SIZE;
		//Controllo per non allocare una pagina in piu
		if(i!=(int)npages-1)
			tmp->next = kmalloc(sizeof(pagetable));
		else
			tmp->next = NULL;
		 tmp=(pagetable*)tmp->next;
	}
	
	kprintf("[*] npages: %d\n", (int)npages);
	tmp = (pagetable *) as->as_pagetable;
	while(tmp){
		kprintf("[*] tmp vaddr: %08x\n", tmp->pt_vaddr);
		tmp = (pagetable *) tmp->next;
	}
	(void) readable;
	(void) writeable;
	(void) executable;
	(void) sz;
	return 0;
}
#endif


