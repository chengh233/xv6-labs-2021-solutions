#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
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
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  backtrace();
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
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

uint64
sys_sigalarm(void)
{
  int n = 0;
  uint64 fn = 0;
  if(argint(0, &n) < 0)
    return -1;
  if(argaddr(1, &fn) < 0)
    return -1;
  if (n == 0 && fn == 0) {
    myproc()->beats = 0;
  }
  myproc()->tickets = n;
  myproc()->handler = fn;

  return 0;
}

uint64
sys_sigreturn(void)
{
  myproc()->trapframe->epc =  myproc()->trapframe_pre->epc;           // saved user program counter
  myproc()->trapframe->ra  =  myproc()->trapframe_pre->ra;
  myproc()->trapframe->sp  =  myproc()->trapframe_pre->sp;
  myproc()->trapframe->s0  =  myproc()->trapframe_pre->s0;
  myproc()->trapframe->s1  =  myproc()->trapframe_pre->s1;
  myproc()->trapframe->a0  =  myproc()->trapframe_pre->a0;
  myproc()->trapframe->a1  =  myproc()->trapframe_pre->a1;
  myproc()->trapframe->a2  =  myproc()->trapframe_pre->a2;
  myproc()->trapframe->a3  =  myproc()->trapframe_pre->a3;
  myproc()->trapframe->a4  =  myproc()->trapframe_pre->a4;
  myproc()->trapframe->a5  =  myproc()->trapframe_pre->a5;
  myproc()->trapframe->a6  =  myproc()->trapframe_pre->a6;
  myproc()->trapframe->a7  =  myproc()->trapframe_pre->a7;
  myproc()->trapframe->s2  =  myproc()->trapframe_pre->s2;
  myproc()->trapframe->s3  =  myproc()->trapframe_pre->s3;
  myproc()->trapframe->s4  =  myproc()->trapframe_pre->s4;
  myproc()->trapframe->s5  =  myproc()->trapframe_pre->s5;
  myproc()->trapframe->s6  =  myproc()->trapframe_pre->s6;
  myproc()->trapframe->s7  =  myproc()->trapframe_pre->s7;
  myproc()->trapframe->s8  =  myproc()->trapframe_pre->s8;
  myproc()->trapframe->s9  =  myproc()->trapframe_pre->s9;
  myproc()->trapframe->s10 =  myproc()->trapframe_pre->s10;
  myproc()->trapframe->s11 =  myproc()->trapframe_pre->s11;
  myproc()->beats = 0;
  return 0;
}
