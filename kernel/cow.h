#define PTE_COW (1L << 8) // COW mapping indicator

#define RCA_SIZE ((PHYSTOP - KERNBASE) >> 12)

#define PA2RCAI(pa) (((uint64)pa - KERNBASE) >> 12) // physical address to reference count array index
#define RCAI2PA(rcai) (((uint64)rcai << 12) + KERNBASE) // reference count array index to physical address

extern int kpage_refcnt_arr[RCA_SIZE];