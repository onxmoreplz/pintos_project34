/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "../lib/kernel/hash.h"
#include "../include/userprog/process.h"
#include "../include/threads/vaddr.h"

// ---------- Project 3 ----------
unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED);
bool page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED);

struct list frame_table;


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
/* 새 Page 구조체를 할당받고 초기화한 다음, Page 타입에 맞는 초기화 함수를 설정한다.*/
/* 그 후 다시 User 프로그램에게 제어권을 넘긴다.*/
bool vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {


		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		typedef bool (*initializerFunc)(struct page*, enum vm_type, void *);
		initializerFunc initializer = NULL;

		switch (VM_TYPE(type))
		{
		case VM_ANON:
			initializer = anon_initializer;
			break;
		case VM_FILE:
			initializer = file_backed_initializer;
			break;
		}

		struct page *new_page = malloc(sizeof(struct page));
		uninit_new(new_page, upage, init, type, aux, initializer);

		new_page->writable = writable;
		new_page->page_cnt = -1;

		spt_insert_page (spt, new_page);

		
		return true;

		/* create "uninit" page struct by calling uninit_new. */
		
		// uninit_new (page , void *va, vm_initializer *init,
		// enum vm_type type, void *aux,
		// bool (*initializer)(struct page *, enum vm_type, void *))

		/* TODO: Insert the page into the spt. */
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
/* GitBook : You are allowed to add more members when you implement a frame management interface */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	// struct page *page;
	
	/* TODO: Fill this function. */
	struct page *p = (struct page *)malloc(sizeof(struct page)); 
	struct hash_elem *e;

	p->va = pg_round_down(va);
	
	e = hash_find(&spt->hash_page_table, &p->hash_elem);

	free(p);

	return e != NULL ? hash_entry(e, struct page, hash_elem) : NULL;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED, struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	if (!hash_insert(&spt->hash_page_table, &page->hash_elem))
	{
		succ = true;
	}

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */
	 victim = list_entry(list_pop_front(&frame_table), struct frame, f_elem);

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */
	if (swap_out(victim->page)){
		return victim;
	}

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
/**
 * Gitbook
 * 
 * Gets a new physical page from the user pool by calling palloc_get_page. 
 * When successfully got a page from the user pool, also allocates a frame, 
 * initialize its members, and returns it. After you implement vm_get_frame, 
 * you have to allocate all user space pages (PALLOC_USER) through this function. 
 * You don't need to handle swap out for now in case of page allocation failure. 
 * 
 * Just mark those case with PANIC ("todo") for now.
*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = (struct frame *)malloc(sizeof(struct frame)); 
	frame->kva = palloc_get_page(PAL_USER);
	/* TODO: Fill this function. */
	if (frame->kva == NULL) {
		//frame = vm_evict_frame();
		frame->page = NULL;
		return frame;
	}

	//list_push_back(&frame_table,&frame->f_elem);
	frame->page = NULL;

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
/**
 * Page fault가 일어나면 해당 Page의 FLAG를 확인하여 Page fault의 종류를 알아냄
 *  
 * exception.c에서 사용됨 
 *   ->  vm_try_handle_fault (f, fault_addr, user, write, not_present) 
*/
bool vm_try_handle_fault (
	struct intr_frame *f UNUSED, 
	void *addr UNUSED,
	bool user UNUSED, 
	bool write UNUSED, 
	bool not_present UNUSED
) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;

	if (is_kernel_vaddr(addr)){
		return false;
	}

	if (not_present) {
		if(!vm_claim_page(addr)){
			return false;
		}
		else{
			return true;
		}
	}
	
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	return false;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
/**
 * 현재 실행되고 있는 프로세스의 SPT에서 인자로 받은 가상주소 va가 포함된 페이지를 불러옴
 * 그 후 해당 Page(가상 메모리에 있음)에 대해 vm_do_claim() 호출
*/
bool vm_claim_page (void *va UNUSED) {
	ASSERT(is_user_vaddr(va));
	struct page *page;
	page = spt_find_page(&thread_current()->spt,va);
	if (page == NULL) {
		return false;
	}
	/* TODO: Fill this function */

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
/* User 가상 메모리에 있는 페이지를 물리 메모리의 Frame에 할당 */
static bool vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	if (install_page(page->va,frame->kva,page->writable)) {
		return swap_in (page, frame->kva);
	}

	/* TODO: Insert page table entry to map page's VA to frame's PA. */

	return false;
}

/* Initialize new supplemental page table */
void supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->hash_page_table, page_hash, page_less, NULL); //&spt [O | X]
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}

unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED) {
  const struct page *p = hash_entry (p_, struct page, hash_elem);
  return hash_bytes (&p->va, sizeof p->va);
}

/**
 * page_less : Returns true if page a precedes page b.
*/
bool page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED) {
  const struct page *a = hash_entry (a_, struct page, hash_elem);
  const struct page *b = hash_entry (b_, struct page, hash_elem);

  return a->va < b->va;
}
