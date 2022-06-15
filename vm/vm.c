/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"

/* --------------- Project 3 --------------- */
#include <hash.h>
#include "threads/vaddr.h"
#include "userprog/process.h"
/* ----------------------------------------- */

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
	/* --------------- Project 3 ---------------- */
	list_init(&frame_table);
	// start = list_begin(&frame_table);
	/* ------------------------------------------ */

}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	// vm_alloc_page_with_initializer() 안의 spt_find_page()에서 해당 페이지가 없으면 malloc으로 해당 Page만큼 크기 할당 받는다.
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */

		/* --------------- Project 3 --------------- */
		bool (*initializer)(struct page *, enum vm_type, void *);
		
		switch (VM_TYPE(type)) {
			case VM_ANON:
				initializer = anon_initializer;
				break;
			case VM_FILE:
				initializer = file_backed_initializer;
				break;
		}

		struct page *new_page = malloc(sizeof(struct page));
		uninit_new (new_page, upage, init, type, aux, initializer);

		new_page->writable = writable;
		// new_page->page_cnt = -1;

		/* TODO: Insert the page into the spt. */
		return spt_insert_page(spt, new_page);
		/* ----------------------------------------- */		
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	/* --------------- Project 3 --------------- */
	struct page *page = (struct page*)malloc(sizeof(struct page));
	page->va = pg_round_down(va);

	struct hash_elem *e;
	e = hash_find(&spt->spt_hash, &page->hash_elem);
	
	free(page);
	
	if (e == NULL) {
		return NULL;
	}

	return hash_entry(e, struct page, hash_elem);
	/* ----------------------------------------- */

	// return page;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */

	/* --------------- Project 3 --------------- */
	struct hash_elem *e = hash_find(&spt->spt_hash, &page->hash_elem);
	
	if (e != NULL) {
		return succ;
	}

	hash_insert(&spt->spt_hash, &page->hash_elem);
	
	return succ = true;
	/* ----------------------------------------- */

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	hash_delete(&spt->spt_hash, &page->hash_elem);
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	/* ---------------- Project 3 ----------------*/
	// struct thread *cur = thread_current();
	// struct list_elem *e = list_begin(&frame_table);

	victim = list_entry(list_pop_front (&frame_table), struct frame, frame_elem);

	/* -------------------------------------------*/
	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	/* ---------------- Project 3 ----------------*/
	swap_out(victim->page);
	/* -------------------------------------------*/

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	/* ---------------- Project 3 ----------------*/
	/* TODO: Fill this function. */
	struct frame *frame = NULL;
	void *kva = palloc_get_page(PAL_USER); // User Pool에서 커널 가상 주소 공간으로 1page 할당
	if (kva == NULL) {
		// frame = vm_evict_frame();
	}
	else {
		frame = malloc(sizeof(struct frame));
		frame->kva = kva;
	}

	list_push_back (&frame_table, &frame->frame_elem);
	frame->page = NULL;

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;

	/* -------------------------------------------*/
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	if (vm_alloc_page(VM_ANON | VM_MARKER_0, addr, 1)) { // VM_MARKER_0 : 이 페이지가 STACK에 있다는 것을 표시, writeable : 1
		vm_claim_page(addr);
		thread_current()->stack_bottom -= PGSIZE;
	}
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED, bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	/* --------------- Project 3 ---------------- */
	if (is_kernel_vaddr(addr)) {	/* first checks if it is a valid page fault */
		return false;
	}

	void *rsp_stack = is_kernel_vaddr(f->rsp) ? thread_current()->rsp_stack : f->rsp;
	if (not_present) {
		if (!vm_claim_page(addr)) {
			if (rsp_stack - 8 <= addr && USER_STACK - 0x100000 <= addr && addr <= USER_STACK) {
				vm_stack_growth(thread_current()->stack_bottom - PGSIZE);
				return true;
			}
			return false;
			PANIC("to do!");
		}
		else {
			return true;
		}
	} 

	return false;
	// return vm_do_claim_page (page);
	/* ------------------------------------------ */

}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
/* va가 속한 페이지를 불러옴*/
bool vm_claim_page (void *va UNUSED) {
	ASSERT(is_user_vaddr(va)) 

	struct page * page = spt_find_page (&thread_current()->spt, va);
	if (page == NULL){
		return false;
	}

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
/* 인자로 받은 Page와 새 Frame을 서로 연결 */
/* pml4_set_page() 함수로 프로세스의 pml4페이지 가상주소와 프레임 물리주소 매핑한 결과를 저장 */
static bool vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	// page와 frame에 저장된 실제 physical memory 주소 (kernel vaddr) 관계를 page table에 등록
	
	/* --------------- Project 3 --------------- */
	if (install_page(page->va, frame->kva, page->writable)) {
		return swap_in(page, frame->kva);
	}

	return false;
	/* ----------------------------------------- */
}

/* --------------- Project 3 --------------- */
unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED) {
	const struct page *p = hash_entry (p_, struct page, hash_elem);
	
	return hash_bytes (&p->va, sizeof p->va);
}

bool page_less (const struct hash_elem *a_,
           		const struct hash_elem *b_, void *aux UNUSED) {
	const struct page *a = hash_entry (a_, struct page, hash_elem);
  	const struct page *b = hash_entry (b_, struct page, hash_elem);

  return a->va < b->va;
}

void *page_destory(struct hash_elem *h_elem, void *aux UNUSED){
	struct page *p = hash_entry(h_elem, struct page, hash_elem);
	vm_dealloc_page(p);
}
/* ----------------------------------------- */

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	/* --------------- Project 3 --------------- */
	hash_init (&spt->spt_hash, page_hash, page_less, NULL);
	/* ----------------------------------------- */
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED, struct supplemental_page_table *src UNUSED) {


	// memcpy(&dst, &src, sizeof(struct supplemental_page_table));
	struct hash_iterator i;
	
	struct hash *child = &src->spt_hash;

	hash_first(&i, child);

	while(hash_next(&i)) {

		struct page	*page_entry = hash_entry (hash_cur (&i), struct page, hash_elem);
		enum vm_type type = page_entry->operations->type;
		void *upage = page_entry->va;
		bool writable = page_entry->writable;
		// vm_initializer *init = page_entry->uninit.init;
		void *aux = page_entry->uninit.aux;

		struct container *container;
		struct page *new_page;


		switch(VM_TYPE(type)){

			case VM_UNINIT :
				container = (struct container *)malloc(sizeof(struct container));

				memcpy(container, (struct container*)page_entry->uninit.aux, sizeof(struct container));
				vm_alloc_page_with_initializer(VM_ANON, upage, writable, page_entry->uninit.init, container);
				
				break;

			case VM_ANON :

				if(!(vm_alloc_page(type, upage, writable))){
					return false;
				}

				new_page = spt_find_page(dst, upage);
				if(!vm_claim_page(upage)){
					return false;
				}
				memcpy(new_page->frame->kva, page_entry->frame->kva, PGSIZE);
				break;

			case VM_FILE :

				if(!(vm_alloc_page(type, upage, writable))){
					return false;
				}

				new_page = spt_find_page(dst, upage);
				if(!vm_claim_page(upage)){
					return false;
				}
				memcpy(new_page->frame->kva, page_entry->frame->kva, PGSIZE);
				break;
		}			
	}

	return true;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */

	hash_destroy(&spt->spt_hash, page_destory);
}
