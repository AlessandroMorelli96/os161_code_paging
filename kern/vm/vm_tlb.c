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

#include "vm_tlb.h"
#include "pt.h"

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

#if OPT_TLB
static
int
tlb_get_rr_victim(void)
{
	int victim;
   	static unsigned int next_victim = 0;
    	victim = next_victim;
     	next_victim = (next_victim + 1) % NUM_TLB;
     	return victim;
}
#endif

void
vm_bootstrap(void)
{	
#if OPT_VIRTUAL_MEMORY_MNG
	int i;
	pagineTotali = (int) (ram_getsize() / PAGE_SIZE); // = 256
	
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
	// Stampa 80 e basta
	//kprintf("dopo \n");

	//for(i = 0; i < pagineTotali; i++){
	//	kprintf("%d", (int)freeRamFrames[i]);
	//}
	//kprintf("\n");
	//for(i = 0; i < pagineTotali; i++){
	//	kprintf("%d", (int)allocsize[i]);
	//}
	//kprintf("\n");
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
void
dumbvm_can_sleep(void)
{
	if (CURCPU_EXISTS()) {
		/* must not hold spinlocks */
		KASSERT(curcpu->c_spinlocks == 0);

		/* must not be in an interrupt handler */
		KASSERT(curthread->t_in_interrupt == 0);
	}
}

#if OPT_VIRTUAL_MEMORY_MNG
static
paddr_t
getfirstfit(unsigned long npages){
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

paddr_t
getppages(unsigned long npages)
{
	paddr_t addr;

	spinlock_acquire(&stealmem_lock);
	kprintf("getppages npages %d\n", (int)npages);

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
		//kprintf("get ppages is Table Active\n");

		//for(int i = 0; i < pagineTotali; i++){
		//	kprintf("%d", (int)freeRamFrames[i]);
		//}
		//kprintf("\n");
	//	for(int i = 0; i < pagineTotali; i++){
	//		kprintf("%d", (int)allocsize[i]);
	//	}
	//	kprintf("\n");
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
vaddr_t
alloc_kpages(unsigned npages)
{
	paddr_t pa;
	kprintf("alloc kpages is Table Active: %d\n", isTableActive());
	if(isTableActive()){
		//kprintf("Pagina: %d\n", npages);
		//for(int i = 0; i < pagineTotali; i++){
		//	kprintf("%d", (int)allocsize[i]);
		//}
		//kprintf("\n");
		//for(int i = 0; i < pagineTotali; i++){
		//	kprintf("%d", (int)freeRamFrames[i]);
		//}
		//kprintf("\n");
	}
		
	dumbvm_can_sleep();
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void
free_kpages(vaddr_t addr)
{
#if OPT_VIRTUAL_MEMORY_MNG
	kprintf("free kpages is Table Active: %d\n", isTableActive());
	if(isTableActive()){
		int npagina = (int)((addr - MIPS_KSEG0)/PAGE_SIZE);
		int i;
		/* nothing - leak the memory. */
		for(i = npagina; i<allocsize[npagina]+npagina; i++){
			freeRamFrames[i] = 1;
		}
		allocsize[npagina] = 0;

		//for(i = 0; i < pagineTotali; i++){
		//	kprintf("%d", (int)allocsize[i]);
		//}
		//kprintf("\n");
		//for(i = 0; i < pagineTotali; i++){
		//	kprintf("%d", (int)freeRamFrames[i]);
		//}
		//kprintf("\n");


	}
#endif
	/* nothing - leak the memory. */
	(void)addr;
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	int i;
	uint32_t ehi, elo;
	struct addrspace *as;
	int spl;
	//kprintf("Vm fault\n");
	//kprintf("faultaddress: 0x%08x\n", faultaddress);

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
		panic("dumbvm: got VM_FAULT_READONLY\n");
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		kprintf("[*] #################\n");
		return EINVAL;
	}

	if (curproc == NULL) {
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		kprintf("[*] --------------------- \n");
		return EFAULT;
	}

	as = proc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		kprintf("[*] ++++++++++++++++++ as\n");
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
	KASSERT(as->as_vbase1 != 0);
	KASSERT(as->as_pbase1 != 0);
	KASSERT(as->as_npages1 != 0);
	KASSERT(as->as_vbase2 != 0);
	KASSERT(as->as_pbase2 != 0);
	KASSERT(as->as_npages2 != 0);
	KASSERT(as->as_stackpbase != 0);
	KASSERT((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	KASSERT((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	KASSERT((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	KASSERT((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
	KASSERT((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;

	if (faultaddress >= vbase1 && faultaddress < vtop1) {
		paddr = (faultaddress - vbase1) + as->as_pbase1;
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
		paddr = (faultaddress - vbase2) + as->as_pbase2;
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) {
		paddr = (faultaddress - stackbase) + as->as_stackpbase;
	}
	else {
		kprintf("[*] *************** 0x%08x\n", faultaddress);
		return EFAULT;
	}

	
#if OPT_PT
	if(faultaddress==4206592)
	kprintf("V--------------------------m fault 0x%08x\n", faultaddress);
/*
	if(isTableActive() && (faulttype == VM_FAULT_READ || faulttype == VM_FAULT_WRITE)){
		//kprintf("[*] return 0x%08x\n", faultaddress);
		//kprintf("[*] paddr 0x%08x\n",paddr);
		pagetable* tmp;
		for(tmp = as->as_pagetable; tmp != NULL && tmp->pt_vaddr != faultaddress; tmp = (pagetable *) tmp->next){}
		if(tmp == NULL){
			kprintf("vm_fault NON trovato: \tvaddr:0x%08x\tas_pt_napges:%d+1\n", faultaddress,as->as_pt_npages);
			paddr = getppages(1);
		//	//int p = getppages(1);
		//	//kprintf("[*] paddr 0x%08x\n",paddr);
			if(pt_add(paddr, as, faultaddress)){
				kprintf("PT not found\n");
			}
		} else {
			//kprintf("vm_fault trovato: \tvaddr: 0x%08x \ttmp->paddr:0x%08x\n", tmp->pt_vaddr,tmp->pt_paddr);
			paddr = tmp->pt_paddr;
		}
	}
*/
#endif
	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();
	
	//kprintf("[5] TLB 0x%08x %d\n", faultaddress, faulttype);
	for (i=0; i<NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		tlb_write(ehi, elo, i);
		//kprintf("Stampa TLB\n");
		for (int l=0; l<NUM_TLB; l++) {
			tlb_read(&ehi, &elo, l);
			if (elo & TLBLO_VALID) {
				//kprintf("ehi 0x%08x ", ehi);
			//	kprintf("elo 0x%08x\n", elo);
				continue;
			}
		}
		splx(spl);
		return 0;
	}
	kprintf("******************* %d\n",i);	
#if OPT_TLB
	kprintf("[6] tlb replace %d\n",i);	
	i = tlb_get_rr_victim();
	ehi = faultaddress;
	elo = paddr | TLBLO_VALID | TLBLO_DIRTY;
	tlb_write(ehi, elo, i);
	splx(spl);
	return 0;
#else
	kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
	splx(spl);
	return EFAULT;
#endif
}
