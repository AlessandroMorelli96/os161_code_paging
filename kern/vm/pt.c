#include "pt.h"


#if OPT_PT
void pt_create(struct addrspace *as){
	kprintf("pt_create\n");
	as->as_pagetable=kmalloc(sizeof(pagetable));
	KASSERT(as->as_pagetable!=NULL);
	as->as_pt_npages=0;
}
#endif
