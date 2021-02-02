#include "vmstats.h"

#if OPT_CODE
void stampa_statistiche(void){
	if(tlb_fault_free+tlb_fault_replace!=tlb_fault) 
		kprintf("Non corretto\n");
	
	kprintf("Tlb fault: %d\nTlb fault free:%d\nTlb fault replace:%d\nTlb invalidation:%d\nTlb reload:%d\n",tlb_fault,tlb_fault_free,tlb_fault_replace,tlb_invalidation,tlb_reload);
	
	if(tlb_reload+page_fault_disk+page_fault_zero!=tlb_fault) 
		kprintf("Non corretto\n");
	
	kprintf("Page falut zero:%d\nPage fault disk:%d\nPage fault ELF:%d\nPage fault swap:%d\n",page_fault_zero,page_fault_disk,page_fault_elf,page_fault_swap);	

	if(page_fault_elf+page_fault_swap!=page_fault_disk) 
		kprintf("Non corretto\n");

	kprintf("Page fault write:%d\n",swap_write);
}
#endif

#if OPT_CODE
void fault(){
	tlb_fault++;
}
#endif

#if OPT_CODE
void free(){
	tlb_fault_free++;
}
#endif

#if OPT_CODE
void replace(){
	tlb_fault_replace++;
}
#endif

#if OPT_CODE
void invalidation(){
	tlb_invalidation++;
}
#endif

#if OPT_CODE
void reload(){
	tlb_reload++;
}
#endif

#if OPT_CODE
void zero(){
	page_fault_zero++;
}
#endif

#if OPT_CODE
void disk(){
	page_fault_disk++;
}
#endif

#if OPT_CODE
void elf(){
	page_fault_elf++;
}
#endif

#if OPT_CODE
void swap(){
	page_fault_swap++;
}
#endif

#if OPT_CODE
void swapwrite(){
	swap_write++;
}
#endif
