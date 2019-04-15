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
extern char line[256], cmd[32], pathname[256], extra[256], temptk[256];
extern char *disk;
char mydirname[256], mybasename[256];

#define OWNER  000700
#define GROUP  000070
#define OTHER  000007
#define MKDIR  1
#define CREAT  2
#define SYMLINK 3

/*GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp;*/

//function that chnages directories
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

//list all files at location specified
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
  for(int i = 0; i<12; i++)
  {
    if(mip->INODE.i_block[i]==0)
      break;

    get_block(dev, mip->INODE.i_block[i], buf);

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
}

//prints all details about a file
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

//caller function for ls_file
int list_file()
{
  ls_dir(pathname);
}

//caller function for rpwd
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

//function that returns current location path recursivly
//goes up from where currently are, prints each
//inode name by finding its parents directory
//goes to root
int rpwd(MINODE *wd)
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
        if(dp2->inode == myIno)//looks for ino
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
  rpwd(pip); //goes deeper and the prints to have it in correct order

  printf("/%s", myName2);
}

//Function in charge of making dirs and normal files
int myMake(int fType)
{
  MINODE *pip;
  char dirname[256]="";
  char basename[256]="";
  
  if (strcmp(pathname, "") == 0)
  {
    printf("ERROR: needs path\n");
    return -1;
  }
  //we devide the dir name from the base name
  int n = tokenize(pathname);

  strcpy(basename, name[n-1]);
  if (pathname[0]=='/')
    strcpy(dirname, "/");
  else
  {
    strcpy(dirname, "./");
  }
  
  if(n >= 2)
  {
    strcat(dirname, name[0]);
    for(int i = 1; i < n-1; i++)
    {
      strcat(dirname,"/");
      strcat(dirname, name[i]);
    }
  }
  printf("dirname = %s basename = %s\n", dirname, basename);

  //now we get the minode for the dir and check that its a dir
  int pino = getino(dirname);
  pip = iget(dev, pino);
  if (!S_ISDIR(pip->INODE.i_mode))
  {
    printf("ERROR: %s is not a dir\n", dirname);
    return -1;
  }
  if (search(pip, basename)!=0)
  {
    printf("ERROR: %s exists in %s\n", basename, dirname);
    return -1;
  }

  mymkdircreat(pip, basename, fType);
  
  if(fType == MKDIR)//dir file
    pip->INODE.i_links_count++;
  pip->INODE.i_atime = time(0l);
  pip->dirty = 1;
  iput(pip);
  return 0;
}

//dual function that creates a dir or a file depending on ftype
//1 for dir
//2 for creat
//3 for link
int mymkdircreat(MINODE *pip, char *basename, int fType)
{
  if (strcmp(pathname, "") == 0)
  {
    printf("ERROR: needs path\n");
    return -1;
  }
  char buf[BLKSIZE];
  int ino = ialloc(dev);
  int bno = 0;
  if(fType == MKDIR)//dir file
   bno = balloc(dev);

  printf("ino = %d | bno = %d\n", ino, bno);
  //filling the inode with information
  MINODE *mip = iget(dev,ino);
  INODE *ip = &mip->INODE;

  if(fType == MKDIR)//dir file
  {
    ip->i_mode = 0x41ED;		        // OR 040755: DIR type and permissions
    ip->i_size = BLKSIZE;	          // Size in bytes
    ip->i_links_count = 2;	        // Links count=2 because of . and .. 
  }
  else//regular file with permission bits to (default) rw-r--r--
  {
    if(fType == CREAT)
      ip->i_mode = 0x81A4;          // OR 0100644: default type and permission
    else
      ip->i_mode = 0xA1A4;          // OR 0120644: symlink type and permission
    ip->i_size = 0;		              // no data block so size zero
    ip->i_links_count = 1;	        // Links count=1
  }

  ip->i_uid  = running->uid;	      // Owner uid 
  ip->i_gid  = running->gid;	      // Group Id
  
  ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);  // set to current time
  if(fType == MKDIR)//dir file
  {
    ip->i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks 
    ip->i_block[0] = bno;             // new DIR has one data block   
  }
  else
  {
    ip->i_blocks = 0;                	// LINUX: Blocks count in 512-byte chunks 
    ip->i_block[0] = 0;               // new file has zero blocks
  }
  
  for(int i = 1; i < 15; i++)
    ip->i_block[i] = 0;
 
  mip->dirty = 1;               // mark minode dirty
  iput(mip);                    // write INODE to disk

  if(fType == MKDIR)//dir file
  {
    // write the . and .. entries into a buf[] of BLOCK_SIZE
    memset(buf, 0, BLKSIZE); //set whole buffer to zeroes
    dp = (DIR *)buf;
    
    //filling in entry of "."
    dp->inode = ino;        // inode number
    //12 because IDEAL_LEN
    dp->rec_len = 12;       // this entry’s length in bytes
    dp->name_len = 1;       // name length in bytes
    strcpy(dp->name, ".");  // name: 1-255 chars

    char *cp = buf + dp->rec_len;
    dp = (DIR *)cp;

    //filling in entry of ".."
    dp->inode = pip->ino;
    dp->rec_len = BLKSIZE - 12;
    dp->name_len = 2;
    strcpy(dp->name, "..");

    //puts block into mem
    put_block(pip->dev, bno, buf);
  }
  //getchar();
  enterName(pip, ino, basename);

  return ino;
}

