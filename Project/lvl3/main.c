/****************************************************************************
*                   KCW testing ext2 file system                            *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h>

#include "type.h"

MTABLE mtable[NMTABLE];
MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;

char     gpath[256]; // global for tokenized components
char     *name[64];  // assume at most 64 components in pathname
int      n;          // number of component strings

int      fd, dev;
int      nblocks, ninodes, bmap, imap, inode_start;
char     line[256], cmd[32], pathname[256], extra[256], temptk[256];
char     *disk = "disk";
int      writePrint = 1, readPrint = 1;
char     mydirname[256], mybasename[256];

#include "util.c"
#include "functions.c"
#include "open_close_lseek.c"
#include "read_cat.c"
#include "write_cp_mv.c"
#include "mount_unmount.c"

#define MKDIR  1
#define CREAT  2
#define SYMLINK 3

//the initialization of all minodes in memory happens here
//and all the processes are started here
int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i;
    p->uid = i;
    p->gid = 0; //group id
    p->cwd = 0;
    p->status = FREE;
    for (j=0; j<NFD; j++)
      p->fd[j] = 0;
  }
  for (i=0; i<NMTABLE; i++){
     mtable[i].dev = 0;
  }
  root = 0;
}

// load root INODE and set root pointer to it
int mount_root()
{  
  MTABLE *mp = &mtable[0];
  printf("mount_root()\n");
  root = iget(dev, 2);
  mp->mntDirPtr = root;
  root->mptr = mp;
  proc[0].cwd = iget(dev, 2);
  proc[1].cwd = iget(dev, 2);
  mp->dev = dev;
}


//creating pointer to disk
int main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];
  MTABLE *mp = &mtable[0];

  //if the program is started with a parameter(s), the first parameter
  //is taken as the disk and replaces the default disk name
  if (argc > 1)
    disk = argv[1];

  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);  exit(1);
  }
  strcpy(mp->devName, disk);
  strcpy(mp->mntName, "/");
  dev = fd;

  //read super block 1 into buffer 
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  //verify it's an ext2 file system
  if (sp->s_magic != 0xEF53){
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }
  printf("OK\n");
  ninodes = mp->ninodes = sp->s_inodes_count;
  nblocks = mp->nblocks = sp->s_blocks_count;

  //read group descriptor block from block 2 (GD is block 2)
  get_block(dev, 2, buf);
  gp = (GD *)buf;

  bmap = mp->bmap = gp->bg_block_bitmap;
  imap = mp->imap = gp->bg_inode_bitmap;
  inode_start = mp->iblock = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);

  init();
  mount_root();

  printf("root refCount = %d\n", root->refCount);

  
  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);
  
  printf("root refCount = %d\n", root->refCount);
  printf("mount : %s mounted on / \n", disk);

  //printf("hit a key to continue : "); getchar();
  while(1){
    pathname[0]= '\0';
    extra[0]= '\0';
    printf("***********************************************************\n");
    printf("[ls|cd|pwd|mkdir|rmdir|creat|link|unlink|symlink|readlink|stat|chmod|touch|quit]\n");
    printf("[open|close|lseek|pfd|read|write|cat|cp|mv|mount|unmount]\n");
    printf("input command: ");
    fgets(line, 128, stdin);
    printf("***********************************************************\n");
    line[strlen(line)-1] = 0;
    if (line[0]==0)
      continue;
    pathname[0] = 0;
    cmd[0] = 0;
    extra[0] = 0;
    
    sscanf(line, "%s %s %s", cmd, pathname, extra);
    printf("cmd=%s arg[1]=%s arg[2]=%s\n", cmd, pathname, extra);

   if (strcmp(cmd, "ls")==0)
      list_file();
   if (strcmp(cmd, "cd")==0)
      change_dir();
   if (strcmp(cmd, "pwd")==0)
      pwd(running->cwd);
   if (strcmp(cmd, "mkdir")==0)
      myMake(MKDIR);
   if (strcmp(cmd, "creat")==0)
      myMake(CREAT);
   if (strcmp(cmd, "rmdir")==0)
      myrmdir();
   if (strcmp(cmd, "link")==0)
      myLink();
   if (strcmp(cmd, "unlink")==0)
      myUnLink();  
   if (strcmp(cmd, "symlink")==0)
      mySymLink();
   if (strcmp(cmd, "readlink")==0)
   {
      myReadLink(buf);
      printf("Symlink to: %s\n", buf);
   }
   if (strcmp(cmd, "stat")==0)
      myStat();
   if (strcmp(cmd, "chmod")==0)
      myChmod();
   if (strcmp(cmd, "touch")==0)
      myTouch();

   if (strcmp(cmd, "open")==0)
      if(strcmp(pathname, "") == 0 || (strcmp(extra, "") == 0))
         printf("ERROR: too few arguments!\n");
      else myOpenFile();

   if (strcmp(cmd, "close")==0)
      if(strcmp(pathname, "") == 0)
         printf("ERROR: too few arguments!\n");
      else myClose(atoi(pathname));
      
   if (strcmp(cmd, "lseek")==0)
      if(strcmp(pathname, "") == 0 || (strcmp(extra, "") == 0))
         printf("ERROR: too few arguments!\n");
      else myLseek(atoi(pathname), atoi(extra));

   if (strcmp(cmd, "pfd")==0)
      myPfd();
   if (strcmp(cmd, "read")==0)
      myReadFile();

   if (strcmp(cmd, "cat")==0)
      if(strcmp(pathname, "") == 0)
         printf("ERROR: too few arguments!\n");
      else myCat();

   if (strcmp(cmd, "write")==0)
      myWriteFile();

   if (strcmp(cmd, "cp")==0)
      if(strcmp(pathname, "") == 0 || (strcmp(extra, "") == 0))
         printf("ERROR: too few arguments!\n");
      else myCP(pathname, extra);

   if (strcmp(cmd, "mv")==0)
      if(strcmp(pathname, "") == 0 || (strcmp(extra, "") == 0))
         printf("ERROR: too few arguments!\n");
      else myMV(pathname, extra);

   if (strcmp(cmd, "mount")==0)
      myMount();
   
   if (strcmp(cmd, "unmount")==0)
      if(strcmp(pathname, "") == 0)
         printf("ERROR: too few arguments!\n");
      else myUnMount(pathname);

   if (strcmp(cmd, "quit")==0)
      quit();
  }

  //getchar();

  /*for (int i=0; i < 5; i++){  
    ino = ialloc(fd);
    printf("allocated ino = %d\n", ino);
  }*/
}
 
int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  exit(0);
}
