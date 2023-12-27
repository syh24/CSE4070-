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
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/synch.h"

struct lock file_lock;

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  lock_init(&file_lock);
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
      //three parameter
			if(!is_user_vaddr(f->esp+4) || !is_user_vaddr((void *)*(uint32_t *)(f->esp+8)) || !is_user_vaddr(f->esp+12))
        exit(-1);
			f->eax = read((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp+8), (unsigned)*((uint32_t *)(f->esp+12)));
      break;
    case SYS_WRITE:
      //three parameter
      if(!is_user_vaddr(f->esp+4) || !is_user_vaddr(f->esp+8) || !is_user_vaddr(f->esp+12))
        exit(-1);
      f->eax = write((int)*(uint32_t *)(f->esp+4), (const void *)*(uint32_t *)(f->esp+8), (unsigned)*((uint32_t *)(f->esp+12)));
      break;
    case SYS_CREATE:
      //two parameter
      if(!is_user_vaddr(f->esp+4) || !is_user_vaddr(f->esp+8)) exit(-1);
      f->eax = create((const char*)*(uint32_t *)(f->esp+4), (unsigned)*(uint32_t *)(f->esp+8));
      break;
    case SYS_REMOVE:
      //one parameter
      if(!is_user_vaddr(f->esp+4)) exit(-1);
      f->eax = remove((const char*)*(uint32_t *)(f->esp+4));
      break;
    case SYS_OPEN:
      //one parameter
      if(!is_user_vaddr(f->esp+4)) exit(-1);
      f->eax = open((const char*)*(uint32_t *)(f->esp+4));
      break;
    case SYS_SEEK:
      //two parameter
      if(!is_user_vaddr(f->esp+4) || !is_user_vaddr(f->esp+8)) exit(-1);
      seek((int)*(uint32_t *)(f->esp+4), (unsigned)*(uint32_t *)(f->esp+8));
      break;
    case SYS_FILESIZE:
      //one parameter
      if(!is_user_vaddr(f->esp+4)) exit(-1);
      f->eax = filesize((int)*(uint32_t *)(f->esp+4));
      break;
    case SYS_TELL:
      //one parameter
      if(!is_user_vaddr(f->esp+4)) exit(-1);
      f->eax = tell((int)*(uint32_t *)(f->esp+4));
      break;
    case SYS_CLOSE:
      //one parameter
      if(!is_user_vaddr(f->esp+4)) exit(-1);
      close((int)*(uint32_t *)(f->esp+4));
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

  //all fd close
  for (int i = 2; i < 128; i++) {
    if (thread_current()->fd[i] != NULL) close(i);
  }
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
  lock_acquire(&file_lock);
  //stdout
  if (fd == 1) {
    putbuf((const void*)buffer, size);
    lock_release(&file_lock);
    return size;
  } else if (fd >= 2) {
    //file이 null일 때 예외처리
    if (thread_current()->fd[fd] == NULL) {
      lock_release(&file_lock);
      return -1;
    }

    //write file
    lock_release(&file_lock);
    return file_write(thread_current()->fd[fd], buffer, size);
  }

  lock_release(&file_lock);
  return -1;
}

int 
read (int fd, void *buffer, unsigned size) {
  lock_acquire(&file_lock);

  //stdin
	if(fd == 0) {
		for (unsigned i = 0; i < size; i++) {
			*(char *)(buffer + i) = input_getc();
		}

    lock_release(&file_lock);
    return size;

	} else if (fd >= 2) {
    if (thread_current()->fd[fd] == NULL) {
      lock_release(&file_lock);
      return -1;
    }

    //read file
    lock_release(&file_lock);
    return file_read(thread_current()->fd[fd], buffer, size);
  }

  lock_release(&file_lock);
	return -1;
}

int
filesize (int fd) {
  if (thread_current()->fd[fd] == NULL) exit(-1);
  return file_length(thread_current()->fd[fd]);
}

void
seek (int fd, unsigned position) {
  if (thread_current()->fd[fd] == NULL) exit(-1);
  file_seek(thread_current()->fd[fd], position);
}

bool
create (const char *file, unsigned initial_size)
{
  if (file == NULL) exit(-1);
  return filesys_create(file, initial_size);
}

bool
remove (const char *file)
{
  if(file == NULL) exit(-1);
  return filesys_remove(file);
}

int
open (const char *file) {
  //잘못된 파일일 경우
  if (file == NULL) return -1;

  lock_acquire(&file_lock);
  struct file* open_file = filesys_open(file);

  //파일이 잘못열렸을 경우
  if (open_file == NULL) {
    lock_release(&file_lock);
    return -1;
  }
  //file이 실행중 write되는 것을 방지
  if (!strcmp(thread_name(), file))
    file_deny_write(open_file);
  //fd 0, 1은 stdin, stdout이므로 체크X
  for (int i = 2; i < 128; i++) {
    //fd is NULL
    if (thread_current()->fd[i] == NULL) {
      thread_current()->fd[i] = open_file;
      lock_release(&file_lock);
      return i;
    }
  }
  lock_release(&file_lock);
  return -1;
}

void
close (int fd) {
  if (thread_current()->fd[fd] == NULL) exit(-1);
  struct file* f = thread_current()->fd[fd];
  thread_current()->fd[fd] = NULL;
  return file_close(f);
}

unsigned 
tell (int fd) {
	if(thread_current()->fd[fd]==NULL) exit(-1);
	return file_tell(thread_current()->fd[fd]);
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
