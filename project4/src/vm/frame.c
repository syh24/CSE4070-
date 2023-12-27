#include "vm/frame.h"
#include <stdlib.h>

void lru_init() 
{
    list_init(&lru_list);
    lock_init(&lru_list_lock);
    lru_clock = NULL;
}

void add_page(struct page *page)
{
	lock_acquire (&lru_list_lock);
    list_push_back (&lru_list, &(page->lru));
	lock_release (&lru_list_lock);
}

void del_page(struct page *page)
{
    if (page != NULL) {
		//현재 가르키고 있는 애랑 clock이 같을 경우 다음 거를 가르키도록 수정
        if (lru_clock == &page->lru) {
            lru_clock = list_next (lru_clock);
        }
        list_remove (&page->lru);
    }
}

struct page *alloc_page(enum palloc_flags flags)
{
	// printf("alloc ");
	// lock_acquire (&lru_list_lock); 
	uint8_t *kaddr = palloc_get_page(flags);
	/*실패시 성공할 때 까지 시도 */
	while(kaddr == NULL){
		retry_alloc_page();
		kaddr = palloc_get_page(flags);
	}

	//페이지 메모리 할당
	struct page *page = malloc(sizeof(struct page));

	page->kaddr  = kaddr;
	page->thread = thread_current();
	
	//lru 리스트에 해당 페이지 추가한다.
	add_page(page);
	// lock_release (&lru_list_lock);

	return page;
}

void free_page_by_str(struct page *page) 
{
    del_page(page);
	pagedir_clear_page(page->thread->pagedir, pg_round_down(page->pte->uaddr));
	palloc_free_page(page->kaddr);
    free(page);
}

void
free_page (void *kaddr)
{
	// printf("free page ");
	lock_acquire (&lru_list_lock);
	struct list_elem *e;
	struct page *page = NULL;

	// kaddr가진 페이지 free
	for (e = list_begin(&lru_list); e != list_end(&lru_list); e = list_next(e)) {
		struct page *tmp = list_entry (e, struct page, lru);
		if (tmp->kaddr == kaddr) {
			page = tmp;
			break;
		}
	}

	//page가 null이 아니면 free 시켜준다.
    if (page != NULL) {
		free_page_by_str (page);
	}

	// printf("free fin");

	lock_release (&lru_list_lock);
}

static struct list_elem *lru_clock_algorithm(void)
{
	if (list_empty (&lru_list))
		return NULL;

	//null이거나 끝이면 처음으로
	if (lru_clock == NULL || lru_clock == list_end (&lru_list))
		return list_begin (&lru_list);

	return list_next (&lru_list);
}

void 
retry_alloc_page(void)
{
	struct list_elem *elem;
	struct page *lru_page;

	// printf("try cheeck ");
	lock_acquire (&lru_list_lock);

	while(1){
		
		// printf("lru algo start");
		elem = lru_clock_algorithm();
		// printf("lru algo fin ");

		//empty일 경우
		if(elem == NULL){
			lock_release(&lru_list_lock);
			return;
		}
		lru_page = list_entry(elem, struct page, lru);
		
		// printf("second chance start ");
		if(pagedir_is_accessed(lru_page->thread->pagedir, lru_page->pte->uaddr)){
			lru_page->pte->is_second_chance = true;
			pagedir_set_accessed(lru_page->thread->pagedir, lru_page->pte->uaddr, false);
			continue;
		}
		break;
	}

	// printf("memory swap ");
	if (lru_page->pte->type == VM_SWAP) {
		if (lru_page->pte->is_second_chance)
			lru_page->pte->swap_slot = swap_out(lru_page->kaddr); 
	} else if (lru_page->pte->type == VM_BIN) {
		if (pagedir_is_dirty(lru_page->thread->pagedir, lru_page->pte->uaddr) && lru_page->pte->is_second_chance) {
			lru_page->pte->dirty_bit = true;
			lru_page->pte->type = VM_SWAP;
			lru_page->pte->swap_slot = swap_out(lru_page->kaddr); 	
		}
	}

	// printf("swap fin ");
	lru_page->pte->loaded = false;
	free_page_by_str(lru_page);
	lock_release(&lru_list_lock);
}