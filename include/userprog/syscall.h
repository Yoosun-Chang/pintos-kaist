#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

/** project2-System Call */

/* Process identifier. */
typedef int pid_t;

#include <stdbool.h>
// void check_address(void *addr);
void halt(void);
void exit(int status);

pid_t fork(const char *thread_name);
int exec(const char *cmd_line);
int wait(pid_t);

bool create(const char *file, unsigned initial_size);
bool remove(const char *file);

int open(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned length);
int write(int fd, const void *buffer, unsigned length);
void seek(int fd, unsigned position);
int tell(int fd);
void close(int fd);

/** Project 2-Extend File Descriptor */
int dup2(int oldfd, int newfd);
/** Project 3-Anonymous Page */
#ifndef VM
void check_address(void *addr);
#else
struct page *check_address(void *addr);
#endif

/** Project 3-Memory Mapped Files */
#include "include/filesys/off_t.h";
#include "stddef.h";
void *mmap(void *addr, size_t length, int writable, int fd, off_t offset);
void munmap(void *addr);

#endif /* userprog/syscall.h */