#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "lib/user/syscall.h"

void syscall_init (void);

void exit(int status);

int wait(pid_t pid);

pid_t exec (const char *file);

int write (int fd, const void *buffer, unsigned size);

int read (int fd, void *buffer, unsigned size);

int fibonacci(int n);

int max_of_four_int(int a, int b, int c, int d);

#endif /* userprog/syscall.h */