//function that adds the name of the file to parent
int enterName(MINODE *pip, int myino, char *myname)
{
  char buf[BLKSIZE], *cp;
  int IDEAL_LEN, need_length, remain, i;
  need_length = 4*((8 + strlen(myname) + 3)/4);

  ip = &pip->INODE;
  for(i = 0; i < 12; i++)
  {
    if (ip->i_block[i] == 0)
    {
      printf("Placing \"%s\" in new block\n", myname);
      int db = balloc(pip->dev);
      ip->i_block[i] = db;
      get_block(pip->dev, db, buf);
      ip->i_size += BLKSIZE;//size of file in bytes

      dp = (DIR *)buf;
      dp->rec_len = BLKSIZE;
      break;
    }

    get_block(dev, ip->i_block[i], buf);
    dp = (DIR *)buf;
    cp = buf;

    while(cp + dp->rec_len < buf + BLKSIZE)
    {
      /****** Technique for printing, compare, etc.******
      c = dp->name[dp->name_len];
      dp->name[dp->name_len] = 0;
      printf("%s ", dp->name);
      dp->name[dp->name_len] = c;*/
      cp += dp->rec_len;
      dp = (DIR *)cp; 
    }
    IDEAL_LEN = 4*((8 + dp->name_len + 3)/4);
    //IDEAL_LEN = 400;
    remain = dp->rec_len - IDEAL_LEN;
    
    if(remain >= need_length)
    {
      printf("Placing \"%s\" in old block\n", myname);
      dp->rec_len = IDEAL_LEN;
      cp += IDEAL_LEN;
      dp = (DIR *)cp;
      dp->rec_len = remain;
      break;
    }
  }

  dp->name_len = strlen(myname);
  strcpy(dp->name, myname);
  dp->inode = myino;
  put_block(pip->dev, pip->INODE.i_block[i], buf);
}

