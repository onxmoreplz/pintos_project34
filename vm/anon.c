/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"

#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "bitmap.h"

#include <string.h>

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;

static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* --------------- Project 3 ---------------- */
struct bitmap *swap_table;
const size_t SECTORS_PER_PAGE = PGSIZE / DISK_SECTOR_SIZE;

/* Initialize the data for anonymous pages */
/* ANON 페이지를 위한 디스크 내 스왑 영역을 생성해주는 함수 */
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	// swap_disk = NULL;
	swap_disk = disk_get(1, 1);
	size_t swap_size = disk_size(swap_disk) / SECTORS_PER_PAGE;
	swap_table = bitmap_create(swap_size);
}

/* ------------------------------------------ */

/* Initialize the file mapping */
/* Anonymous Page의 경우, 물리 메모리에서 받아와 초기화할 때 모든 데이터를 0으로 초기화 */
bool anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	struct uninit_page *uninit_page = &page->uninit;
	memset(uninit_page, 0, sizeof(struct uninit_page));

	page->operations = &anon_ops;
	struct anon_page *anon_page = &page->anon;

	anon_page->swap_index = -1;

	return true;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
	int slot_idx_of_page = anon_page->swap_index;

	if (!bitmap_test (swap_table, slot_idx_of_page))
		return false;	

	for (int i = 0; i < SECTORS_PER_PAGE; i ++)
	{
		/* 디스크(swap_disk)의 디스크 섹터 SEC_NO(slot_idx_of_page)의 데이터를 읽어서 BUFFER(kva + DISK_SECTOR_SIZE * i)에 저장해준다. */
		disk_read(swap_disk, slot_idx_of_page * SECTORS_PER_PAGE + i, kva + DISK_SECTOR_SIZE * i);
	}
	bitmap_set(swap_table, slot_idx_of_page, false);
	
	return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	
	size_t empty_slot_idx = bitmap_scan (swap_table, 0, 1, false);
	if (empty_slot_idx == BITMAP_ERROR)
		return false;

	for (int i = 0; i < SECTORS_PER_PAGE; i ++)
	{
		disk_write(swap_disk, empty_slot_idx * SECTORS_PER_PAGE + i, page->va + DISK_SECTOR_SIZE * i);
	}
	bitmap_set(swap_table, empty_slot_idx, true);
	pml4_clear_page(&thread_current()->pml4, page->va);

	anon_page->swap_index = empty_slot_idx;
	return true;

}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
}
