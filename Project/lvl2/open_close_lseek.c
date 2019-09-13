/************* open_close_lseek.c file **************/

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
extern char mydirname[256], mybasename[256];

#define MKDIR  1
#define CREAT  2
#define SYMLINK 3

int myOpenFile()
{
    return myOpen(pathname, extra);
}

//functions that is used for opening files
int myOpen(char *filename, char *smode)
{
    int mode = 0;

    /*//gets path name to file
    pathname[0] = 0;
    printf("Please input path to file:\n");
    fgets(pathname, 128, stdin);
    pathname[strlen(pathname)-1] = 0;
    
    //gets how user wants to open file
    extra[0] = 0;
    printf("Please input R|W|RW|APPEND (how to open):\n");
    fgets(extra, 128, stdin);
    extra[strlen(extra)-1] = 0;*/
    for(int i = 0; i < strlen(smode); i++)
    {
        if(smode[i] <= 90)
        {
            smode[i] += 32;
        }
    }

    if(strcmp(smode, "r") == 0)
    {
        printf("MODE SET TO: R\n");
        mode = 0;
    }
    else if(strcmp(smode, "w") == 0)
    {
        printf("MODE SET TO: W\n");
        mode = 1;
    }
    else if(strcmp(smode, "rw") == 0)
    {
        printf("MODE SET TO: RW\n");
        mode = 2;
    }
    else if(strcmp(smode, "append") == 0)
    {
        printf("MODE SET TO: APPEND\n");
        mode = 3;
    }
    else
    {
        printf("ERROR: no such mode exists!\n");
        return -1;
    }

    if (filename[0]=='/')
        dev = root->dev;    // root INODE's dev
    else
        dev = running->cwd->dev;    //current running INODE's dev
    int ino = getino(filename);
    if(ino == -1)
    {
        if (mode > 0)
        {
            divide(filename);
            int pino = getino(mydirname);
            if (pino == -1)
            {
                printf("ERROR: %s dir doesn't exists!\n", mydirname);
                return -1;
            }
            MINODE *pmip = iget(dev, pino);
            mymkdircreat(pmip, mybasename, CREAT);
        }
        else
        {
            printf("ERROR: no such file exists for reading!\n");
            return -1;
        }
    }
    ino = getino(filename);
    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    
    //checking if it is a reg file
    if (!S_ISREG(ip->i_mode))
    {
        printf("ERROR: \"%s\" is not a regular file!\n", filename);
        return -1;
    }
    
    //acc[r,w] checks what kind of permission user has
    int acc[2] = {0,0};
    if (running->uid == ip->i_uid) //checks if current user is owner
    {
        if (ip->i_mode & (1 << 8)) acc[0] = 1;
        if (ip->i_mode & (1 << 7)) acc[1] = 1;
    }
    else if (running->gid == ip->i_gid) //checks if current user is in same group as owner
    {
        if (ip->i_mode & (1 << 5)) acc[0] = 1;
        if (ip->i_mode & (1 << 4)) acc[1] = 1;
    }
    else //otherwise "other"
    {
        if (ip->i_mode & (1 << 2)) acc[0] = 1;
        if (ip->i_mode & (1 << 1)) acc[1] = 1;
    }
    
    for(int i = 0; running->fd[i] != 0; i++)
    {
        if (running->fd[i]->mptr->ino == ino)
        {
            if(mode > 0)
            {
                printf("ERROR: File ALREADY opened with INCOMPATIBLE mode!\n");
                return -1;
            }
        }
    }

    if ((mode == 0 || mode == 2) && acc[0] == 0)
    {
        printf("ERROR: read mode is not allowed!\n");
        return -1;
    }
    if ((mode == 1 || mode == 2 || mode == 3) && acc[1] == 0)
    {
        printf("ERROR: write mode is not allowed!\n");
        return -1;
    }

    OFT *oftp = (OFT *)malloc(sizeof(OFT));

    oftp->mode = mode;      // mode = 0|1|2|3 for R|W|RW|APPEND 
    oftp->refCount = 1;
    oftp->mptr = mip;  // point at the file's minode[]

    switch(mode)
    {
        case 0:
            oftp->offset = 0;     // R: offset = 0
            break;
        case 1:
            truncate(mip);        // W: truncate file to 0 size
            oftp->offset = 0;
            break;
        case 2: 
            oftp->offset = 0;     // RW: do NOT truncate file
            break;
        case 3:
            oftp->offset =  mip->INODE.i_size;  // APPEND mode
            break;
    }

    int i = 0;
    while(running->fd[i] != 0 && i < NFD) i++;
    if (i >= NFD)
    {
        printf("ERROR: out of fd's!\n");
        return -1;
    }
    running->fd[i] = oftp;

    if (mode == 0)
        //touches the time
        ip->i_atime = time(0l);
    else
        ip->i_atime = ip->i_mtime = time(0l); //touches the time
    
    mip->dirty = 1;
    
    fd = i;
    printf("FD: [%d]\n", fd);
    return fd;
}

