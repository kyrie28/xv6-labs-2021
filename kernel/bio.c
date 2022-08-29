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
#define BHASH(x) (x % NBUCKET)

struct bucket {
  struct spinlock lock;
  struct buf head;
};

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  struct bucket buckets[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  for (int i = 0; i < NBUCKET; i++) {
    initlock(&bcache.buckets[i].lock, "bcache.bucket");
    bcache.buckets[i].head.prev = &bcache.buckets[i].head;
    bcache.buckets[i].head.next = &bcache.buckets[i].head;
  }

  // Create linked list of buffers
  for (b = bcache.buf; b < bcache.buf+NBUF; b++) {
    b->next = bcache.buckets[0].head.next;
    b->prev = &bcache.buckets[0].head;
    initsleeplock(&b->lock, "buffer");
    bcache.buckets[0].head.next->prev = b;
    bcache.buckets[0].head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint hash = BHASH(blockno);

  // Is the block already cached?
  acquire(&bcache.buckets[hash].lock);
  for (b = bcache.buckets[hash].head.next; b != &bcache.buckets[hash].head; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.buckets[hash].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.buckets[hash].lock);

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  acquire(&bcache.lock);

  b = 0;
  int membucket = 0;
  uint memtimestamp = (uint)-1;
  for (int i = 0; i < NBUCKET; i++) {
    acquire(&bcache.buckets[i].lock);
    for (struct buf *buf = bcache.buckets[i].head.prev;
         buf != &bcache.buckets[i].head;
         buf = buf->prev) {
      if (buf->refcnt == 0 && memtimestamp > buf->timestamp) {
        buf->refcnt++;
        if (b) {
          release(&bcache.buckets[i].lock);
          acquire(&bcache.buckets[membucket].lock);
          b->refcnt--;
          release(&bcache.buckets[membucket].lock);
          acquire(&bcache.buckets[i].lock);
        }
        b = buf;
        membucket = i;
        memtimestamp = buf->timestamp;
      }
    }
    release(&bcache.buckets[i].lock);
  }

  if (b) {
    acquire(&bcache.buckets[membucket].lock);
    b->next->prev = b->prev;
    b->prev->next = b->next;
    release(&bcache.buckets[membucket].lock);

    acquire(&bcache.buckets[hash].lock);
    b->next = bcache.buckets[hash].head.next;
    b->prev = &bcache.buckets[hash].head;
    bcache.buckets[hash].head.next->prev = b;
    bcache.buckets[hash].head.next = b;
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    release(&bcache.buckets[hash].lock);
    
    release(&bcache.lock);
    
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

  releasesleep(&b->lock);

  uint hash = BHASH(b->blockno);
  acquire(&bcache.buckets[hash].lock);
  b->refcnt--;  
  release(&bcache.buckets[hash].lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


