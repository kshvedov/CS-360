int tswitch();

//turns color red
void red(void)
{
    printf("\e[38;5;196m");
}

//turns color green 
void green(void)
{
    printf("\e[38;5;046m");
}

//returns text color back to white
void back(void)
{
    printf("\e[0m");
}

int sleep(int event)
{
  printf("proc %d going to sleep on event=%d\n", running->pid, event);

  running->event = event;
  running->status = SLEEP;
  enqueue(&sleepList, running);
  printList("sleepList", sleepList);
  tswitch();
}

int wakeup(int event)
{
  PROC *temp, *p;
  temp = 0;
  printList("sleepList", sleepList);

  while (p = dequeue(&sleepList)){
     if (p->event == event){
	printf("wakeup %d\n", p->pid);
	p->status = READY;
	enqueue(&readyQueue, p);
     }
     else{
	enqueue(&temp, p);
     }
  }
  sleepList = temp;
  printList("sleepList", sleepList);
}

int kexit(int exitValue)
{
  printf("proc %d in kexit(), value=%d\n", running->pid, exitValue);
  if (exitValue == 1)
    {
      red();
      printf("P1 can never die\n");
      back();
    }
  else
  {
    PROC *temp = running->parent;
    PROC *temp2 = running->parent;
    //finds process P1
    while (temp->pid != 1)
    {
      temp = temp->parent;
    }  

    //finds last child and adds the children to the end
    if (temp->child == NULL)
    {
      //temp becomes p1
      temp->child = running->child;
      temp2 = temp->child;
      while(temp2 != NULL)
      {
        temp2->parent = temp;
        temp2->ppid = temp->pid;
        temp2 = temp2->sibling;
      }
    }
    else
    {
      temp = temp->child;
      while (temp->sibling != NULL)
      {
        temp = temp->sibling;
      }
      temp->sibling = running->child;
      //temp becomes p1
      temp2 = temp->sibling;
      temp = temp->parent;
      while(temp2 != NULL)
      {
        temp2->parent = temp;
        temp2->ppid = temp->pid;
        temp2 = temp2->sibling;
      }
    }
    running->child = NULL;

    running->exitCode = exitValue;
    running->status = ZOMBIE;

    wakeup(running->ppid);

    tswitch();
  }
}

int wait(int *status)
{
  if(running->child == NULL)
  {
    return -1;
  }
  PROC *temp = running->child;
  PROC *prev = NULL;

  while(1)
  {
    while (temp->sibling != NULL && temp->status != ZOMBIE)
    {
      prev = temp;
      temp = temp->sibling;
    }
    if (temp->status == ZOMBIE)
    {
      int zPid = temp->pid;
      green();
      printf("Zombie being burried [%d]\n\n", zPid);
      back();
      status = temp->exitCode;
      if (prev == NULL)
        running->child = temp->sibling;
      else
        prev->sibling = temp->sibling;
      temp->status = FREE;
      temp->sibling = NULL;
      temp->priority = FREE;
      enqueue(freeList, temp);
      return zPid;
    }
    sleep(running->pid);
  }
}