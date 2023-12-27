#ifndef PAGE_H
#define PAGE_H

#include <hash.h>
#include <list.h>
#include "threads/thread.h"
#include "threads/vaddr.h"

#define VM_BIN 0		// binary
#define VM_SWAP 1		// swap  

struct pte {
    struct hash_elem elem;
    uint8_t type;
    bool writable;
    bool loaded;
    bool is_second_chance;
    bool dirty_bit;
    void *uaddr;
    size_t ofs;

    size_t read_bytes;
    size_t zero_bytes;
    size_t swap_slot;
    struct file *file;
};

struct page {
  void *kaddr;
  struct pte *pte;
  struct thread *thread;
  struct list_elem lru;
};


//function
void init_pte (struct hash *pt);
void insert_pte (struct hash *pt, struct pte *entry);
struct pte* find_pte (void *uaddr);
void delete_pte (struct hash *pt, struct pte *entry);
void destroy_pte (struct hash *pt);
bool load_file (void *kaddr, struct pte *entry);

#endif