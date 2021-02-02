#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>

#include <synch.h>
#include "vm_tlb.h"
#include "pt.h"
#include "swap.h"
#include "vmstats.h"
#include "syscall.h"

/* under dumbvm, always have 72k of user stack */
/* (this must be > 64K so argument blocks of size ARG_MAX will fit) */
#define DUMBVM_STACKPAGES    18


static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
static struct spinlock freemem_lock = SPINLOCK_INITIALIZER;

static int allocTableActive = 0;
static int * freeRamFrames;
static int * allocsize;
static int pagineTotali = 0;

static int isTableActive(){
	int active;
	spinlock_acquire(&freemem_lock);
	active = allocTableActive;
	spinlock_release(&freemem_lock);
	return active;
}

#if OPT_CODE
static int tlb_get_rr_victim(void) {	//ricerca vittima nella tlb
	int victim;
   	static unsigned int next_victim = 0;
    	victim = next_victim;
     	next_victim = (next_victim + 1) % NUM_TLB;
     	return victim;
}
#endif

#if OPT_CODE
void tlb_invalidate(paddr_t paddr) {	//invalidazione entry tlb dato il paddr
	uint32_t ehi,elo,i;
	for (i=0; i<NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if ((elo & 0xfffff000) == (paddr & 0xfffff000))	{
			tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);		
		}
	}
}
#endif

void
vm_bootstrap(void) {	
#if OPT_VIRTUAL_MEMORY_MNG
	int i;
	pagineTotali = (int) (ram_getsize() / PAGE_SIZE);
	
	freeRamFrames = (int*) kmalloc(pagineTotali * sizeof(int));
	allocsize = (int*) kmalloc(pagineTotali * sizeof(int));
	
	if (freeRamFrames == NULL || allocsize == NULL)
		return;

	// 0 occupato, 1 libero
	int firstfreepage = (int) (ram_stealmem(0) / PAGE_SIZE);
	for(i = 0; i < firstfreepage; i++){
		freeRamFrames[i] = 0;
		allocsize[i] = 0;
	}
	allocsize[0] = firstfreepage;
	for(; i < pagineTotali; i++){
		freeRamFrames[i] = 1;
		allocsize[i] = 0;
	}

#if OPT_CODE
	int result=swap_init_create();	//creazione e apertura swapfile
	if(result){
		kprintf("ERRORE CREAZIONE SWAPFILE\n");
		return;
	}
	//inizializzazione contatori fault
	tlb_fault=tlb_fault_free=tlb_fault_replace=tlb_invalidation=tlb_reload=0;
	page_fault_zero=page_fault_disk=page_fault_elf=page_fault_swap=0;
	swap_write=0;
#endif
	spinlock_acquire(&freemem_lock);
	allocTableActive = 1;
	spinlock_release(&freemem_lock);
#endif
}

/*
 * Check if we're in a context that can sleep. While most of the
 * operations in dumbvm don't in fact sleep, in a real VM system many
 * of them would. In those, assert that sleeping is ok. This helps
 * avoid the situation where syscall-layer code that works ok with
 * dumbvm starts blowing up during the VM assignment.
 */
void dumbvm_can_sleep(void) {
	if (CURCPU_EXISTS()) {
		/* must not hold spinlocks */
		KASSERT(curcpu->c_spinlocks == 0);

		/* must not be in an interrupt handler */
		KASSERT(curthread->t_in_interrupt == 0);
	}
}

#if OPT_VIRTUAL_MEMORY_MNG
static paddr_t getfirstfit(unsigned long npages){
	int i = 0;
	int l = 0;
	int isPossible = 1;
	while (i < pagineTotali){
		if (allocsize[i]){
			i+=allocsize[i];
		}else {
			// segnare con positivi gli occupati e con negativi quelli liberi in allocsize
			for (isPossible=1, l=i; l < (int) npages+i && isPossible; l++){
				if(allocsize[l]) isPossible = 0;	
			}

			if (isPossible){
				return i * PAGE_SIZE;
			}
		}
	}
	
	return 0;

}
#endif

