// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  /*
  size of page statue is PHYSTOP / PGSIZE not (PHYSTOP-KERBASE)/PGSIZE
  */
  int pagestatus[(PHYSTOP-KERNBASE)/PGSIZE];
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  memset(kmem.pagestatus, 0, sizeof(kmem.pagestatus));
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  acquire(&kmem.lock);
  if (--kmem.pagestatus[PAGE_INDEX((uint64)pa)] <= 0) {
    // printf("curr ref of %d: %d\n", PAGE_INDEX((uint64)pa), kmem.pagestatus[PAGE_INDEX((uint64)pa)]);
    memset(pa, 1, PGSIZE);
    r = (struct run*)pa;
    r->next = kmem.freelist;
    kmem.freelist = r;
  }
  release(&kmem.lock);
  // memset(pa, 1, PGSIZE);

  // r = (struct run*)pa;

  // acquire(&kmem.lock);
  // r->next = kmem.freelist;
  // kmem.freelist = r;
  // release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.pagestatus[PAGE_INDEX((uint64)r)] = 1;
    kmem.freelist = r->next;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void
incref (void* pa)
{
  acquire(&kmem.lock);
  kmem.pagestatus[PAGE_INDEX((uint64)pa)] += 1;
  release(&kmem.lock);
}

int
cowhanlder (pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  uint flags;
  uint64 pa;
  if (!pagetable || va > MAXVA) return -1;
  pte = walk(pagetable, va, 0);
  /*
  we only track User or Valid page here
  */
  if (!pte || !(*pte & PTE_U) || !(*pte & PTE_V))
    return -1;
  pa = PTE2PA(*pte);
  acquire(&kmem.lock);
  if (kmem.pagestatus[PAGE_INDEX((uint64)pa)] == 1) {
    *pte &= ~PTE_C;
    *pte |= PTE_W;
    release(&kmem.lock);
    return 0;
  }
  release(&kmem.lock);
  char* mem;
  flags = PTE_FLAGS(*pte);
  if((mem = kalloc()) == 0) {
    printf ("out of mem\n");
    return -1;
  }
  memmove(mem, (char*)pa, PGSIZE);
  kfree ((char*)pa);
  *pte = PA2PTE(mem) | flags | PTE_W;
  *pte &= ~PTE_C;
  return 0;
}