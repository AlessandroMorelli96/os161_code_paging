/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <proc.h>
#include <spl.h>
#include <mips/tlb.h>
#include <synch.h>

#include "vm_tlb.h"
#include "pt.h"
#include "swap.h"
#include "vmstats.h"

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

/* under dumbvm, always have 72k of user stack */
/* (this must be > 64K so argument blocks of size ARG_MAX will fit) */
#define DUMBVM_STACKPAGES    18

static struct spinlock spl_id = SPINLOCK_INITIALIZER;

struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}
	
	as->as_text_segment = 0;
	as->as_txt_seg_npages = 0;

#if OPT_CODE	
	as->as_pagetable = pt_create();
	if(as->as_pagetable == NULL){
		kprintf("Errore PT create return NULL\n");
		return NULL;
	}

	as->as_pt_npages = 0;

	int res=swap_create(as);

	if(res)
		return NULL;
	
	as->lk=lock_create("addrresspace");
	spinlock_acquire(&spl_id);
	as->as_active=1;
	spinlock_release(&spl_id);
#endif
	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	(void)old;
	(void)ret;
	struct addrspace *new;

	dumbvm_can_sleep();

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

#if OPT_CODE
	/*
	new->as_pt_npages = old->as_pt_npages;

	if(pt_copy(old->as_pagetable, (&new->as_pagetable))){
		kprintf("Errore copia pagetable");
	}
	*/
#endif
	/* (Mis)use as_prepare_load to allocate some physical memory. */
	if (as_prepare_load(new)) {
		as_destroy(new);
		return ENOMEM;
	}
	*ret = new;

	return 0;
}

void
as_destroy(struct addrspace *as)
{
	dumbvm_can_sleep();
#if OPT_CODE
	pt_destroy(as->as_pagetable);
	as->as_pt_npages = 0;
	kfree(as->as_pagetable);
	swap_destroy(as->pts);
#endif
	kfree(as);
}

void
as_activate(void)
{
	int i, spl;
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		return;
	}
	

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();
#if OPT_CODE
	if(as->as_active==1){	//invalidazione tlb solo una volta appena creato l'addrspace
		for (i=0; i<NUM_TLB; i++) {
			tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
		}
		as->as_active=0; //flag disabilitato
		tlb_invalidation++;
	}
#endif
	splx(spl);

}

void
as_deactivate(void)
{
	/*
	 * Write this. For many designs it won't need to actually do
	 * anything. See proc.c for an explanation of why it (might)
	 * be needed.
	 */
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	size_t npages;

	//dumbvm_can_sleep();

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;
	/* We don't use these - all pages are read-write */
	(void)readable;
	(void)writeable;
	(void)executable;

#if OPT_CODE
	if (as->as_text_segment == 0) { //definiamo lo spazio per i text segment
		as->as_text_segment = vaddr;
		as->as_txt_seg_npages = npages;
		return 0;
	}
	return 0;
#endif
	/*
	 * Support for more than two regions is not available.
	 */


	kprintf("dumbvm: Warning: too many regions\n");
	return ENOSYS;
}

int
as_prepare_load(struct addrspace *as)
{
	
	(void)as;
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	dumbvm_can_sleep();
	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	(void)as;
	*stackptr = USERSTACK;
	return 0;
}

