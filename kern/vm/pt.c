#include "pt.h"
#include "swap.h"
#include "vmstats.h"
#include <kern/errno.h>

#if OPT_CODE
int
pt_victim(struct addrspace *as)
{
	int victim;
   	static unsigned int next_victim = 0;
    	victim = next_victim;
     	next_victim = (next_victim + 1) % as->as_pt_npages;
     	return victim;
}
#endif

#if OPT_CODE
pagetable *
pt_create(void){

	pagetable * new = kmalloc(sizeof(pagetable));

	if(new == NULL)
		return NULL;

	return new;
}
#endif

#if OPT_CODE
void
pt_destroy(pagetable *pt){

	pagetable *tmp = pt;
	pagetable *old;
	for(;tmp->next!=NULL;){
		old = tmp;
		freeppages(old->pt_paddr,1);
		old->pt_vaddr=0;
		old->pt_paddr=0;
		tmp = (pagetable*) tmp->next;
	}
	freeppages(tmp->pt_paddr,1);

}
#endif

#if OPT_CODE
int
pt_add(paddr_t paddr, struct addrspace *as, vaddr_t vaddr){

	if(as->as_pt_npages==0){ //pagetable vuota
		if(as->as_pagetable == NULL){
			return 1;
		}
		as->as_pt_npages++;
		as->as_pagetable->pt_vaddr = vaddr;
		as->as_pagetable->pt_paddr = paddr;
		as->as_pagetable->next = NULL;
		page_fault_zero++;
		return 0;
	}
	else{
		int i=0;
		if(paddr!=0){  //c'è spazio per aggiungere una pagina in ram
			pagetable *tmp;
			for (tmp = as->as_pagetable; tmp->next != NULL; tmp = (pagetable *) tmp->next, i++) {}
			pagetable *new = kmalloc(sizeof(pagetable));
			if(new == NULL)
				return 1;

			new->pt_vaddr = vaddr;
			new->pt_paddr = paddr;
			new->next = NULL;
			as->as_pt_npages++;
			tmp->next = (struct pagetable *) new;
			page_fault_zero++;
			return 0;
		} else if(as->count_swap<SWAPFILE_NPAGE){	 //swap out di una vecchia pagina (cercando una vittma) per inserire una nuova pagina in ram	
			//ricerca vittima
			int vittima=pt_victim(as);
			pagetable *old=as->as_pagetable;
			for(int i=0;i<vittima;i++)				
				old=(pagetable *)old->next;
			
			if(swap_out(as,old))
				return 1;
			
			if(!freeppages(old->pt_paddr,1)){
				kprintf("Errore freeppages\n");
				return 1;
			}

			paddr=getppages(1);
			if(paddr==0){
				kprintf("Errore getppages\n");
				return 1;
			}

			old->pt_vaddr=vaddr;
			old->pt_paddr=paddr;
			
			return 0;
		}
	}

	panic("Out of swap space\n"); //aggiungo una pagina ma lo swapfile è pieno
	return 1;
}
#endif

#if OPT_CODE
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
