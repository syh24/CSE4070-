#include "vm/swap.h"

const size_t _BLOCK_CNT = PGSIZE / BLOCK_SECTOR_SIZE;

void swap_init()
{
	// printf("create bitmap");
	//비트맵 생성
	bitmap = bitmap_create(8 * 1024);
}

void swap_in(size_t idx, void *kaddr)
{
	// printf("swap in");

	//swap block 가져옴
	struct block *swap = block_get_role(BLOCK_SWAP);

	// printf("test");
	//data가 없으면 오류
	if (!bitmap_test(bitmap, idx)) return;
	// printf("test fin");
	//block에서 데이터를 읽어옴
	for(int i = 0; i < (int)_BLOCK_CNT; i++){
		block_read(swap, _BLOCK_CNT * idx + i, BLOCK_SECTOR_SIZE * i + kaddr);
	}

	//해당 idx의 비트맵을 0으로 바꿔준다
	bitmap_reset(bitmap, idx);
	// printf("swap fin");
}

size_t swap_out(void *kaddr)
{
	// printf("swap out");

	struct block *swap = block_get_role(BLOCK_SWAP);

	size_t idx = bitmap_scan(bitmap, 0, 1, 0);
	if(idx != BITMAP_ERROR){
		for(int i = 0; i < (int)_BLOCK_CNT; i++){
			block_write(swap, _BLOCK_CNT * idx + i, BLOCK_SECTOR_SIZE * i + kaddr);
		}
		bitmap_set(bitmap, idx, true);
	}
	// printf("swap out fin ");
	return idx;
}