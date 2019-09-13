/************* mount_unmount.c file **************/

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern MTABLE mtable[NMTABLE];
extern PROC   proc[NPROC], *running;
extern char   gpath[256];
extern char   *name[64];
extern int    n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256], extra[256], temptk[256];
extern char *disk;
extern char mydirname[256], mybasename[256];

#define OWNER  000700
#define GROUP  000070
#define OTHER  000007
#define MKDIR  1
#define CREAT  2
#define SYMLINK 3

int myMount()
{
    int i = 0;
    char buf[BLKSIZE];
    /*pathname[0] = 0;
    printf("Please input pathname to file system:\n");
    fgets(pathname, 128, stdin);
    pathname[strlen(pathname)-1] = 0;
    
    extra[0] = 0;
    printf("Please input the mount point:\n");
    fgets(extra, 128, stdin);
    extra[strlen(extra)-1] = 0;*/

    if(strcmp(pathname, "") == 0 || (strcmp(extra, "") == 0))
    {
        for(i = 0; mtable[i].dev != 0 && i < NMTABLE; i++)
        {
            printf("\"%s\" mounted on: \"%s\", DEV: %d\n", mtable[i].devName, mtable[i].mntName, mtable[i].dev);
        }
        return 0;
    }
    for (i = 0; i < NMTABLE; i++)
    {
        if (strcmp(extra, mtable[i].devName) == 0)
        {
            printf("ERROR: Device already mounted!\n");
            return -1;
        }
    }
    i = 0;

    while(mtable[i].dev != 0 && i < NMTABLE) i++;
    if(i >= NMTABLE)
    {
        printf("ERROR: Cant mount anymore! Not enough space\n");
        return -1;
    }
    //setting everything to correct vars since already allocated in system on startup
    MTABLE *mp = &mtable[i];
    if ((fd = open(pathname, O_RDWR)) < 0){
        printf("ERROR: open \"%s\" failed\n", pathname);
        return -1;
    }

    //verify it's an ext2 file system
    get_block(fd, 1, buf);
    sp = (SUPER *)buf;
    if (sp->s_magic != 0xEF53){
        printf("ERROR: magic = %x is not an ext2 filesystem\n", sp->s_magic);
        return -1;
    }

    //mount_point: find its ino, then gets its minode
    int ino = getino2(extra, &dev);
    if(ino == -1)
    {
        printf("ERROR: location doesnt exist!\n");
        mp->dev = 0;
        return -1;
    }
    MINODE *mip = iget(dev, ino);

    //divide(extra);
    //checks that is is dir type
    if (!S_ISDIR(mip->INODE.i_mode))
    {
        printf("ERROR: %s is not a dir\n", extra);
        return -1;
    }

    //Check mount_point is NOT busy (e.g. can't be someone's CWD)
    for(i = 0; i < NPROC; i++)
    {
        if(proc[i].cwd == NULL)
            continue;
        if(proc[i].cwd == mip)
        {
            printf("ERROR: %s is currently in use!\n", extra);
            return -1;
        }
    }

    printf("Recording new DEV: %d in mount table\n", fd);
    mp->dev = fd;
    strcpy(mp->devName, pathname);
    strcpy(mp->mntName, extra);
    mp->ninodes = sp->s_inodes_count;
    mp->nblocks = sp->s_blocks_count;
    mp->free_blocks = sp->s_free_blocks_count;
    mp->free_inodes = sp->s_free_inodes_count;

    get_block(mp->dev, 2, buf);
    gp = (GD *)buf;
    mp->bmap = gp->bg_block_bitmap;
    mp->imap = gp->bg_inode_bitmap;
    mp->iblock = gp->bg_inode_table;

    //Mark mount_point's minode as being mounted on and let it point at the
    //MOUNT table entry, which points back to the mount_point minode.
    mip->mounted = 1;
    mip->mptr = mp;
    mp->mntDirPtr = mip;
}

int myUnMount(char *filesys)
{
    int mounted = 0, i, j, activeFiles = 0;

    //Search the MOUNT table to check filesys is indeed mounted
    for(i = 0; i < NMTABLE; i++)
    {
        if (strcmp(filesys, mtable[i].devName) == 0)
        {
            mounted = 1;
            break;
        }
    }
    if(mounted == 0)
    {
        printf("ERROR: Sytem not mounted!\n");
        return -1;
    }

    //check whether any file is still active in the mounted filesys
    for(j = 0; j < NMINODE; j++)
    {
        if(minode[j].dev == mtable[i].dev && minode[j].refCount != 0)
        {
            if(minode[j].ino != mtable[i].mntDirPtr->ino)
            {
                activeFiles = 1;
                printf("ERROR: Minode[%d] mounted on %d\n", j, mtable[i].dev);
            }
        }
    }
    if(activeFiles == 1)
    {
        printf("ERROR: %s mounted sytem has active files!\n", filesys);
        return -1;
    }

    MINODE *mip = mtable[i].mntDirPtr;
    mip->mounted = 0;
    iput(mip);
    return 0;
}