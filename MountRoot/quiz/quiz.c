char buf[BLKSIZE];
lseek (dev, (long)2*BLKSIZE, 0);
read(dev, buf, BLKSIZE);

GD *gp = (GD *)buf;

int inode_start = gp->bg_inode_table;

int block =  ((ino - 1) / 8) + inode_start;
int offset =  (ino - 1) % 8;

lseek (dev, (long)block*BLKSIZE, 0);
read(dev, buf, BLKSIZE);


INODE *ip = (INODE *)buf + offset;
thisInode = *ip;
________________________________________________________
char buf[BLKSIZE];
char myName[256];

lseek (dev, (long)mip->INODE.i_block[0]*BLKSIZE, 0);
read(dev, buf, BLKSIZE);

DIR *dp = (DIR *)buf;
char *cp = dp;

while(cp < buf + BLKSIZE)
{
	strncpy(myName, dp->name, dp->name_len);
	myName[dp->name_len] = 0;
	if (strcmp(myName,"..") == 0)
	{
		printf("%d", dp->inode);
		//find dir name look bellow
		break;	
	}
	cp += dp->rec_len;
	dp = (DIR *)cp;
}
__________________________________________________________

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
