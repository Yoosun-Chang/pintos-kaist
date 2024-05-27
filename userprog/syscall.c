#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

/** project2-System Call */
#include "threads/mmu.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/palloc.h"
#include "userprog/process.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/** project2-System Call */
struct lock filesys_lock;  // 파일 읽기/쓰기 용 lock

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
    
    /** project2-System Call */
    // read & write 용 lock 초기화
    lock_init(&filesys_lock);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	/** project2-System Call */
int sys_number = f->R.rax;
#ifdef VM
    thread_current()->stack_pointer = f->rsp;
#endif
    // Argument 순서
    // %rdi %rsi %rdx %r10 %r8 %r9

    switch (sys_number) {
        case SYS_HALT:
            halt();
            break;
        case SYS_EXIT:
            exit(f->R.rdi);
            break;
        case SYS_FORK:
            f->R.rax = fork(f->R.rdi);
            break;
        case SYS_EXEC:
            f->R.rax = exec(f->R.rdi);
            break;
        case SYS_WAIT:
            f->R.rax = process_wait(f->R.rdi);
            break;
        case SYS_CREATE:
            f->R.rax = create(f->R.rdi, f->R.rsi);
            break;
        case SYS_REMOVE:
            f->R.rax = remove(f->R.rdi);
            break;
        case SYS_OPEN:
            f->R.rax = open(f->R.rdi);
            break;
        case SYS_FILESIZE:
            f->R.rax = filesize(f->R.rdi);
            break;
        case SYS_READ:
            f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
            break;
        case SYS_WRITE:
            f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
            break;
        case SYS_SEEK:
            seek(f->R.rdi, f->R.rsi);
            break;
        case SYS_TELL:
            f->R.rax = tell(f->R.rdi);
            break;
        case SYS_CLOSE:
            close(f->R.rdi);
            break;
        case SYS_DUP2:
            f->R.rax = dup2(f->R.rdi, f->R.rsi);
            break;
/** Project 3-Memory Mapped FIles */
#ifdef VM
        case SYS_MMAP:
            f->R.rax = mmap(f->R.rdi, f->R.rsi, f->R.rdx, f->R.r10, f->R.r8);
            break;
        case SYS_MUNMAP:
            munmap(f->R.rdi);
            break;
#endif
        default:
            exit(-1);
    }
}

#ifndef VM
/** project2-System Call */
void check_address(void *addr) {
    thread_t *curr = thread_current();

    if (is_kernel_vaddr(addr) || addr == NULL || pml4_get_page(curr->pml4, addr) == NULL)
        exit(-1);
}
#else
/** Project 3-Anonymous Page */
struct page *check_address(void *addr) {
    struct thread *curr = thread_current();

    if (is_kernel_vaddr(addr) || addr == NULL || !spt_find_page(&curr->spt, addr))
        exit(-1);

    return spt_find_page(&curr->spt, addr);
}

/** Project 3-Memory Mapped Files 버퍼 유효성 검사 */
void check_valid_buffer(void *buffer, size_t size, bool writable) {
    for (size_t i = 0; i < size; i +=8) {
        /* buffer가 spt에 존재하는지 검사 */
        struct page *page = check_address(buffer + i);

        if (!page || (writable && !(page->writable))) /** Project 3-Copy On Write */
            exit(-1);
    }
}
#endif

void 
halt(void) 
{
    power_off();
}

void 
exit(int status) 
{
    struct thread *t = thread_current();
    t->exit_status = status;
    printf("%s: exit(%d)\n", t->name, t->exit_status); // Process Termination Message
    thread_exit();
}

pid_t 
fork(const char *thread_name) 
{
    check_address(thread_name);

    return process_fork(thread_name, NULL);
}

int 
exec(const char *cmd_line) 
{
    check_address(cmd_line);

    off_t size = strlen(cmd_line) + 1;
    char *cmd_copy = palloc_get_page(PAL_ZERO);

    if (cmd_copy == NULL)
        return -1;

    memcpy(cmd_copy, cmd_line, size);

    if (process_exec(cmd_copy) == -1)
        return -1;

    return 0;  // process_exec 성공시 리턴 값 없음 (do_iret)
}

int 
wait(pid_t tid) 
{
    return process_wait(tid);
}

bool 
create(const char *file, unsigned initial_size) 
{
    check_address(file);

    lock_acquire(&filesys_lock);
    bool success = filesys_create(file, initial_size);
    lock_release(&filesys_lock);

    return success;
}

