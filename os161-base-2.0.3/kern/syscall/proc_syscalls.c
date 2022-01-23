/*
 * AUthor: G.Cabodi
 * Very simple implementation of sys__exit.
 * It just avoids crash/panic. Full process exit still TODO
 * Address space is released
 */

#include <types.h>
#include <kern/unistd.h>
#include <clock.h>
#include <copyinout.h>
#include <syscall.h>
#include <lib.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>

/*
 * simple proc management system calls
 */
void
sys__exit(int status)
{
  /* get address space of current process and destroy */
  as_deactivate(); //disattivazione dell'address space

  thread_exit();

  panic("thread_exit returned (should not happen)\n");
  (void)status;
}
