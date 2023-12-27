#ifndef FRAME_H
#define FRAME_H

#include <list.h>
#include <hash.h>
#include "filesys/file.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"
#include "vm/swap.h"
#include "vm/page.h"

struct list lru_list;
struct lock lru_list_lock;
struct list_elem *lru_clock;

void lru_init (void);
void add_page (struct page *page);
void del_page (struct page *page);
struct page *alloc_page (enum palloc_flags flags);
void free_page (void *kdaar);
void try_to_free_pages();

#endif