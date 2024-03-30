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

#define NULL 0
#define HASH_TABLE_SIZE 331

struct hash_entry {
  struct spinlock lock;
  struct buf* head;
  struct buf* tail;
};

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

struct hash_entry hash_table[HASH_TABLE_SIZE];

struct buf*
hash_find(uint dev, uint blockno, struct hash_entry* entry)
{
  struct buf* b = entry->head;
  while (b) {
    if(b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      return b;
    }
    b = b->next;
  }
  return NULL;
}

int
hash_put(struct buf* b,struct hash_entry* entry)
{
  if (entry->tail == NULL) {
    entry->head = b;
    entry->tail = b;
  } else {
    entry->tail->next = b;
    entry->tail = b;
  }
  return 0;
}

int
hash_remove(uint dev, uint blockno, struct hash_entry* entry)
{
  struct buf* b = entry->head;
  struct buf* pre = NULL;
  while(b) {
    if(b->dev == dev && b->blockno == blockno) {
      if(pre == NULL) {
        entry->head = b->next;
        entry->tail = b->next;
      } else {
        pre->next = b->next;
        if (pre->next == NULL) {
          entry->tail = pre;
        }
      }
      return 0;
    }
    pre = b;
    b = b->next;
  }
  return 1;
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = NULL;
    b->timestamp = 0;
    initsleeplock(&b->lock, "buffer");
  }
  struct hash_entry* e = NULL;
  for(e = &hash_table[0]; e < &hash_table[0]+HASH_TABLE_SIZE; e++){
    initlock(&e->lock, "entry");
    e->head = NULL;
    e->tail = NULL;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{

  struct buf *b = NULL;

  uint key = (dev+blockno)%HASH_TABLE_SIZE;
  struct hash_entry *entry = &hash_table[key];
  acquire(&entry->lock);
  // check if there is one in the cache
  b = hash_find(dev, blockno, entry);
  if (b) {
    release(&entry->lock);
    acquiresleep(&b->lock);
    return b;
  } else {
    // check time stamp and find the least used buffer
    uint min = 0xFFFFFFFF;
    uint idx = 0;
    int i = 0;
    for (i = 0;i < NBUF; i++) {
      struct buf* tmp = &bcache.buf[i];
      if(tmp->refcnt == 0) {
        if(tmp->timestamp < min) {
          min = tmp->timestamp;
          idx = i;
        }
      }
    }
    // find least uesed get key
    b = &bcache.buf[idx];
    uint old_key = (b->dev+b->blockno)%HASH_TABLE_SIZE;
    struct hash_entry *old_entry = &hash_table[old_key];
    if (old_entry != entry) {
      acquire(&old_entry->lock);
    }
    hash_remove(b->dev, b->blockno, old_entry);
    if (old_entry != entry) {
      release(&old_entry->lock);
    }
    // Add new entry to table
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    b->next = NULL;
    hash_put(b, entry);
    release(&entry->lock);
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
  uint key = (b->dev+b->blockno)%HASH_TABLE_SIZE;
  struct hash_entry *entry = &hash_table[key];
  acquire(&entry->lock);
  b->refcnt--;
  if(b->refcnt == 0) b->timestamp = ticks;
  release(&entry->lock);
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


