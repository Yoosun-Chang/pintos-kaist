/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"

/** Project 3-Memory Mapped FIles */
#include "threads/mmu.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "userprog/syscall.h"

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

	/** Project 3-Memory Mapped Files */
    struct vm_load_arg *aux = (struct vm_load_arg *)page->uninit.aux;
    file_page->file = aux->file;
    file_page->offset = aux->ofs;
    file_page->page_read_bytes = aux->read_bytes;

    return true;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
	/** Project 3-Swap In/Out */
	int read = file_read_at(file_page->file, page->frame->kva, file_page->page_read_bytes, file_page->offset);
	memset(page->frame->kva + read, 0, PGSIZE - read);
	return true;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
	/** Project 3-Swap In/Out */
	struct frame *frame = page->frame;

	if (pml4_is_dirty(thread_current()->pml4, page->va))
	{
		file_write_at(file_page->file, page->frame->kva, file_page->page_read_bytes, file_page->offset);
		pml4_set_dirty(thread_current()->pml4, page->va, false);
	}
	page->frame->page = NULL;
	page->frame = NULL;
	pml4_clear_page(thread_current()->pml4, page->va);
	return true;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;

	/** Project 3-Memory Mapped Files */
    if (pml4_is_dirty(thread_current()->pml4, page->va)) {
        file_write_at(file_page->file, page->va, file_page->page_read_bytes, file_page->offset);
        pml4_set_dirty(thread_current()->pml4, page->va, false);
    }

    if (page->frame) {
        list_remove(&page->frame->frame_elem);
        page->frame->page = NULL;
        page->frame = NULL;
        free(page->frame);
    }

    pml4_clear_page(thread_current()->pml4, page->va);
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
			
	/** Project 3-Memory Mapped FIles */
	lock_acquire(&filesys_lock);
    struct file *mfile = file_reopen(file);
    void *ori_addr = addr;
    size_t read_bytes = (length > file_length(mfile)) ? file_length(mfile) : length;
    size_t zero_bytes = PGSIZE - read_bytes % PGSIZE;

    ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
    ASSERT(pg_ofs(addr) == 0);
    ASSERT(offset % PGSIZE == 0);

    struct aux *aux;
    while (read_bytes > 0 || zero_bytes > 0) {
        size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;

        struct vm_load_arg *aux = (struct vm_load_arg *)malloc(sizeof(struct vm_load_arg));
        if (!aux)
            goto err;

        aux->file = mfile;
        aux->ofs = offset;
        aux->read_bytes = page_read_bytes;

        if (!vm_alloc_page_with_initializer(VM_FILE, addr, writable, lazy_load_segment, aux)) {
            goto err;
        }

        read_bytes -= page_read_bytes;
        zero_bytes -= page_zero_bytes;
        addr += PGSIZE;
        offset += page_read_bytes;
    }
	lock_release(&filesys_lock);
    return ori_addr;

err:
    free(aux);
    return NULL;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	/** Project 3-Memory Mapped FIles */
    struct thread *curr = thread_current();
    struct page *page;

    lock_acquire(&filesys_lock);
    while ((page = spt_find_page(&curr->spt, addr))) {
        if (page)
            destroy(page);

        addr += PGSIZE;
    }
    lock_release(&filesys_lock);
}
