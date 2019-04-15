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

MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;

char   gpath[256]; // global for tokenized components
char   *name[64];  // assume at most 64 components in pathname
int    n;          // number of component strings

int    fd, dev;
int    nblocks, ninodes, bmap, imap, inode_start;
char   line[256], cmd[32], pathname[256], extra[256], temptk[256];
char   *disk = "disk";

#include "util.c"
#include "functions.c"

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
  root = 0;
}

// load root INODE and set root pointer to it
int mount_root()
{  
  printf("mount_root()\n");
  root = iget(dev, 2);
  proc[0].cwd = iget(dev, 2);
  proc[1].cwd = iget(dev, 2);  
}


//creating pointer to disk
int main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];

  //if the program is started with a parameter(s), the first parameter
  //is taken as the disk and replaces the default disk name
  if (argc > 1)
    disk = argv[1];

  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);  exit(1);
  }
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
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  //read group descriptor block from block 2 (GD is block 2)
  get_block(dev, 2, buf);
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  inode_start = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);

  init();
  mount_root();

  printf("root refCount = %d\n", root->refCount);

  
  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);
  
  printf("root refCount = %d\n", root->refCount);

  //printf("hit a key to continue : "); getchar();
  while(1){
    pathname[0]= '\0';
    extra[0]= '\0';
    printf("***********************************************************\n");
    printf("[ls|cd|pwd|mkdir|rmdir|creat|link|unlink|symlink|readlink|stat|chmod|touch|quit]\n");
    printf("input command : ");
    fgets(line, 128, stdin);
    printf("***********************************************************\n");
    line[strlen(line)-1] = 0;
    if (line[0]==0)
      continue;
    pathname[0] = 0;
    cmd[0] = 0;
    
    sscanf(line, "%s %s %s", cmd, pathname, extra);
    printf("cmd=%s pathname=%s\n", cmd, pathname);

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
