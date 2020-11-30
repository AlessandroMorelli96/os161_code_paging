#include "opt-pt.h"
#include "types.h"
#include "lib.h"
#include "addrspace.h"


void pt_create(struct addrspace *);

void pt_destroy(struct addrspace *);

int pt_define_region(struct addrspace *, vaddr_t ,size_t, size_t , int , int , int );
