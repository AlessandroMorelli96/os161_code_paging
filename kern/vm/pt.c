#include "pt.h"
#include <kern/errno.h>


#if OPT_PT
pagetable *
pt_create(void){
	kprintf("pt_create\n");

	pagetable * new = kmalloc(sizeof(pagetable));
	
	if(new == NULL){
		return NULL;
	}
	
	return new;
}
#endif

#if OPT_PT
void
pt_destroy(pagetable *pt){
	kprintf("pt_destroy\n");
	pagetable *tmp = pt;
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
	//kfree(as->as_pagetable);
}
#endif

#if OPT_PT
int
pt_add(paddr_t paddr, struct addrspace *as, vaddr_t vaddr){
	pagetable *tmp;
	kprintf("pt_add\n");	
	
	//kprintf("[*] pt_add\n");

	//kprintf("[*] paddr: 0x%8x\n", paddr);
	//kprintf("[*] vaddr: 0x%08x\n", vaddr);
	int i=0;
	for (tmp = as->as_pagetable;
		tmp->next != NULL; //&& tmp->pt_paddr == 0;
		tmp = (pagetable *) tmp->next, i++) {

		//kprintf("[*] tmp->pt_vaddr: 0x%08x", tmp->pt_vaddr);
		//kprintf(" tmp->pt_paddr: 0x%08x\n", tmp->pt_paddr);
	}
	
	if(i<max_pages){
		pagetable *new = kmalloc(sizeof(pagetable));
		if(new == NULL)
			return 1;
		new->pt_vaddr = vaddr;
		new->pt_paddr = paddr;
		new->next = NULL;
		as->as_pt_npages++;
		tmp->next = (struct pagetable *) new;
		return 0;
	} else
		return 1;
}
#endif

#if OPT_PT
int
pt_define_region(pagetable *pt, vaddr_t vaddr, size_t sz, size_t npages, int readable, int writeable, int executable){
	kprintf("pt_define_region\n");
	
	pagetable* tmp = pt;
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
	
	//kprintf("[*] npages: %d\n", (int)npages);
	//tmp = (pagetable *) pt;
	//while(tmp){
	//	kprintf("[*] tmp vaddr: %08x\n", tmp->pt_vaddr);
	//	tmp = (pagetable *) tmp->next;
	//}
	(void) readable;
	(void) writeable;
	(void) executable;
	(void) sz;
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
		tmp->pt_paddr = old_tmp->pt_paddr;
		tmp->pt_vaddr = old_tmp->pt_vaddr;
		if(old_tmp->next == NULL)
			tmp->next = NULL;
		else
			tmp->next = kmalloc(sizeof(pagetable));
	}
	
	//tmp = (pagetable *) new->as_pagetable;
	//old_tmp = (pagetable *) old->as_pagetable;
	//while(tmp){
	//	kprintf("[*] tmp vaddr:\t %08x %08x\n", old_tmp->pt_vaddr, tmp->pt_vaddr);
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
	kprintf("pt_prepare_load\n");
	pagetable * tmp = pt;
	for(; tmp != NULL; tmp = (pagetable *)tmp->next){
		tmp->pt_paddr = getppages(1);
		kprintf("paddr: 0x%08x\n", tmp->pt_paddr);
	}
	if(pt->pt_paddr == 0)
		return ENOMEM;
	(void) npages;
	return 0;
}
#endif
