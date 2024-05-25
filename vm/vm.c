/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"

/** Project 3-Memory Management */
#include "threads/mmu.h"
static struct list frame_table; 

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

	/** Project 3-Memory Management */
	list_init(&frame_table);
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
	if (spt_find_page (spt, upage) == NULL) {

		/** #project3-Anonymous Page */
		struct page *page = malloc(sizeof(struct page));

		if (!page)
			goto err;

		typedef bool (*initializer_by_type)(struct page *, enum vm_type, void *);
		initializer_by_type initializer = NULL;

		switch (VM_TYPE(type))
		{
		case VM_ANON:
			initializer = anon_initializer;
			break;
		case VM_FILE:
			initializer = file_backed_initializer;
			break;
		}

		uninit_new(page, upage, init, type, aux, initializer);
		page->writable = writable;
		
		return spt_insert_page(spt, page);
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {

	/** Project 3-Memory Management */
	struct page *page = (struct page *)malloc(sizeof(struct page));     
    page->va = pg_round_down(va);                                       
    struct hash_elem *e = hash_find(&spt->spt_hash, &page->hash_elem);  
    free(page);                                                         

    return e != NULL ? hash_entry(e, struct page, hash_elem) : NULL;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	/** Project 3-Memory Management */
	return hash_insert(&spt->spt_hash, &page->hash_elem) ? false : true;
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

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	/** Project 3-Memory Management */
	struct frame *frame = (struct frame *)malloc(sizeof(struct frame));
	ASSERT (frame != NULL);

	frame->kva = palloc_get_page(PAL_USER | PAL_ZERO);  

    if (frame->kva == NULL)
        frame = vm_evict_frame();  
    else
        list_push_back(&frame_table, &frame->frame_elem);
		
    frame->page = NULL;

	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	/** Project 3-Stack Growth*/
    bool success = false;
    if (vm_alloc_page(VM_ANON | VM_MARKER_0, addr, true)) {
        success = vm_claim_page(addr);

        if (success) {
            thread_current()->stack_bottom -= PGSIZE;
        }
    }
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;

	/** Project 3-Anonymous Page */
	struct page *page = spt_find_page(&thread_current()->spt, addr);
   
    if (addr == NULL || is_kernel_vaddr(addr))
        return false;

    if (not_present)
    {
		/** Project 3-Stack Growth*/
		void *stack_pointer = is_kernel_vaddr(f->rsp) ? thread_current()->stack_pointer : f->rsp;
    	if (stack_pointer - 8 <= addr && addr >= STACK_LIMIT && addr <= USER_STACK) {
        	vm_stack_growth(thread_current()->stack_bottom - PGSIZE);
        	return true;
    }
        page = spt_find_page(spt, addr);
        if (page == NULL)
            return false;
        if (write == 1 && page->writable == 0)
            return false;
        return vm_do_claim_page(page);
    }
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
bool
vm_claim_page (void *va UNUSED) {
	/** Project 3-Memory Management */
	struct page *page = spt_find_page(&thread_current()->spt, va);

    if (page == NULL)
        return false;

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

    if (!pml4_set_page(thread_current()->pml4, page->va, frame->kva, page->writable))
        return false;

	return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	/** Project 3-Memory Management */
	hash_init(spt, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
	
	/** Project 3-Anonymous Page */
	struct hash_iterator i;
	hash_first(&i, &src->spt_hash);
	while (hash_next(&i))
	{
		struct page *src_page = hash_entry(hash_cur(&i), struct page, hash_elem);
		enum vm_type src_type = src_page->operations->type;

		if (src_type == VM_UNINIT)
		{
			vm_alloc_page_with_initializer(
				src_page->uninit.type,
				src_page->va,
				src_page->writable,
				src_page->uninit.init,
				src_page->uninit.aux);
		}
		else
		{
			if (vm_alloc_page(src_type, src_page->va, src_page->writable) && vm_claim_page(src_page->va))
			{
				struct page *dst_page = spt_find_page(dst, src_page->va);
				memcpy(dst_page->frame->kva, src_page->frame->kva, PGSIZE);
			}
		}
	}
	return true;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/** Project 3-Anonymous Page */
	hash_clear(&spt->spt_hash, hash_page_destroy);
}

/** Project 3-Memory Management */
uint64_t 
page_hash(const struct hash_elem *e, void *aux)
{
	struct page *page = hash_entry(e, struct page, hash_elem);
	return hash_bytes(&page->va, sizeof page->va);
}

/** Project 3-Memory Management */
bool 
page_less(const struct hash_elem *a, const struct hash_elem *b, void *aux)
{
	struct page *page_a = hash_entry(a, struct page, hash_elem);
	struct page *page_b = hash_entry(b, struct page, hash_elem);

	return page_a->va < page_b->va;
}

/** Project 3-Anonymous Page */
void 
hash_page_destroy(struct hash_elem *e, void *aux)
{
    struct page *page = hash_entry(e, struct page, hash_elem);
    destroy(page);
    free(page);
}