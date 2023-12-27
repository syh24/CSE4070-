#include "vm/page.h"

static unsigned pte_hash_func (const struct hash_elem *e, void *aux) {
    struct pte *entry = hash_entry (e, struct pte, elem);
    return hash_int((int)entry->uaddr);
}

static bool pte_hash_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux) {
    struct pte *entry_a = hash_entry (a, struct pte, elem);
	struct pte *entry_b = hash_entry (b, struct pte, elem);
	return entry_a->uaddr < entry_b->uaddr;
}


void init_pte (struct hash *pt) {
    hash_init(pt, pte_hash_func, pte_hash_less_func, NULL);
}

void insert_pte (struct hash *pt, struct pte *entry) {
    hash_insert(pt, &entry->elem);
}

struct pte* find_pte (void *uaddr) {

    // printf("find pte ");
    struct thread *t = thread_current();
    struct pte entry;
    entry.uaddr = pg_round_down(uaddr);
    return hash_entry(hash_find(&t->vm, &entry.elem), struct pte, elem);
}

void delete_pte (struct hash *pt, struct pte *entry) {
    // printf("delete pte ");
    struct hash_elem *elem = hash_delete(pt, &entry->elem);
    free_page(pagedir_get_page(thread_current()->pagedir, entry->uaddr));
    free(entry);
}

void destroy_pte (struct hash *pt) {
    hash_destroy (pt, NULL);
    return;
}

bool load_file (void *kaddr, struct pte *entry) {
    // printf("load file start ");
    file_seek(entry->file, entry->ofs);
    size_t read_bytes = file_read_at (entry->file, kaddr, entry->read_bytes, entry->ofs);

    if (read_bytes != (int)entry->read_bytes) return false;

    memset(kaddr + read_bytes, 0, entry->zero_bytes);

    // printf("load file fin");

    return true;
}