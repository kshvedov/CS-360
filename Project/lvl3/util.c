/*********** util.c file ****************/

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern MTABLE mtable[NMTABLE];
extern PROC   proc[NPROC], *running;
extern char   gpath[256];
extern char   *name[64];
extern int    n;
extern int    fd, dev;
extern int    nblocks, ninodes, bmap, imap, inode_start;
extern char   line[256], cmd[32], pathname[256], temptk[256];

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}   

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}   

int tokenize(char *pathname)
{
  //printf("Pathname: %s\n", pathname);
  // tokenize pathname in GLOBAL gpath[]
  strcpy(temptk, pathname);
  name[0] = strtok(temptk, "/"); // first call to strtok()
  int i = 1;
  for (; name[i-1]; i++)
  {
      name[i] = strtok(NULL, "/"); // calling strtok() until it returns NULL
  }
  return i - 1;
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, disp;
  INODE *ip;
  MTABLE *mp;

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->dev == dev && mip->ino == ino){
       mip->refCount++;
       //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
    
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
       //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino to buf    
       blk  = (ino-1) / 8 + inode_start;
       disp = (ino-1) % 8;

       //printf("iget: ino=%d blk=%d disp=%d\n", ino, blk, disp);

       get_block(dev, blk, buf);
       ip = (INODE *)buf + disp;
       // copy INODE to mp->INODE
       mip->INODE = *ip;

       return mip;
    }
  }   
  printf("PANIC: no more free minodes\n");
  return 0;
}

iput(MINODE *mip)
{
 int i, block, offset;
 char buf[BLKSIZE];
 INODE *ip;

 if (mip==0) 
     return;

 mip->refCount--;
 
 if (mip->refCount > 0) return;
 if (!mip->dirty)       return;
 
 /* write back */
 //printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino); 

 block =  ((mip->ino - 1) / 8) + inode_start;
 offset =  (mip->ino - 1) % 8;

 /* first get the block containing this inode */
 get_block(mip->dev, block, buf);

 ip = (INODE *)buf + offset;
 *ip = mip->INODE;

 put_block(mip->dev, block, buf);

} 

//int search(INODE *ip, char *name, int dev)
int search(MINODE *mip, char *name)
{
  INODE *ip = &mip->INODE;

  char sbuf[BLKSIZE], temp[256];
    DIR *dp;
    char *cp;

    for (int i=0; i < 12; i++){  // assume DIR at most 12 direct blocks
        if (ip->i_block[i] == 0)
            break;
        
        printf("i=%d i_block[0]=%d\n",ip->i_uid ,ip->i_block[i]);
        get_block(mip->dev, ip->i_block[i], sbuf);

        dp = (DIR *)sbuf;
        cp = sbuf;

        while(cp < sbuf + BLKSIZE){
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            //printf("%4d\t%4d\t%4d\t\t%s\n", 
                //dp->inode, dp->rec_len, dp->name_len, temp);
            if (strcmp(temp, name)==0)
                return dp->inode;

            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    return 0;
}

int getino(char *pathname)
{
  int i, ino, blk, disp;
  INODE *ip;
  MINODE *mip;

  printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
      return 2;

  if (pathname[0]=='/')
    mip = iget(dev, 2);
  else
    mip = iget(running->cwd->dev, running->cwd->ino);

  n = tokenize(pathname);

  for (i=0; i<n; i++){
      printf("===========================================\n");
      ino = search(mip, name[i]);

      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return -1;
      }
      iput(mip);
      mip = iget(dev, ino);
    }
    iput(mip);
   return ino;
}

int getino2(char *pathname, int *dev)
{
  int i, ino, blk, disp, previno;
  INODE *ip;
  MINODE *mip;

  printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
  {
    *dev = root->dev;
    return 2;
  }

  if (pathname[0]=='/')
  {
    *dev = root->dev;
    mip = iget(*dev, 2);
  }
  else
  {
    *dev = running->cwd->dev;
    mip = iget(*dev, running->cwd->ino);
  }

  n = tokenize(pathname);

  for (i=0; i<n; i++){
      printf("===========================================\n");
      previno = mip->ino;
      ino = search(mip, name[i]);
      //printf("previno:%d ino:%d\n", previno, ino);
      //printf("rootino:%d\n", root->ino);
      if (previno == ino && root->dev != mip->dev)
      {
        for(int j = 0; j < NMTABLE; j++)
        {
          if (mip->dev == mtable[j].dev)
          {
            *dev = mtable[j].mntDirPtr->dev;
            //printf("root dev:%d\nmountdir dev:%d\n", root->dev, mtable[j].mntDirPtr->dev);
            //printf("cur dev: %d\n", mip->dev);
            ino = search(mtable[j].mntDirPtr, name[i]);
            break;
          }
        }
      }

      if (ino==0){
        iput(mip);
        printf("name %s does not exist\n", name[i]);
        return -1;
      }
      iput(mip);
      mip = iget(*dev, ino);

      if(mip->mounted == 1 && *dev != mip->mptr->dev)
      {
        *dev = mip->mptr->dev;
        ino = 2;
        iput(mip);
        mip = iget(*dev, ino);
      }
    }
  printf("FOUND\ndev:%d    ino:%d\n", *dev, ino);
  iput(mip);
  return ino;
}

/*****initilly for mkdir and create*****/
//tests what the bit at the exact location is
int tst_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  if (buf[i] & (1 << j))
     return 1;
  return 0;
}

//function that sets bit within buf
int set_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] |= (1 << j);
}

//function that clears bit from a block
int clr_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] &= ~(1 << j);
}

//function thats writes to bitmap that block is taken
int ialloc(int dev)
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       put_block(dev, imap, buf);
       return i+1;
    }
  }
  return 0;
}

//allocates disc block
int balloc(int dev)
{
  int  i;
  char buf[BLKSIZE];

  // read block bitmap block
  get_block(dev, bmap, buf);

  for (i=0; i < nblocks; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       put_block(dev, bmap, buf);
       return i+1;
    }
  }
  return 0;
}
/************************************/
/**********Dealloc Functions*********/

int idealloc(int dev, int ino)
{
  int i;
  char buf[BLKSIZE];
  //MTABLE *mp = (MTABLE *)get_mtable(dev);
  if (ino > ninodes){ // ninodes global
    printf("ERROR: inumber %d out of range\n", ino);
    return -1;
  }
  // get inode bitmap block
  get_block(dev, imap, buf);
  clr_bit(buf, ino-1);
  // write buf back
  put_block(dev, imap, buf);
  // update free inode count in SUPER and GD
  //incFreeInodes(dev);
}

int bdealloc(int dev, int ibl)
{
  int i;
  char buf[BLKSIZE];
  if (ibl > nblocks){ // nblocks global
    printf("ERROR: inumber %d out of range\n", ibl);
    return -1;
  }
  // get block bitmap block
  get_block(dev, bmap, buf);
  clr_bit(buf, ibl-1);
  // write buf back
  put_block(dev, bmap, buf);
  // update free inode count in SUPER and GD
  //incFreeInodes(dev);
}