paddr_t getppages(unsigned long npages) {

	paddr_t addr;
	spinlock_acquire(&stealmem_lock);

#if OPT_VIRTUAL_MEMORY_MNG
	if(isTableActive()){
		addr = getfirstfit(npages);
		if(addr){
			int i;
			int firstPage = addr / PAGE_SIZE;
			allocsize[firstPage] = npages;
			for (i = firstPage; i < firstPage + (int)npages; i++){
				freeRamFrames[i] = 0;
			}

		}
		int tot=0;
		for(int i = 0; i < pagineTotali; i++){
			if(freeRamFrames[i]==0)
				tot++;
	
		}
	} else{
		addr = ram_stealmem(npages);
	}
#else
	addr = ram_stealmem(npages);
#endif
	spinlock_release(&stealmem_lock);
	return addr;
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t alloc_kpages(unsigned npages) {

	paddr_t pa;
		
	dumbvm_can_sleep();

	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	if(isTableActive()){
		int i=0;
		int c=0;
		for(;i<pagineTotali;i++){
			if(freeRamFrames[i]==1)
				c++;
		}
	}
	return PADDR_TO_KVADDR(pa);
}


int freeppages(paddr_t addr, unsigned long npages){
  long i, first, np=(long)npages;	

  if (!isTableActive()) return 0; 
  first = addr/PAGE_SIZE;
  KASSERT(allocsize!=NULL);
  KASSERT(pagineTotali>first);

  spinlock_acquire(&freemem_lock);
  for (i=first; i<first+np; i++) {
    freeRamFrames[i] = (unsigned char)1;
  }
  allocsize[first] = 0;
  spinlock_release(&freemem_lock);

  return 1;
}

void free_kpages(vaddr_t addr) {
#if OPT_VIRTUAL_MEMORY_MNG
	if(isTableActive()){
		paddr_t paddr = addr - MIPS_KSEG0;
		long first = paddr/PAGE_SIZE;	
		KASSERT(allocsize!=NULL);
		KASSERT(pagineTotali>first);
		freeppages(paddr, allocsize[first]);
	}
#endif
	/* nothing - leak the memory. */
	(void)addr;
}

#if OPT_CODE
void vm_tlbshootdown(const struct tlbshootdown *ts) {
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}
#endif

#if OPT_CODE
void vfs_close_swap_wrap(void) {
	vfs_close_swap();
}
#endif

#if OPT_CODE
static pagetable* ricerca_pagina(struct addrspace *as, vaddr_t faultaddress, int *indice_sw){
	pagetable* tmp=as->as_pagetable;
	
	for(;tmp != NULL && tmp->pt_vaddr != faultaddress; tmp = (pagetable *) tmp->next); //ricerca nella pagetable
	if(tmp==NULL && as->as_pt_npages<pagineTotali){		//se non trova nella pagetable cerco nello swapfile solo se la pagetable è piena
		for(int i=0;i<SWAPFILE_NPAGE;i++)		//ricerca nel vettore dello swapfile
			if(as->pts[i].sw_vaddr==faultaddress) { 
				*indice_sw=i;
				break;
			}	
	}
	return tmp;
}
#endif


#if OPT_CODE
static void stack_or_elf(vaddr_t faultaddress){
	vaddr_t stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;		//stack segment base
	vaddr_t stacktop = USERSTACK;						//stack segment top
	if (faultaddress >= stackbase && faultaddress < stacktop) {
		zero();	//incremento page_fault_zeroed
	}
	else{
		elf();	//incremento page_fault_elf
		disk();	//incremento page_fault_disk
	}
}
#endif

int vm_fault(int faulttype, vaddr_t faultaddress) {
	
	paddr_t paddr;
	int i;
	uint32_t ehi, elo;
	struct addrspace *as;
	int spl;
	faultaddress &= PAGE_FRAME;
	
	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		//readonly memory exception con terminazione processo
		sys__exit(ROME);
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		return EINVAL;
	}

	if (curproc == NULL) {
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	as = proc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
#if OPT_CODE
	vaddr_t txt_base = as->as_text_segment;					//text segment base
	vaddr_t txt_top = txt_base + as->as_txt_seg_npages * PAGE_SIZE;		//text segment top
	int indice_sw=-1;

	if(isTableActive() ){
		pagetable* tmp=as->as_pagetable;

		if(as->as_pt_npages!=0){	//se la pagetable non è vuota si cerca la pagina 
			lock_acquire(as->lk);
			tmp = ricerca_pagina(as,faultaddress,&indice_sw);
			lock_release(as->lk);
		}

		if(as->as_pt_npages==0 || (indice_sw==-1 && tmp == NULL)){	//pagina nuova da inserire nella pagetable
			lock_acquire(as->lk);
			paddr = getppages(1);					//se la pagetable è piena, getppages ritorna 0
			if(pt_add(paddr, as, faultaddress)){			//aggiunta pagina nella pagetable
				kprintf("PT non trovata\n");
				lock_release(as->lk);
				return EFAULT;
			}

			if(paddr==0){						//recupero nella pagetable paddr inserito da pt_add
				tmp=(pagetable *) as->as_pagetable;
				for(; tmp->pt_vaddr!= faultaddress && tmp!=NULL; tmp = (pagetable *) tmp->next);	//ricerca del faultaddress
				if(tmp->pt_vaddr==faultaddress) 							//dato il faultaddress, recupero il paddr
					paddr=tmp->pt_paddr;
				else{ 
					lock_release(as->lk);
					return EFAULT;
				}
			}

			stack_or_elf(faultaddress);	//verifica quale valore incrementare e incremento

			lock_release(as->lk);
		} 		
		else if(indice_sw!=-1){		// pagina trovata nello swapfile, quindi: swap out e swap in
			lock_acquire(as->lk);
			//SWAP OUT
			pagetable *old=pt_victim(as);
			
			if(swap_out(as,old)){	//swapout vittima
				return EFAULT;
			} 
			//SWAP IN
			if(swap_in(as,old,indice_sw)) //swap in pagina ricercata dato l'indice
				return EFAULT;

			old->pt_vaddr=faultaddress;	//sostituzione della vittima con il nuovo vaddr nella pagetable

			paddr = old->pt_paddr;		//recupero paddr
			
			disk();	//incremento page_fault_disk
			lock_release(as->lk);
		}
		else if (tmp!=NULL){	//pagina già nella pagetable
			reload();	//incremento tlb_reload
			paddr = tmp->pt_paddr;
		}
	}
	
#endif

	/* make sure it's page-aligned */
	
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();
	
	for (i=0; i<NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
#if OPT_CODE
			if (ehi >= txt_base && ehi < txt_top && (elo & TLBLO_DIRTY)) { //i segmenti di testo vengono settati READONLY, con un controllo per non settarlo più volte inutilmente
				elo = elo & ~TLBLO_DIRTY;
				tlb_write(ehi, elo, i);
			}
#endif	
			continue;
		}
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
#if OPT_CODE
		fault(); 	//incremento tlb_fault
		free();		//incremento tlb_fault_free
#endif
		tlb_write(ehi, elo, i);
		splx(spl);	
		return 0;
	}
	
#if OPT_CODE
	i = tlb_get_rr_victim();
	ehi = faultaddress;
	elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
	if (faultaddress >= txt_base && faultaddress < txt_base) { //solo se è un text segment viene riscritto elo
		elo = paddr | TLBLO_VALID;
	}

	fault(); 	//incremento tlb_fault
	replace();	//incremento tlb_fault_replace
	tlb_write(ehi, elo, i);
	splx(spl);
	return 0;
#else
	kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
	splx(spl);
	return EFAULT;
#endif
}
