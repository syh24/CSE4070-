#include "userprog/syscall.h"
#include <stdio.h>
#include "userprog/process.h"
#include <syscall-nr.h>
#include <stdlib.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "devices/input.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  switch (*(uint32_t *)(f->esp)) {
    case SYS_HALT:
      shutdown_power_off();
      break;
    case SYS_EXIT:
      if(!is_user_vaddr(f->esp+4))
          exit(-1);
      exit(*(uint32_t *)(f->esp+4));
      break;
    case SYS_EXEC:
      if(!is_user_vaddr(f->esp+4))
				exit(-1);
      f->eax = exec((const char *)*(uint32_t *)(f->esp+4));
      break;
    case SYS_WAIT:
      if(!is_user_vaddr(f->esp+4))
				exit(-1);
      f->eax = wait((pid_t)*(uint32_t *)(f->esp+4));
      break;
    case SYS_READ:
			if(!is_user_vaddr(f->esp+20) || !is_user_vaddr(f->esp+24) || !is_user_vaddr(f->esp+28))
        exit(-1);
			f->eax = read((int)*(uint32_t *)(f->esp+20), (void *)*(uint32_t *)(f->esp+24), (unsigned)*((uint32_t *)(f->esp+28)));
      break;
    case SYS_WRITE:
      if(!is_user_vaddr(f->esp+20) || !is_user_vaddr(f->esp+24) || !is_user_vaddr(f->esp+28))
        exit(-1);
      f->eax = write((int)*(uint32_t *)(f->esp+20), (void *)*(uint32_t *)(f->esp+24), (unsigned)*((uint32_t *)(f->esp+28)));
      break;
    case SYS_FIBO:
      if(!is_user_vaddr(f->esp+4)) exit(-1);
      f->eax = fibonacci((int)*(uint32_t *)(f->esp+4));
      break;
    case SYS_MAX:
      if(!is_user_vaddr(f->esp+4)) exit(-1);
      f->eax = max_of_four_int((int)*(uint32_t *)(f->esp+4), (int)*(uint32_t *)(f->esp+8), (int)*(uint32_t *)(f->esp+12), (int)*(uint32_t *)(f->esp+16));
      break; 
  }
}

void
exit(int status) {
  thread_current()->exit_status = status;
	printf("%s: exit(%d)\n", thread_name(), status);
	thread_exit();
}

int
wait(pid_t pid) {
  return process_wait(pid);
}

pid_t
exec (const char *file) {
  return (pid_t) process_execute(file);
}

int
write (int fd, const void *buffer, unsigned size) {
  //stdout
  if (fd == 1) {
    putbuf((const void*)buffer, size);
    return size;
  }
  return -1;
}

int 
read (int fd, void *buffer, unsigned size) {
  //stdin
	if(fd == 0) {
		for (unsigned i = 0; i < size; i++) {
			*(char *)(buffer + i) = input_getc();
		}
	}
	return size;
}

int
fibonacci(int n) {
  if (n <= 0) return 0;
  else if (n == 1 || n == 2) return 1;
  return fibonacci(n - 2) + fibonacci(n - 1);
}

int
max_of_four_int(int a, int b, int c, int d) {
  int mx = a;
  mx = mx < b ? b : mx;
  mx = mx < c ? c : mx;
  mx = mx < d ? d : mx;
  return mx;
}
