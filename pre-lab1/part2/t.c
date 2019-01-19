#include <stdio.h>
#include <stdlib.h>

int *FP;

int main(int argc, char *argv[ ], char *env[ ])
{
  int a,b,c;
  printf("enter main\n");
  
  printf("&argc=%x argv=%x env=%x\n", &argc, argv, env);
  printf("&a=%8x &b=%8x &c=%8x\n", &a, &b, &c);

  //(1) Write C code to print values of argc and argv[] entries
  printf("******Values of argc and argv[]******\n");
  printf("argc=%x\n", argc);
  for(int i = 0; i < argc; i++)
  {
    printf("argv[%d]=%s\n", i, argv[i]);
  }
  printf("************************************\n");

  a=1; b=2; c=3;
  A(a,b);
  printf("exit main\n");
}

int A(int x, int y)
{
  int d,e,f;
  printf("enter A\n");
  printf("******Addresses of x, y******\n");
  printf("&x=%x, &y=%x\n", &x, &y);
  // write C code to PRINT ADDRESS OF d, e, f
  printf("******Addresses of d,e,f******\n");
  printf("&d=%x, &e=%x, &f=%x\n", &d, &e, &f);
  printf("************************************\n");

  d=4; e=5; f=6;
  B(d,e);
  printf("exit A\n");
}

int B(int x, int y)
{
  int g,h,i;
  printf("enter B\n");
  printf("******Addresses of x, y******\n");
  printf("&x=%x, &y=%x\n", &x, &y);
  // write C code to PRINT ADDRESS OF g,h,i
  printf("******Addresses of g,h,i******\n");
  printf("&g=%x, &h=%x, &i=%x\n", &g, &h, &i);
  printf("************************************\n");

  g=7; h=8; i=9;
  C(g,h);
  printf("exit B\n");
}

int C(int x, int y)
{
  int u, v, w, i, *p;

  printf("enter C\n");
  printf("******Addresses of x, y******\n");
  printf("&x=%x, &y=%x\n", &x, &y);
  // write C cdoe to PRINT ADDRESS OF u,v,w,i,p;
  printf("******Addresses of u,v,w,i,p******\n");
  printf("&u=%x, &v=%x, &w=%x, &i=%x, &p=%x\n", &u, &v, &w, &i, &p);
  printf("************************************\n");

  u=10; v=11; w=12; i=13;

  FP = (int *)getebp();  // FP = stack frame pointer of the C() function


//(2). Write C code to print the stack frame link list.
  printf("******Printing Stack frame list FP=%x******\n", FP);
  while(FP != 0)
  {
    printf("%x => ", FP);
    FP = *FP;
  }
  printf("%x\n", FP);
  printf("************************************\n");

  p = (int *)&p;

//(3). Print the stack contents from p to the frame of main()
//    YOU MAY JUST PRINT 128 entries of the stack contents.
  printf("******Printing Stack Content, 128 entries******\n", FP);
  for (int k = 0; k < 128; k++)
  {
    printf("%d)\t%x\t%x\n", k+1, p, *p);
    p++;
  }
//(4). On a hard copy of the print out, identify the stack contents
//   as LOCAL VARIABLES, PARAMETERS, stack frame pointer of each function.
}