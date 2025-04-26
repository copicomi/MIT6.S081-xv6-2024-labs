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
void superfree(void *pa);
void* superalloc();

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem, supermem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&supermem.lock, "supermem");
  freerange(end, (void*)PHYSTOP);
}


// 分配 16 个 superpage，其余为 普通 page
void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end - 16 * SUPERPGSIZE; p += PGSIZE) {
    kfree(p);
  }

	p = (char*)SUPERPGROUNDUP((uint64)p);
	for (; p + SUPERPGSIZE <= (char*)pa_end; p += SUPERPGSIZE) {
		superfree(p);
	}
}
// super page 释放
void
superfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % SUPERPGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("superfree");


#ifndef LAB_SYSCALL
  // Fill with junk to catch dangling refs.
  memset(pa, 1, SUPERPGSIZE);
#endif
  
  r = (struct run*)pa;

  acquire(&supermem.lock);
  r->next = supermem.freelist;
  supermem.freelist = r;
  release(&supermem.lock);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");


#ifndef LAB_SYSCALL
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);
#endif
  
  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}


void *
superalloc(void)
{
  struct run *r;

  acquire(&supermem.lock);
  r = supermem.freelist;
  if(r) {
    supermem.freelist = r->next;
  }
  release(&supermem.lock);
#ifndef LAB_SYSCALL
  if(r)
    memset((char*)r, 5, SUPERPGSIZE); // fill with junk
#endif
  return (void*)r;
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
    kmem.freelist = r->next;
  }
  release(&kmem.lock);
#ifndef LAB_SYSCALL
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
#endif
  return (void*)r;
}