//function for removing dir's
int myrmdir()
{
  if (strcmp(pathname, "") == 0)
  {
    printf("ERROR: needs path\n");
    return -1;
  }
  //get inumber of pathname
  int ino = getino(pathname), pino = 0;
  char temp[256] = "", buf[BLKSIZE], tName[256] = "";
  divide(pathname);
  char bname[256];
  strcpy(bname, mybasename);

  printf("Deleting from: %s\n", bname);
  if(strcmp(bname, ".") == 0 || strcmp(bname, "..") == 0)
  {
    printf("ERROR: Not a good idea to delete %s, not allowed\n", bname);
    return -1;
  }

  //get its minode[ ] pointer
  MINODE *mip = iget(dev, ino);

  //checking ownership, super user ok, not super user, uid should match
  if(running->uid == mip->INODE.i_uid || running->uid == 0)
  {
    //all is good just leaves if
    printf("User uid:%d\n", running->uid);
  }
  else
  {
    printf("ERROR: can not be accesed by current user\n");
    return -1;
  }

  //checks dir type
  if (!S_ISDIR(mip->INODE.i_mode))
  {
    printf("ERROR: %s is not a dir\n", bname);
    return -1;
  }

  //checking if busy using link count
  if (mip->INODE.i_links_count > 2)
  {
    printf("ERROR: %s is BUSY!\n", bname);
    return -1;
  }

  //checking that the dir is empty
  int eCount = 0;
  for(int i = 0; i<12; i++)
  {
    if(mip->INODE.i_block[i]==0)
      break;

    get_block(dev, mip->INODE.i_block[i], buf);

    dp = (DIR *)buf;
    char *cp = dp;

    while(cp < buf + BLKSIZE){
      strncpy(tName, dp->name, dp->name_len);
      tName[dp->name_len] = 0;

      if(strcmp(tName,".") != 0)
      {
        if(strcmp(tName,"..") != 0)
        {
          printf("ERROR: %s is not empty!\n", bname);
          return -1;
        }
        else
        {
          pino = dp->inode; //save parents ino for later
        }
        
      }

      eCount++;
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
  }
  printf("Number of Entries: %d\n", eCount);
  if (eCount > 2)
  {
    printf("ERROR: %s is not empty!\n", bname);
    return -1;
  }

  //Deallocate its block and inode
  for (int i=0; i<12; i++){
      if (mip->INODE.i_block[i]==0)
          continue;
      bdealloc(mip->dev, mip->INODE.i_block[i]);
  }
  idealloc(mip->dev, mip->ino);
  iput(mip); //which clears mip->refCount = 0

  //get parent DIR's ino and Minode (pointed by pip)
  MINODE *pip = iget(mip->dev, pino);

  rm_child(pip, bname);

  pip->INODE.i_links_count--;
  pip->INODE.i_atime = time(0l);
  pip->INODE.i_mtime = time(0l);
  pip->dirty = 1;
  iput(pip);
  return 0;
}

//helper function to devide paths
int divide(char *mypath)
{
  //we devide the dir name from the base name
  int n = tokenize(mypath);
  strcpy(mybasename, name[n-1]);
  if (mypath[0]=='/')
    strcpy(mydirname, "/");
  else
  {
    strcpy(mydirname, "./");
  }
  
  if(n >= 2)
  {
    strcat(mydirname, name[0]);
    for(int i = 1; i < n-1; i++)
    {
      strcat(mydirname,"/");
      strcat(mydirname, name[i]);
    }
  }
  printf("dirname = %s basename = %s\n", mydirname, mybasename);
}

//removes child from parent
int rm_child(MINODE *parent, char *name)
{
  char buf[BLKSIZE], tName[256], buf2[BLKSIZE], temp[BLKSIZE];
  ip = &parent->INODE;

  for(int i = 0; i<12; i++)
  {
    if(ip->i_block[i]==0)
      break;

    get_block(dev, ip->i_block[i], buf);

    dp = (DIR *)buf;
    DIR *prevdp = NULL;
    char *cp = dp;

    while(cp < buf + BLKSIZE){
      strncpy(tName, dp->name, dp->name_len);
      tName[dp->name_len] = 0;

      printf("%s-->%d\n", tName, dp->rec_len);
      if(strcmp(tName, name)==0)
      {
        char *nextcp = cp + dp->rec_len;
        int rmLen = dp->rec_len;
        if(dp->rec_len == BLKSIZE)//if only entry in block
        {
          printf("\"%s\" is the only entry in block\n", name);
          bdealloc(parent->dev, ip->i_block[i]);
          memset(temp, 0, BLKSIZE);
          put_block(parent->dev, ip->i_block[i], temp);
          ip->i_block[i] = 0;
          ip->i_size -= BLKSIZE;//size of file in bytes
        }
        else if(nextcp >= buf + BLKSIZE)//if last entry
        {
          printf("\"%s\" is the last entry in block\n", name);
          prevdp->rec_len += dp->rec_len;
          put_block(parent->dev, ip->i_block[i], buf);
        }
        else//if somewhere in the block 
        {
          printf("\"%s\" is somewhere in block\n", name);
          DIR *nextdp = (DIR *)nextcp;
          while(nextcp < buf + BLKSIZE)
          {
            dp = (DIR *)cp;
            nextdp = (DIR *)nextcp;

            dp->inode = nextdp->inode;
            dp->rec_len = nextdp->rec_len;
            dp->name_len = nextdp->name_len;
            dp->file_type = nextdp->file_type;
            printf("%s replaced by %s\n",dp->name, nextdp->name);
            strncpy(dp->name, nextdp->name, nextdp->name_len);

            cp += nextdp->rec_len;
            nextcp += nextdp->rec_len;
          }
          printf("Before: %d\n", dp->rec_len);
          dp->rec_len += rmLen;
          printf("After: %d\n", dp->rec_len);
          put_block(parent->dev, ip->i_block[i], buf);
        }
        parent->dirty = 1;
        return 0;
      }

      cp += dp->rec_len;
      prevdp = dp;
      dp = (DIR *)cp;
    }
  }
}

//Function that creates a new link to the a file, not a dir
int myLink()
{
  if (strcmp(pathname, "") == 0 || strcmp(extra, "") == 0)
  {
    printf("ERROR: needs 2 paths\n");
    return -1;
  }
  char pName[256], lName[256], buf[BLKSIZE], buf2[BLKSIZE], *cp;
  printf("Creating Parent:%s\tLink:%s\n", pathname, extra);
  int ino = getino(pathname);
  if(ino == -1)
  {
    printf("ERROR: no such file %s\n", pathname);
    return -1;
  }
  MINODE *mip = iget(dev, ino);
  INODE *ip = &mip->INODE;

  char temp[256], *temp2, lpath[256], lfname[256], ppath[256], pfname[256];

  divide(pathname);
  strcpy(ppath, mydirname);
  strcpy(pfname, mybasename);

  divide(extra);
  strcpy(lpath, mydirname);
  strcpy(lfname, mybasename);

  //printf("File path: %s\nFile: %s\nLink path: %s\nLink: %s\n",
          //ppath, pfname, lpath, lfname);
  //checks dir type
  if (S_ISDIR(ip->i_mode))
  {
    printf("ERROR: \"%s\" is a dir, cant make a link!\n", pfname);
    return -1;
  }
  if (getino(lpath) == -1)
  {
    printf("ERROR: \"%s\" does not exist\n", lpath);
    return -1;
  }
  if (getino(extra) != -1)
  {
    printf("ERROR: \"%s\" already exists at that location\n", lfname);
    return -1;
  }
  //placing link in location identified by user
  ino = getino(lpath);
  MINODE *lmip = iget(dev, ino);
  INODE *lip = &lmip->INODE;
  int IDEAL_LEN, need_length, remain, i;
  need_length = 4*((8 + strlen(lfname) + 3)/4);
  for(i = 0; i < 12; i++)
  {
    //if new blovk needs to be allocated uses this
    if (lip->i_block[i] == 0)
    {
      printf("Placing \"%s\" in new block\n", lfname);
      int db = balloc(lmip->dev);
      lip->i_block[i] = db;
      get_block(mip->dev, db, buf);
      lip->i_size += BLKSIZE;//size of file in byte

      dp = (DIR *)buf;
      dp->rec_len = BLKSIZE;
      break;
    }

    get_block(lmip->dev, lip->i_block[i], buf);
    dp = (DIR *)buf;
    cp = buf;

    while(cp + dp->rec_len < buf + BLKSIZE)
    {
      cp += dp->rec_len;
      dp = (DIR *)cp; 
    }
    IDEAL_LEN = 4*((8 + dp->name_len + 3)/4);
    remain = dp->rec_len - IDEAL_LEN;
    
    //if can be placed in all block uses this 
    if(remain >= need_length)
    {
      printf("Placing \"%s\" in old block\n", lfname);
      dp->rec_len = IDEAL_LEN;
      cp += IDEAL_LEN;
      dp = (DIR *)cp;
      dp->rec_len = remain;
      break;
    }
  }
  dp->name_len = strlen(lfname);
  strcpy(dp->name, lfname);
  dp->inode = mip->ino;
  //dp->file_type = set;
  put_block(lmip->dev, lip->i_block[i], buf);

  ip->i_links_count++;
  iput(mip);
}

//unlinks a hardlink from a file in memory
int myUnLink()
{
  if (strcmp(pathname, "") == 0)
  {
    printf("ERROR: needs a path\n");
    return -1;
  }
  char pName[256], lName[256], buf[BLKSIZE], buf2[BLKSIZE], *cp;
  printf("Deleting link:%s\n", pathname);
  int ino = getino(pathname);
  if(ino == -1)
  {
    printf("ERROR: no such file %s\n", pathname);
    return -1;
  }
  MINODE *mip = iget(dev, ino);
  INODE *ip = &mip->INODE;

  char temp[256], *temp2, lpath[256], lfname[256];

  divide(pathname);
  strcpy(lpath, mydirname);
  strcpy(lfname, mybasename);

  //checks dir type
  if (S_ISDIR(ip->i_mode))
  {
    printf("ERROR: \"%s\" is a dir, cant unlink!\n", lfname);
    return -1;
  }
  ip->i_links_count--;
  //if no more connections to file removes it completely
  if(ip->i_links_count == 0)
  {
    truncate(ip);
    idealloc(dev, ino);
  }
  divide(pathname);
  ino = getino(mydirname);
  MINODE *pmip = iget(dev, ino);
  rm_child(&pmip->INODE, mybasename);
}

//helper function for link that gets rid of blocks
int truncate(INODE *ip)
{
  for(int i = 0; i < 12; i++)
    {
      if(ip->i_block[i] != 0)
        bdealloc(dev, ip->i_block[i]);
    }
  if(ip->i_block[12]!=0)
  {
      char sbuf[BLKSIZE];
      int blkNum;
      int *cp;
      printf("----------- TRUNCATING INDIRECT BLOCKS ---------------\n");

      get_block(dev, ip->i_block[12], sbuf);
      cp = sbuf;
      while(cp < sbuf + BLKSIZE){
          blkNum = *cp;
          if (blkNum != 0)
            bdealloc(dev, blkNum);
          cp += 1;
      }
  }
  if(ip->i_block[13]!=0)
  {
      char sbuf[BLKSIZE], ssbuf[BLKSIZE];
      int blkNum, bblkNum;
      int *cp, *ccp;
      printf("----------- TRUNCATING DOUBLE INDIRECT BLOCKS ---------------\n");
      get_block(dev, ip->i_block[13], sbuf);
      cp = sbuf;
      while(cp < sbuf + BLKSIZE){
          blkNum = *cp;
          if (blkNum == 0)
              break;
          get_block(dev, blkNum, ssbuf);
          ccp = ssbuf;
          while(ccp < ssbuf + BLKSIZE){
              bblkNum = *ccp;
              if (bblkNum != 0)
                bdealloc(dev, bblkNum);
              ccp += 1;
          }
          cp += 1;
      }
  }
  printf("Done Truncating\n");
  return 0;
}

//creates a sym link file by placing the path of the file as the blocks
int mySymLink()
{
  if (strcmp(pathname, "") == 0 || strcmp(extra, "") == 0)
  {
    printf("ERROR: needs 2 paths\n");
    return -1;
  }
  char pName[256], lName[256], buf[BLKSIZE], buf2[BLKSIZE], *cp;
  printf("Creating Parent:%s\tLink:%s\n", pathname, extra);
  int ino = getino(pathname);
  if(ino == -1)
  {
    printf("ERROR: no such file %s\n", pathname);
    return -1;
  }
  MINODE *mip = iget(dev, ino);
  INODE *ip = &mip->INODE;

  char temp[256], *temp2, lpath[256], lfname[256], ppath[256], pfname[256];

  divide(pathname);
  strcpy(ppath, mydirname);
  strcpy(pfname, mybasename);

  divide(extra);
  strcpy(lpath, mydirname);
  strcpy(lfname, mybasename);

  //printf("File path: %s\nFile: %s\nLink path: %s\nLink: %s\n",
          //ppath, pfname, lpath, lfname);
  if (getino(lpath) == -1)
  {
    printf("ERROR: \"%s\" does not exist\n", lpath);
    return -1;
  }
  if (getino(extra) != -1)
  {
    printf("ERROR: \"%s\" already exists at that location\n", lfname);
    return -1;
  }

  int lino = getino(lpath);
  MINODE *lmip = iget(dev, lino);

  int fino = mymkdircreat(lmip, lfname, SYMLINK);
  MINODE *lfmip = iget(dev, fino);
  INODE *lfip = &lfmip->INODE;
  
  //takes the iblocks as a char and then copies the path into it
  cp =(char *)lfip->i_block;
  strcpy(cp, pathname);
  lfip->i_size = strlen(pathname);
  lfmip->dirty = 1;
  iput(lfmip);

  lmip->dirty = 1;
  iput(lmip);

  return 0;
}

//reads symlink file
int myReadLink(char *buf)
{
  if (strcmp(pathname, "") == 0)
  {
    printf("ERROR: needs a path\n");
    return -1;
  }
  int ino = getino(pathname);
  MINODE *mip = iget(dev, ino);
  INODE *ip = &mip->INODE;
  if (!S_ISLNK(ip->i_mode))
  {
    printf("ERROR: not a link file!\n");
    return -1;
  }
  strcpy(buf, (char *)ip->i_block);
  return ip->i_size;
}

//stat : get file status information
int myStat()
{
  if (strcmp(pathname, "") == 0)
  {
    printf("ERROR: needs a path\n");
    return -1;
  }
  int ino = getino(pathname);
  if(ino == -1)
  {
    printf("ERROR: file or dir doesn't exist\n");
    return -1;
  }
  MINODE *mip = iget(dev, ino);
  INODE *ip = &mip->INODE;
  divide(pathname);
  printf("  File: %s\n", mybasename);
  printf("  Size: %d\t\tBlocks: %d\t\tIO Block: %d\t", ip->i_size, ip->i_blocks, BLKSIZE);
  if(S_ISDIR(ip->i_mode))
    printf("directory\n");
  else if(S_ISREG(ip->i_mode))
    printf("regular file\n");
  else if(S_ISLNK(ip->i_mode))
    printf("link\n");
  printf("Device: %s\t\tInode: %d\t\tLniks: %d\n", disk, mip->ino, ip->i_links_count);
  printf("Access: (" );
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
  printf(")\tUid: (%d)\t\tGid: (%d)\n", ip->i_uid, ip->i_gid);
  printf("Access: %s\n", ctime(&ip->i_atime));
  printf("Modify: %s\n", ctime(&ip->i_mtime));
  printf("Change: %s\n", ctime(&ip->i_ctime));
  return 0;
}

//chmod : change permissions of a file
//Here the digits 7, 5, and 4 each individually represent the
//permissions for the user, group, and others, in that order.
//Each digit is a combination of the numbers 4, 2, 1, and 0:
//4 --> "read",
//2 --> "write",
//1 --> "execute"
//0 --> "no permission."
int myChmod()
{
  if (strcmp(pathname, "") == 0)
  {
    printf("ERROR: needs a permision type (3 digits)\n");
    return -1;
  }
  if (strcmp(extra, "") == 0)
  {
    printf("ERROR: needs a path\n");
    return -1;
  }
  int ino = getino(extra);
  int perm;
  if(ino == -1)
  {
    printf("ERROR: file or dir doesn't exist\n");
    return -1;
  }
  if(strlen(pathname) != 3)
  {
    printf("ERROR: %s has to be three digits long!\n", pathname);
    return -1;
  }
  MINODE *mip = iget(dev, ino);
  INODE *ip = &mip->INODE;

  //scans what the user want to change it to
  sscanf(pathname, "%x", &perm);
  //empties end(permisions)
  ip->i_mode &= 0777000;
  //adds new permisions
  ip->i_mode += perm; 

  mip->dirty = 1;
  iput(mip);

  return 0;
}

//touch filename: change filename timestamp (create file if it
//does not exist)
int myTouch()
{
  if (strcmp(pathname, "") == 0)
  {
    printf("ERROR: needs a path\n");
    return -1;
  }
  int ino = getino(pathname);
  divide(pathname);
  if(ino == -1) //checks if file exists if doesnt, makes one
  {
    printf("file or dir doesn't exist, creating file %s\n", mybasename);
    int pino = getino(mydirname);
    if (pino == -1)
      {
        printf("ERROR: such a path doesnt exist(%s)\n", mydirname);
        return -1;
      }
    MINODE *mip = iget(dev, pino);
    INODE *ip = &mip->INODE;
    mymkdircreat(mip, mybasename, CREAT);
    mip->dirty = 1;
    iput(mip);
    return 0;
  }
  MINODE *mip = iget(dev, ino);
  INODE *ip = &mip->INODE;

  //touches the time
  ip->i_atime = ip->i_mtime = time(0l);

  mip->dirty = 1;
  iput(mip);

  return 0;
}