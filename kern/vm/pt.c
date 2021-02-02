#include "pt.h"
#include "swap.h"
#include "vmstats.h"
#include <kern/errno.h>

#if OPT_CODE
pagetable*
pt_victim(struct addrspace *as)
{
	int victim;
   	static unsigned int next_victim = 0;
    	victim = next_victim;
     	next_victim = (next_victim + 1) % as->as_pt_npages;
	pagetable *old=as->as_pagetable;			
	for(int i=0;i<victim;i++){				
		old=(pagetable *)old->next;	//ricerca vittima dato l'indice
	}
     	return old;
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
//static pagetable* inserimento_pagina(pagetable* tmp, vaddr_t vaddr, paddr_t paddr);
#endif

#if OPT_CODE
int
pt_add(paddr_t paddr, struct addrspace *as, vaddr_t vaddr){

	if(as->as_pt_npages==0){ //pagetable vuota
		if(as->as_pagetable == NULL){
			return 1;
		}
		//funzione carina as->as-pagetable = f(as->as_pagetable,vaddr,paddr)
		as->as_pt_npages++;
		as->as_pagetable->pt_vaddr = vaddr;
		as->as_pagetable->pt_paddr = paddr;
		as->as_pagetable->next = NULL;
		return 0;
	}
	else{
		int i=0;
		if(paddr!=0){  //c'è spazio per aggiungere una pagina in ram
			pagetable *tmp;
			for (tmp = as->as_pagetable; tmp->next != NULL; tmp = (pagetable *) tmp->next, i++);
			pagetable *new = kmalloc(sizeof(pagetable));	//aggiungiamo new in coda alla lista
			if(new == NULL)
				return 1;
			//funzione carina (new,vaddr,paddr)
			new->pt_vaddr = vaddr;
			new->pt_paddr = paddr;
			new->next = NULL;
			as->as_pt_npages++;
			tmp->next = (struct pagetable *) new;
			return 0;
		} else {	 //swap out di una vecchia pagina (cercando una vittma) per inserire una nuova pagina in ram	
			//ricerca vittima
			pagetable *old=pt_victim(as);

			//swap out
			if(swap_out(as,old))
				return 1;
			//libera pagina
			if(!freeppages(old->pt_paddr,1)){
				kprintf("Errore freeppages\n");
				return 1;
			}
			//rialloca pagina
			paddr=getppages(1);
			if(paddr==0){
				kprintf("Errore getppages\n");
				return 1;
			}
			//sovrascrivo la vittima nella pagetable
			old->pt_vaddr=vaddr;
			old->pt_paddr=paddr;
			
			return 0;
		}
	}

	panic("I don't how to handle this\n");
	return 1;
}
#endif

#if OPT_CODE
int
pt_copy(pagetable *old, pagetable **ret){
	
	pagetable *new;	
	new = pt_create();
	if(new == NULL){
		return ENOMEM;
	}

	pagetable * tmp = new;
	pagetable * old_tmp = old;
	for(; old_tmp !=NULL; old_tmp = (pagetable *) old_tmp->next, tmp = (pagetable *) tmp->next){
		tmp->pt_vaddr = old_tmp->pt_vaddr;
		tmp->pt_paddr = old_tmp->pt_paddr;
		if(old_tmp->next == NULL)
			tmp->next = NULL;
		else	{
			
				tmp->next = kmalloc(sizeof(pagetable));

			}
	}
	
	*ret = new;
	return 0;
}
#endif
