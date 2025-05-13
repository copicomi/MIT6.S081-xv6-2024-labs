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
#define HASH(x) ((x)%(NBUCKET))

extern uint ticks;

struct {
  struct buf buf[NBUF];
  struct spinlock lock;
  struct {
  	struct spinlock lock;
  } buffer[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  for (int i = 0; i < NBUCKET; i ++) {
	  initlock(&bcache.buffer[i].lock, "bucket");
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  uint have_space = 0;
  uint min_timestamp = 0xffffffffu;
  int select_i = 0;

  // acquire(&bcache.lock);

  int hid = HASH(blockno);

  for (int p = 0, k = hid; p < NBUCKET; p ++) {
	 	k = (p + hid) % NBUCKET;
  	acquire(&bcache.buffer[k].lock);
  	for (int i = k; i < NBUF; i += NBUCKET) {
	  	b = &bcache.buf[i];
    	if(b->dev == dev && b->blockno == blockno){
      		b->refcnt++;
			b->timestamp = ticks;
  	  		release(&bcache.buffer[k].lock);
      		// release(&bcache.lock);
      		acquiresleep(&b->lock);
      		return b;
		}
    	if(!have_space && b->refcnt == 0) {
			have_space = 1;
			select_i = i;
		}
    	if(have_space && b->refcnt == 0 && b->timestamp < min_timestamp) {
			min_timestamp = b->timestamp;
			select_i = i;
		}
		if(!have_space && b->timestamp < min_timestamp) {
			min_timestamp = b->timestamp;
			select_i = i;
		}
    }
  	release(&bcache.buffer[k].lock);
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  b = &bcache.buf[select_i];
  // hid = HASH(select_i);
  acquire(&bcache.buffer[hid].lock);
    {
      	b->dev = dev;
      	b->blockno = blockno;
      	b->valid = 0;
      	b->refcnt = 1;
		b->timestamp = ticks;
    	release(&bcache.buffer[hid].lock);
      	// release(&bcache.lock);
      	acquiresleep(&b->lock);
      	return b;
   	}
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


  int hid = HASH(b->blockno);
  acquire(&bcache.buffer[hid].lock);
  b->refcnt--;
  release(&bcache.buffer[hid].lock);

  releasesleep(&b->lock);
}

void
bpin(struct buf *b) {
  int hid = HASH(b->blockno);
  acquire(&bcache.buffer[hid].lock);
  b->refcnt++;
  release(&bcache.buffer[hid].lock);
}

void
bunpin(struct buf *b) {
  int hid = HASH(b->blockno);
  acquire(&bcache.buffer[hid].lock);
  b->refcnt--;
  release(&bcache.buffer[hid].lock);
}


