/*
 * AUthor: G.Cabodi
 * Very simple implementation of sys_read and sys_write.
 * just works (partially) on stdin/stdout
 */

#include <types.h>
#include <kern/unistd.h>
#include <clock.h>
#include <copyinout.h>
#include <syscall.h>
#include <lib.h>

#include <spinlock.h>

/*
 * simple file system calls for write/read
 */
int
sys_write(int fd, userptr_t buf_ptr, size_t size)
{
  int i;
  char *p = (char *)buf_ptr;
  static struct spinlock write_lock = SPINLOCK_INITIALIZER;
  if (fd != STDOUT_FILENO && fd != STDERR_FILENO) {
      kprintf("sys_write supported only to stdout\n");
      return -1;
  }
  spinlock_acquire(&write_lock);
  for (i = 0; i < (int)size; i++) {
      putch(p[i]);
  }
  spinlock_release(&write_lock);
  return (int)size;
}

int
sys_read(int fd, userptr_t buf_ptr, size_t size)
{
  int i;
  char *p = (char *)buf_ptr;
  static struct spinlock read_lock = SPINLOCK_INITIALIZER;
  if (fd!=STDIN_FILENO) {
    kprintf("sys_read supported only to stdin\n");
    return -1;
  }
  spinlock_acquire(&read_lock);
  for (i = 0; i < (int)size; i++) {
      p[i] = getch();
      if (p[i] < 0)
          return i;
  }
  spinlock_release(&read_lock);
  return (int)size;
}
