// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13
#define NBUF_PER_BUCKET ((NBUF + 12) / 13)

#define HASH(x) ((x)%(NBUCKET))

struct {
  struct spinlock lock;
  struct buf buf[NBUF_PER_BUCKET];
} bcache[NBUCKET];
// 哈希桶维护缓存区

struct spinlock block;

void
binit(void)
{
  	initlock(&block, "b");

  for (int i = 0; i < NBUCKET; ++ i) {
  	initlock(&bcache[i].lock, "bcache");
	for (int j = 0; j < NBUF_PER_BUCKET; j ++) {
  		struct buf *b = &bcache[i].buf[j];
    	initsleeplock(&b->lock, "buffer");
  	}
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
	acquire(&block);


	int hash_id = HASH(blockno);

	acquire(&bcache[hash_id].lock);
	  for (int j = 0; j < NBUF_PER_BUCKET; j ++) {
  			struct buf *b = &bcache[hash_id].buf[j];
    		if(b->dev == dev && b->blockno == blockno){
      			b->refcnt++;
				release(&bcache[hash_id].lock);
	release(&block);
      			acquiresleep(&b->lock);
      			return b;
    		}
	  }
	release(&bcache[hash_id].lock);


	for (int i = 0; i < NBUCKET; i ++) {
		if (i == hash_id) continue;
		acquire(&bcache[i].lock);

	  	for (int j = 0; j < NBUF_PER_BUCKET; j ++) {
  			struct buf *b = &bcache[i].buf[j];
    		if (b->refcnt == 0) {
      			b->dev = dev;
      			b->blockno = blockno;
      			b->valid = 0;
      			b->refcnt = 1;
      			release(&bcache[i].lock);
	release(&block);
      			acquiresleep(&b->lock);
      			return b;
    		}
	  	}

		release(&bcache[i].lock);
	}

	acquire(&bcache[hash_id].lock);

  			struct buf *b = &bcache[hash_id].buf[0];
      			b->dev = dev;
      			b->blockno = blockno;
      			b->valid = 0;
      			b->refcnt = 1;
     release(&bcache[hash_id].lock);
	release(&block);
      			acquiresleep(&b->lock);
      			return b;


  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

	int hash_id = HASH(b->blockno);

  acquire(&bcache[hash_id].lock);
  b->refcnt--;
  release(&bcache[hash_id].lock);
  releasesleep(&b->lock);
}

void
bpin(struct buf *b) {
	int hash_id = HASH(b->blockno);
  acquire(&bcache[hash_id].lock);
  b->refcnt++;
  release(&bcache[hash_id].lock);
}

void
bunpin(struct buf *b) {
	int hash_id = HASH(b->blockno);
  acquire(&bcache[hash_id].lock);
  b->refcnt--;
  release(&bcache[hash_id].lock);
}


