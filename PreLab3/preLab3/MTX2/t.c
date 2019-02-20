/*********** t.c file of A Multitasking System *********/
#include <stdio.h>
#include "string.h"
#include "type.h"

PROC proc[NPROC];      // NPROC PROCs
PROC *freeList;        // freeList of PROCs 
PROC *readyQueue;      // priority queue of READY procs
PROC *running;         // current running proc pointer

PROC *sleepList;       // list of SLEEP procs

#include "queue.c"     // include queue.c file
#include "wait.c"      // include wait.c file

/*******************************************************
  kfork() creates a child process; returns child pid.
  When scheduled to run, child PROC resumes to body();
********************************************************/
int body(char *myname), tswitch(), do_sleep(), do_wakeup(), do_exit(), do_switch();
int do_kfork();

char *myname = "Konstantin Shvedov";

// initialize the MT system; create P0 as initial running process
int init() 
{
  int i;
  PROC *p;
  for (i=0; i<NPROC; i++){ // initialize PROCs
    p = &proc[i];
    p->pid = i;            // PID = 0 to NPROC-1  
    p->status = FREE;
    p->priority = 0;      
    p->next = p+1;
  }
  proc[NPROC-1].next = 0;  
  freeList = &proc[0];     // all PROCs in freeList     
  readyQueue = 0;          // readyQueue = empty

  sleepList = 0;           // sleepList = empty
  
  // create P0 as the initial running process
  p = running = dequeue(&freeList); // use proc[0] 
  p->status = READY;
  p->priority = 0;
  p->ppid = 0;             // P0 is its own parent
  
  printList("freeList", freeList);
  printf("init complete: P0 running\n"); 
}

int menu()
{
  printf("****************************************\n");
  printf(" ps fork switch exit jesus sleep wakeup \n");
  printf("****************************************\n");
}

char *status[ ] = {"FREE", "READY", "SLEEP", "ZOMBIE"};

int do_ps()
{
  int i;
  PROC *p;
  printf("PID  PPID  status\n");
  printf("---  ----  ------\n");
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    printf(" %d    %d    ", p->pid, p->ppid);
    if (p == running)
      printf("RUNNING\n");
    else
      printf("%s\n", status[p->status]);
  }
}

int do_jesus()
{
  int i;
  PROC *p;
  printf("Jesus perfroms miracles here\n");
  for (i=1; i<NPROC; i++){
    p = &proc[i];
    if (p->status == ZOMBIE){
      p->status = READY;
      enqueue(&readyQueue, p);
      printf("raised a ZOMBIE %d to live again\n", p->pid);
    }
  }
  printList("readyQueue", readyQueue);
}
    
int body(char *myname)   // process body function
{
  printf("%s\n", myname);
  int c;
  char cmd[64];
  printf("proc %d starts from body()\n", running->pid);
  while(1){
    printf("***************************************\n");
    printf("proc %d running: parent=%d\n", running->pid,running->ppid);
    //********************************************************    
    //printing the children list of the running process
    printf("Children List of running proc:\n");
    PROC *temp = running->child;
    if(temp != NULL)
    {
      printf("[%d]", temp->pid);
    }

    {
      temp = temp->sibling;
      if(temp != NULL)
        {
          printf("->f[%d]", temp->pid);
        }
    } while (temp != NULL);
    //********************************************************
    printList("readyQueue", readyQueue);
    printSleep("sleepList ", sleepList);
    
    menu();
    printf("enter a command : ");
    fgets(cmd, 64, stdin);
    cmd[strlen(cmd)-1] = 0;

    if (strcmp(cmd, "ps")==0)
      do_ps();
    if (strcmp(cmd, "fork")==0)
      do_kfork();
    if (strcmp(cmd, "switch")==0)
      do_switch();
    if (strcmp(cmd, "exit")==0)
      do_exit();
    if (strcmp(cmd, "jesus")==0)
      do_jesus();
   if (strcmp(cmd, "sleep")==0)
      do_sleep();
   if (strcmp(cmd, "wakeup")==0)
      do_wakeup();
    if (strcmp(cmd, "wait")==0)
      do_wait();  
  }

}

