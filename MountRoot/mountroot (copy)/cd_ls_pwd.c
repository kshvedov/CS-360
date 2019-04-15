/************* cd_ls_pwd.c file **************/

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char   gpath[256];
extern char   *name[64];
extern int    n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256];

#define OWNER  000700
#define GROUP  000070
#define OTHER  000007

change_dir()
{
  if(strcmp(pathname, "")==0)
  {
    INODE *ip = &(root->INODE);
    iput(running->cwd);
    running->cwd = root;
  }
  else
  {
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);
    INODE *ip = &(mip->INODE);
    if (S_ISDIR(ip->i_mode))
    {
      iput(running->cwd);
      running->cwd = mip;
    }
    else
    {
      printf("Pathname is not a DIR!\n");
    }
  }
}

void ls_dir(char *dname)
{
  char buf[BLKSIZE], temp[256];
  DIR *dir, *dp;
  int ino;
  if(strcmp(dname, "")==0)
  {
    ino = running->cwd->ino; 
  }
  else
  {
    ino = getino(dname);
  }
  MINODE *mip = iget(dev, ino);
  get_block(dev, mip->INODE.i_block[0], buf);

  dp = (DIR *)buf;
  char *cp = dp;

  while(cp < buf + BLKSIZE){
    strncpy(temp, dp->name, dp->name_len);
    temp[dp->name_len] = 0;

    ls_file(dp->inode, temp);

    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
}

void ls_file(int ino, char *name)
{
  //printf("NAME:%s, INODE#:%d\n",name,ino);
  MINODE *mip = iget(dev, ino);
  INODE *ip = &(mip->INODE);
  if (S_ISDIR(ip->i_mode))
    putchar('d');
  else if (S_ISREG(ip->i_mode))
    putchar('-');
  else if (S_ISLNK(ip->i_mode))
    putchar('l');
  for(int i = 8; i >=0; i--)
  {
    if (ip->i_mode & (1 << i))
      putchar('r');
    else putchar('-');
    i--;
    if (ip->i_mode & (1 << i))
      putchar('w');
    else putchar('-');
    i--;
    if (ip->i_mode & (1 << i))
      putchar('x');
    else putchar('-');
  }
  printf("%3d ", ip->i_links_count);
  printf("%5d %5d ", ip->i_gid, ip->i_uid);
  printf("%10d ", ip->i_size);
  printf("%.19s ", ctime(&ip->i_mtime));
  printf("%s\n", name);
}

int list_file()
{
  ls_dir(pathname);
}


int pwd(MINODE *wd)
{
  if (wd == root)
  {
    printf("/");
  }
  else
    rpwd(wd);
  printf("\n");
}

rpwd(MINODE *wd)
{
  if (wd==root) return;

  int myIno = 0, parentIno = 0;
  DIR *dir, *dp;
  DIR *dir2, *dp2;
  MINODE *pip;
  char buf[BLKSIZE], myName[256];
  char buf2[BLKSIZE], myName2[256];

  get_block(dev, wd->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  char *cp = dp;

  while(cp < buf + BLKSIZE){
    strncpy(myName, dp->name, dp->name_len);
    myName[dp->name_len] = 0;

    if (strcmp(myName,".") == 0)
    {
      myIno = dp->inode;
    }
    if (strcmp(myName,"..") == 0)
    {
      pip = iget(dev, dp->inode);

      get_block(dev, pip->INODE.i_block[0], buf2);
      dp2 = (DIR *)buf2;
      char *cp2 = dp2;

      while(cp2 < buf2 + BLKSIZE){
        if(dp2->inode == myIno)
        {
          strncpy(myName2, dp2->name, dp2->name_len);
          myName2[dp2->name_len] = 0;
          break;
        }
        cp2 += dp2->rec_len;
        dp2 = (DIR *)cp2;
      }
      break;
    }
    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
  rpwd(pip);

  printf("/%s", myName2);
}