bool 
remove(const char *file) 
{
    check_address(file);

    lock_acquire(&filesys_lock);
    bool success = filesys_remove(file);
    lock_release(&filesys_lock);

    return success;
}

int 
open(const char *file) 
{
    check_address(file);

    lock_acquire(&filesys_lock);
    struct file *newfile = filesys_open(file);

    if (newfile == NULL)
        goto err;

    int fd = process_add_file(newfile);

    if (fd == -1)
        file_close(newfile);

    lock_release(&filesys_lock);
    return fd;
err:
    lock_release(&filesys_lock);
    return -1;
}

int 
filesize(int fd) {
    struct file *file = process_get_file(fd);

    if (file == NULL)
        return -1;

    return file_length(file);
}

/** Project 2-Extend File Descriptor */
int 
read(int fd, void *buffer, unsigned length) 
{
#ifdef VM
    check_valid_buffer(buffer, length, true);
#endif
    check_address(buffer);
    
    struct file *file = process_get_file(fd);

    if (file == STDIN) { 
        int i = 0; 
        char c;
        unsigned char *buf = buffer;

        for (; i < length; i++) {
            c = input_getc();
            *buf++ = c;
            if (c == '\0')
                break;
        }
        return i;
    }

    if (file == NULL || file == STDOUT || file == STDERR)  
        return -1;

    off_t bytes = -1;

    lock_acquire(&filesys_lock);
    bytes = file_read(file, buffer, length);
    lock_release(&filesys_lock);

    return bytes;
}

/** Project 2-Extend File Descriptor */
int 
write(int fd, const void *buffer, unsigned length) 
{
#ifdef VM
    check_valid_buffer(buffer, length, false);
#endif
    check_address(buffer);

    lock_acquire(&filesys_lock);
    off_t bytes = -1;

    struct file *file = process_get_file(fd);

    if (file == STDIN || file == NULL)  
        goto done;

    if (file == STDOUT || file == STDERR) {  
        putbuf(buffer, length);
        bytes = length;
        goto done;
    }

    bytes = file_write(file, buffer, length);

done:
    lock_release(&filesys_lock);
    return bytes;
}

void 
seek(int fd, unsigned position) 
{
        
    struct file *file = process_get_file(fd);

    if (file == NULL || (file >= STDIN && file <= STDERR))
        return;

    file_seek(file, position);
}

int 
tell(int fd) 
{
    struct file *file = process_get_file(fd);

    if (file == NULL || (file >= STDIN && file <= STDERR))
        return -1;

    return file_tell(file);
}

/** Project 2-Extend File Descriptor */
void 
close(int fd) 
{
    struct thread *curr = thread_current();
    struct file *file = process_get_file(fd);

    if (file == NULL)
        return;

    process_close_file(fd);

    if (file == STDIN) {
        file = 0;
        return;
    }

    if (file == STDOUT) {
        file = 0;
        return;
    }

    if (file == STDERR) {
        file = 0;
        return;
    }

    if (file->dup_count == 0)
        file_close(file);
    else
        file->dup_count--;
}

/** Project 2-Extend File Descriptor */
int dup2(int oldfd, int newfd) {
    if (oldfd < 0 || newfd < 0)
        return -1;

    struct file *oldfile = process_get_file(oldfd);

    if (oldfile == NULL)
        return -1;

    if (oldfd == newfd)
        return newfd;

    struct file *newfile = process_get_file(newfd);

    if (oldfile == newfile)
        return newfd;

    close(newfd);

    newfd = process_insert_file(newfd, oldfile);

    return newfd;
}

/** Project 3-Memory Mapped FIles */
#ifdef VM
void *mmap(void *addr, size_t length, int writable, int fd, off_t offset) {
    if (!addr || pg_round_down(addr) != addr || is_kernel_vaddr(addr) || is_kernel_vaddr(addr + length))
        return NULL;

    if (offset != pg_round_down(offset) || offset % PGSIZE != 0)
        return NULL;

    if (spt_find_page(&thread_current()->spt, addr))
        return NULL;

    struct file *file = process_get_file(fd);

    if ((file >= STDIN && file <= STDERR) || file == NULL)
        return NULL;

    if (file_length(file) == 0 || (long)length <= 0)
        return NULL;

    return do_mmap(addr, length, writable, file, offset);
}

/** Project 3-Memory Mapped Files */
void munmap(void *addr) {
    do_munmap(addr);
}

#endif