int kfork()
{
  int  i;
  PROC *p = dequeue(&freeList);
  if (!p){
     printf("no more proc\n");
     return(-1);
  }
  /* initialize the new proc and its stack */
  p->status = READY;
  p->priority = 1;       // ALL PROCs priority=1, except P0
  p->ppid = running->pid;
  
  running->child = p;
  p->parent = running;
  /************ new task initial stack contents ************
   kstack contains: |retPC|eax|ebx|ecx|edx|ebp|esi|edi|eflag|
                      -1   -2  -3  -4  -5  -6  -7  -8   -9
  **********************************************************/
  for (i=1; i<10; i++)               // zero out kstack cells
      p->kstack[SSIZE - i] = 0;
  p->kstack[SSIZE-1] = (int) ("Konstantin Shvedov");
  p->kstack[SSIZE-3] = (int)body;    // retPC -> body()
  p->ksp = &(p->kstack[SSIZE - 9]);  // PROC.ksp -> saved eflag 
  
  enqueue(&readyQueue, p);           // enter p into readyQueue
  return p->pid;
}

//*****************************************************8
//*****************************************************8
//*****************************************************8
int kexit(int exitValue)
{
  if (exitValue != 1)
  {
    PROC *temp = running->parent;
    //finds process P1
    while (temp->pid != 1)
    {
      temp = temp->parent;
    }  

    //finds last child and adds the children to the end
    if (temp->child == NULL)
    {
      temp->child = running->child;
    }
    else
    {
      temp = temp->child;
      while (temp->sibling != NULL)
      {
        temp = temp->sibling;
      }
      temp->sibling = running->child;
    }
    running->exitCode = exitValue;
    running->status = ZOMBIE;

    
    wakeup(running->parent);
    tswitch();
  }
  else printf("P1 can never die\n");
}

int wait(int *status)
{
  if(running->child == NULL)
  {
    return -1;
  }
  PROC *temp = running->child;
  
  while(1)
  {
    while (temp->status != 3 || temp ==  NULL)
    {
      temp = temp->sibling;
    }
    if (temp->status == 3)
    {
      int zPid = temp->pid;
      status = temp->exitCode;
      enqueue(freeList, temp);
      return zPid;
    }
    sleep(temp->pid);
  }
}
//*****************************************************8
//*****************************************************8
//*****************************************************8

int do_kfork()
{
   int child = kfork();
   if (child < 0)
      printf("kfork failed\n");
   else{
      printf("proc %d kforked a child = %d\n", running->pid, child); 
      printList("readyQueue", readyQueue);
   }
   return child;
}

int do_switch()
{
   tswitch();
}

int do_exit()
{
  kexit(running->pid);  // exit with own PID value 
}

int do_sleep()
{
  int event;
  printf("enter an event value to sleep on : ");
  scanf("%d", &event); getchar();
  sleep(event);
}

int do_wakeup()
{
  int event;
  printf("enter an event value to wakeup with : ");
  scanf("%d", &event); getchar(); 
  wakeup(event);
}

int do_wait()
{
  int zStatus = 0;
  int zPid = 0;
  zPid = wait(&zStatus);
  green();
  printf("Zombie Child Pid: %d\n",zPid);
  printf("Zombie child exit Status: %d\n", zStatus);
  green();
}

/*************** main() function ***************/
int main()
{
   printf("Welcome to the MT Multitasking System\n");
   init();    // initialize system; create and run P0
   kfork();   // kfork P1 into readyQueue  
   while(1){
     printf("P0: switch process\n");
     while (readyQueue == 0);
         tswitch();
   }
}

/*********** scheduler *************/
int scheduler()
{ 
  printf("proc %d in scheduler()\n", running->pid);
  if (running->status == READY)
     enqueue(&readyQueue, running);
  printList("readyQueue", readyQueue);
  running = dequeue(&readyQueue);
  printf("next running = %d\n", running->pid);  
}
