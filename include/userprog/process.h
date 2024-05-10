#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_create_initd (const char *file_name);
tid_t process_fork (const char *name, struct intr_frame *if_);
int process_exec (void *f_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (struct thread *next);

/** project2-Command Line Parsing */
void argument_stack(char **argv, int argc, struct intr_frame *if_);

/** project2-System Call */
struct thread *get_child_process(int pid);
int process_add_file(struct file *f);
struct file *process_get_file(int fd);
int process_close_file(int fd);

/** Project 2-Extend File Descriptor */
process_insert_file(int fd, struct file *f);

#define STDIN 1
#define STDOUT 2
#define STDERR 3

#endif /* userprog/process.h */
