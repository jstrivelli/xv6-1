#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// Halt (shutdown) the system by sending a special
// signal to QEMU.
// Based on: http://pdos.csail.mit.edu/6.828/2012/homework/xv6-syscall.html
// and: https://github.com/t3rm1n4l/pintos/blob/master/devices/shutdown.c
int
sys_halt(void)
{
  char *p = "Shutdown";
  for( ; *p; p++)
    outw(0xB004, 0x2000);
  return 0;
}

int sys_clone(void){
  int add;
  int arg;
  int stack_add;

  if(argint(0, &add) < 0){ return -1; }
  if(argint(1, &arg) <0) {  return -1;}
  if(argint(2, &stack_add)){return -1; }
   
  return clone((void*)add, (void*)arg, (void*) stack_add);
}

int sys_join(void){
   int pid;
   int stack_add;
   int retval;
   
   if(argint(0, &pid) < 0){ return -1; }
   if(argint(1, &stack_add) < 0){return -1;}
   if(argint(2, &retval) < 0){return -1; }
   
   join((int) pid, (void **)stack_add, (void **)retval);
   return 0; 
}

int sys_texit(void){
   int retval;

   if(argint(0, &retval) < 0){return -1;}
   proc->retval = (void *)retval;
   exit();

   return 0;

}

int sys_mutex_init(void){
   int i;
   for(i = 0; i< 32; i++){
      if(proc->mtable[i].active ==0){
	   proc->mtable[i].active = 1;
	   proc->mtable[i].mutex_id = i;
           proc->mtable[i].chan = &(proc->mtable[i]);
           return i;
      }
   }
   return -1;
}

int sys_mutex_destroy(void){
  int mutex_id;
  if(argint(0, &mutex_id)){return -1;}
  //out of bounds of mutex table
  if(mutex_id < 0 || mutex_id > 31){return -1;}
  //mutex is locked
  if(proc->mtable[mutex_id].locked == 1){return -1;}
  //mtable is uninitialized
  if(proc->mtable[mutex_id].active == 0){return -1;}
 
  proc->mtable[mutex_id].active = 0; 
  return 0;
}

int sys_mutex_lock(void){
    int mutex_id;
    if(argint(0,&mutex_id)<0){return -1;}
    //out of bounds of mutex table
    if(mutex_id < 0 || mutex_id > 31){return -1;}
    //mutex is uninitilized 
    if(proc->mtable[mutex_id].active == 0){return -1;}

    acquire(&(proc->parent->mtable[mutex_id].sl));
    while(&(proc->parent->mtable[mutex_id].sl)){
      sleep(proc->parent->mtable[mutex_id].chan, &(proc->parent->mtable[mutex_id].sl));
    }

    proc->parent->mtable[mutex_id].locked = 1;
    release(&(proc->parent->mtable[mutex_id].sl));
    return 0;
}

int sys_mutex_unlock(void){
   int mutex_id;
   if(argint(0,&mutex_id)<0){return -1;}
   //out of bounds
   if(mutex_id>31 || mutex_id<0){return -1;}
   //mutex is uninitilized
   if(proc->mtable[mutex_id].active == 0){return -1;}

   acquire(&(proc->parent->mtable[mutex_id].sl));
   proc->parent->mtable[mutex_id].locked = 0;
   wakeup(proc->parent->mtable[mutex_id].chan);
   release(&(proc->parent->mtable[mutex_id].sl));

   return 0;

}
