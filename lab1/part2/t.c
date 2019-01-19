typedef unsigned int u32;

//globals
char *ctable = "0123456789ABCDEF";
int  BASE = 10;

//declaring functions
int myprintf(char *fmt, ...);
int rpu(u32 x);
int prints(char *x);
int printu(u32 x);
int printd(int x);
int printo(u32 x);
int printx(u32 x);

//Main function that tests all the my printf possibilities
// also prints argc value, all of argv[], all of env[] 
int main(int argc, char *argv[ ], char *env[ ])
{
   int temp = 0;
   myprintf("******Values of argc and argv[]******\n");
   myprintf("argc=%d\n\n", argc);
   for(int i = 0; i < argc; i++)
   {
      myprintf("argv[%d]=%s\n", i, argv[i]);
   }
   myprintf("\n");
   for(int i = 0; env[i]; i++)
   {
      myprintf("env[%d]=%s\n", i, env[i]);
   }
   myprintf("\n**************TESTING***************\n");
   myprintf("*************Given Test*************\n");
   myprintf("cha=%c string=%s      dec=%d hex=%x oct=%o neg=%d\n\n\n", 
	         'A', "this is a test", 100,    100,   100,  -100);
   myprintf("**************MY TEST***************\n");
   myprintf("#\t#cubed\t#cubed_hex\t#cube_doct\t\n");
   for(int i = -5; i < 6; i++)
   {
      temp = i * i * i;
      if(temp < 0)
      {
         myprintf("%d\t%d\t%x\t%o\n", i, temp, temp, temp);
      }
      else myprintf("%d\t%d\t%x\t\t%o\n", i, temp, temp, temp);
   }
   myprintf("************************************\n");
}
//myprintf function that prints everything in string and
//if encounters %..., using a switch statement picks according
//print function
int myprintf(char *fmt, ...)
{
   char *cp = fmt;
   int *ip = (int *)&fmt + 1;
   for(int i = 0; cp[i] != '\0'; i++)
   {
      if(cp[i] == '\n')
      {
         putchar('\n');
      }
      else if(cp[i] == '\t')
      {
         putchar('\t');
      }
      else if(cp[i] != '%')
      {
         putchar(cp[i]);
      }
      else
      {
         i++;
         switch(cp[i])
         {
            case 'c':
               putchar(*ip);
               break;
            case 's':
               prints(*ip);
               break;
            case 'u':
               printu(*ip);
               break; 
            case 'd':
               printd(*ip);
               break;
            case 'o':
               printo(*ip);
               break;
            case 'x':
               printx(*ip);
               break;
         }
         ip++;
      }
   }
}

int rpu(u32 x)
{  
   char c;
   if (x)
   {
      c = ctable[x % BASE];
      rpu(x / BASE);
      putchar(c);
   }
}

int prints(char *x)
{
   for(int i = 0; x[i] != '\0'; i++)
   {
      putchar(x[i]);
   }
   putchar(' ');
}

int printu(u32 x)
{
   (x == 0)? putchar('0') : rpu(x);
   putchar(' ');
}

int printd(int x)
{
   if(x == 0)
   {
      putchar('0');
   }
   else if (x < 0)
   {
      putchar('-');
      x = -x;
   }
   rpu(x);
   putchar(' ');
}

int printx(u32 x)
{
   putchar('0'); putchar('x');
   BASE = 16;
   printu(x);
   BASE = 10;
}

int printo(u32 x)
{
   putchar('0');
   BASE = 8;
   printu(x);
   BASE = 10;
}