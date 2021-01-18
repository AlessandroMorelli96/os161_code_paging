#include "vmstats.h"

#if OPT_VMSTATS
void stampa_statistiche(void){
	if(tlb_fault_free+tlb_fault_replace!=tlb_fault) kprintf("Non corretto");
	//if(tlb_reload+page_fault_disk+page_fault_zero!=tlb_fault) kprintf("Non corretto");
	//if(page_fault_elf+page_fault_swap!=page_fault_disk) kprintf("Non corretto");
	kprintf("Tlb fault: %d\nTlb fault free:%d\nTlb fault replace:%d\nTlb invalidation:%d\nTlb reload:%d\n",tlb_fault,tlb_fault_free,tlb_fault_replace,tlb_invalidation,tlb_reload);
	kprintf("Page falut zero:%d\nPage fault disk:%d\nPage fault ELF:%d\nPage fault swap:%d\n",page_fault_zero,page_fault_disk,page_fault_elf,page_fault_swap);
	kprintf("Page fault write:%d\n",swap_write);
}
#endif
