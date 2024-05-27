/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"

/** Project 3-Memory Management */
#include "threads/mmu.h"
static struct list frame_table; 

/** Project 3-Swap In/Out */
struct lock frame_lock;
struct list_elem *next = NULL;

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
	
	/** Project 3-Swap In/Out */
	lock_init(&frame_lock);
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
		page->accessible = writable; /** Project 3-Copy On Write */
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
	/** Project 3-Memory Mapped Files */
	hash_delete(&thread_current()->spt.spt_hash, &page->hash_elem);
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	/** Project 3-Swap In/Out */
	lock_acquire(&frame_lock);
	for (next = list_begin(&frame_table); next != list_end(&frame_table); next = list_next(next))
	{
		victim = list_entry(next, struct frame, frame_elem);
		if (pml4_is_accessed(thread_current()->pml4, victim->page->va))
			pml4_set_accessed(thread_current()->pml4, victim->page->va, false);
		else{
			lock_release(&frame_lock);
			return victim;
		}
	}
	lock_release(&frame_lock);
	return victim;
}
/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	/** Project 3-Swap In/Out */
	struct frame *victim = vm_get_victim ();
	if (victim->page)
		swap_out(victim->page);
	return victim;
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
	addr = pg_round_down(addr);
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
	struct page *page = NULL;

    if (addr == NULL || is_kernel_vaddr(addr))
        return false;

    if (not_present)
    {
		/** Project 3-Stack Growth*/
		void *rsp = user ?  f->rsp : thread_current()->stack_pointer;
		if (STACK_LIMIT <= rsp - 8 && rsp - 8 == addr && addr <= USER_STACK){
			vm_stack_growth(addr);
			return true;
		}
		else if (STACK_LIMIT <= rsp && rsp <= addr && addr <= USER_STACK){
			vm_stack_growth(addr);
			return true;
		}
		page = spt_find_page(spt, addr);

		if (!page || (write && !page->writable))
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
	
	/** Project 3-Memory Mapped Files */
 	struct hash_iterator iter;
    hash_first(&iter, &src->spt_hash);
    while (hash_next(&iter)) {
        struct page *src_page = hash_entry(hash_cur(&iter), struct page, hash_elem);
        enum vm_type type = src_page->operations->type;
        void *upage = src_page->va;
        bool writable = src_page->writable;

        if (type == VM_UNINIT) {
            void *aux = src_page->uninit.aux;
            vm_alloc_page_with_initializer(page_get_type(src_page), upage, writable, src_page->uninit.init, aux);
        }

        else if (type == VM_FILE) {
            struct vm_load_arg *aux = malloc(sizeof(struct vm_load_arg));
            aux->file = src_page->file.file;
            aux->ofs = src_page->file.offset;
            aux->read_bytes = src_page->file.page_read_bytes;

            if (!vm_alloc_page_with_initializer(type, upage, writable, NULL, aux))
                return false;

            struct page *dst_page = spt_find_page(dst, upage);
            file_backed_initializer(dst_page, type, NULL);
            dst_page->frame = src_page->frame;
            pml4_set_page(thread_current()->pml4, dst_page->va, src_page->frame->kva, src_page->writable);
        }

        else {                                          
            if (!vm_alloc_page(type, upage, writable)) 
                return false;

			/** Project 3-Copy On Write */
            // if (!vm_claim_page(upage))  
            //     return false;

            // struct page *dst_page = spt_find_page(dst, upage); 
            // memcpy(dst_page->frame->kva, src_page->frame->kva, PGSIZE);

			if (!vm_copy_claim_page(dst, upage, src_page->frame->kva, writable)) 
                return false;
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