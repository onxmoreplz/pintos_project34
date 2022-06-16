/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "../include/userprog/process.h"
#include "../include/lib/round.h"
#include "../include/threads/mmu.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
	
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
void * do_mmap (void *addr, size_t length, int writable, struct file *file, off_t offset) {

	struct file *opened_file = file_reopen(file);
	uint32_t read_bytes = file_length(file) < length ? file_length(file) : length;
	uint32_t zero_bytes = PGSIZE - (read_bytes % PGSIZE); // file_length(file)
	void *return_addr = addr;

	if (opened_file == NULL)
		return NULL;

	while (read_bytes > 0 || zero_bytes > 0) {
		/* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* --------------- Project 3 ---------------- */
		/* TODO: Set up aux to pass information to the lazy_load_segment. */
		// void *aux = NULL;
		struct container *container = (struct container *)malloc(sizeof(struct container));
		container->file = opened_file;
		container->page_read_bytes = page_read_bytes;
		container->offset = offset;

		if (!vm_alloc_page_with_initializer (VM_FILE, addr, writable, lazy_load_segment, container)) {
			// file_close(opened_file);
			return NULL;
		}

		/* Advance. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		addr += PGSIZE;
		offset += page_read_bytes;
		/* ------------------------------------------ */
	}
	return return_addr;	
}

/* Do the munmap */
void
do_munmap (void *addr) {
	//struct file *temp_file = (struct file *)addr;
	struct thread *curr = thread_current();
	//0int temp_size = file_length(temp_file);
	
	while(true) {
		struct page *current_page = spt_find_page(&curr->spt, addr);
		if (current_page == NULL)
			return;

		struct container *temp_con = (struct container *)current_page->uninit.aux;
		if(pml4_is_dirty(curr->pml4, current_page->va)){
			file_write_at(temp_con->file, addr,temp_con->page_read_bytes , temp_con->offset);
			pml4_set_dirty(curr->pml4, current_page->va, 0);
		}
		// hash_delete(&(curr->spt.spt_hash), &(current_page->hash_elem));
		// spt_remove_page(&curr->spt, current_page);
		pml4_clear_page(curr->pml4, current_page->va);
		// current_page->frame = NULL;
		

		//length -=PGSIZE;
		addr += PGSIZE;
	}
}
