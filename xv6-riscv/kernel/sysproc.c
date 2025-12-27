#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// Store weights in cache
uint64
sys_weight_store(void)
{
  char name[64];
  uint64 src;
  int len;

  if(argstr(0, name, 64) < 0)
    return -1;
  argaddr(1, &src);
  argint(2, &len);
  
  return weight_store_add(name, src, len);
}
// Load weights from cache
uint64
sys_weight_load(void)
{
  char name[64];
  uint64 dst;
  int len;

  if(argstr(0, name, 64) < 0)
    return -1;
  argaddr(1, &dst);
  argint(2, &len);

  return weight_store_get(name, dst, len);
}

// for multithreading
extern int thread_create(uint64, uint64);
extern int thread_join(int);
extern void thread_exit(void);

uint64
sys_thread_create(void)
{
  uint64 fcn, arg;
  argaddr(0, &fcn);
  argaddr(1, &arg);
  return thread_create(fcn, arg);
}

uint64
sys_thread_join(void)
{
  int pid;
  argint(0, &pid);
  return thread_join(pid);
}

uint64
sys_thread_exit(void)
{
  thread_exit();
  return 0;
}

uint64
sys_rdcycle(void)
{
  return r_time();
}