//closses a file
int myClose(int fd)
{
    if(fd >= 8)
    {
        printf("ERROR: fd out of range\n");
        return -1;
    }
    if(running->fd[fd] == 0)
    {
        printf("ERROR: nothing at fd: %d\n", fd);
        return -1;
    }
    OFT *oftp = running->fd[fd];
    running->fd[fd] = 0;
    oftp->refCount--;
    if (oftp->refCount > 0) return 0;

    // last user of this OFT entry ==> dispose of the Minode[]
    MINODE *mip = oftp->mptr;
    iput(mip);
    return 0; 
}

int myLseek(int fd, int position)
{
    if(fd < 0 || fd >= NFD || running->fd[fd] == 0)
    {
        printf("ERROR: not possible to retrieve fd: %d\n", fd);
        return -1;
    }
    OFT *oftp = running->fd[fd];
    int origPos = running->fd[fd]->offset;
    if(position<0 || position>=running->fd[fd]->mptr->INODE.i_size)
    {
        printf("ERROR: attempting to seek position beyond file!\n");
        return -1;
    }
    oftp->offset = position;
    printf("Original Position: %d\n", origPos);
    return origPos;
}

//displays all current fd's
int myPfd()
{
    printf("fd\tmode\t\toffset\tINODE\n");
    printf("----\t----\t\t------\t--------\n");
    for(int i = 0; i < 8; i++)
    {
        if(running->fd[i] != 0)
        {
            printf("%d\t", i);
            int temp = running->fd[i]->mode;
            if(temp == 0)
            {
                printf("READ\t\t");
            }
            if(temp == 1)
            {
                printf("WRITE\t\t");
            }
            if(temp == 2)
            {
                printf("READWRITE\t");
            }
            if(temp == 3)
            {
                printf("APPEND\t\t");
            }
            printf("%d\t",running->fd[i]->offset);
            printf("[%d, ", running->fd[i]->mptr->dev);
            printf("%d]\n", running->fd[i]->mptr->ino);
        }
    }
    printf("----------------------------------------\n");
}

//duplicates fd into the first empy fd if such exists
int dup(int fd)
{
    if(fd < 0 || fd >= NFD || running->fd[fd] == 0)
    {
        printf("ERROR: not possible to retrieve fd: %d\n", fd);
        return -1;
    }

    OFT *oftp = running->fd[fd];

    //duplicates (copy) fd[fd] into FIRST empty fd[ ] slot;
    int i = 0;
    while(running->fd[i] != 0 && i < NFD) i++;
    if (i >= NFD)
    {
        printf("ERROR: out of fd's!\n");
        return -1;
    }
    OFT *oftpt = (OFT *)malloc(sizeof(OFT));
    running->fd[i] = oftpt;

    running->fd[i]->mode = oftp->mode;
    running->fd[i]->mptr = oftp->mptr;
    running->fd[i]->offset = oftp->offset;
    //increment OFT's refCount by 1;
    running->fd[i]->refCount++;
}

//duplicates fd into gd
int dup2(int fd, int gd)
{
    if(fd < 0 || fd >= NFD || running->fd[fd] == 0)
    {
        printf("ERROR: not possible to retrieve fd: %d\n", fd);
        return -1;
    }

    if(gd < 0 || gd >= NFD)
    {
        printf("ERROR: not possible to retrieve gd: %d\n", gd);
        return -1;
    }
    //CLOSE gd first if it's already opened
    if(running->fd[gd] != 0)
    {
        myClose(gd);
    }
    //duplicates fd[fd] into fd[gd]
    OFT *oftpt = (OFT *)malloc(sizeof(OFT));
    running->fd[gd] = oftpt;

    OFT *oftp = running->fd[fd];
    running->fd[gd]->mode = oftp->mode;
    running->fd[gd]->mptr = oftp->mptr;
    running->fd[gd]->offset = oftp->offset;
    //increment OFT's refCount by 1;
    running->fd[gd]->refCount++;
}