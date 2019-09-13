/************* write_cp_mv.c file **************/

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
extern int writePrint, readPrint;

#define MKDIR  1
#define CREAT  2
#define SYMLINK 3

//writes file
int myWriteFile()
{
    char buf[1024];
    int totalNBytes = 0;
    //ask for a fd and a text string to write;
    pathname[0] = 0;
    printf("Please input fd to write to:\n");
    fgets(pathname, 128, stdin);
    pathname[strlen(pathname)-1] = 0;

    int fd = atoi(pathname);

    //verify fd is indeed opened for WR or RW or APPEND mode
    if(fd < 0 || fd >= NFD || running->fd[fd] == 0)
    {
        printf("ERROR: not possible to retrieve fd: %d\n", fd);
        return -1;
    }
    if(running->fd[fd]->mode < 1)
    {
        printf("ERROR: not possible to write: %d; OPEN IN WRONG MODE!\n", fd);
        return -1;
    }

    //slightly more complicated then asked but writes lines to file until empty line
    printf("Please input lines of <1024 to write\na line with \"_eof\" ends loop:\n");
    do {
        buf[0] = 0;
        //copy the text string into a buf[] and get its length as nbytes.
        fgets(buf, 1024, stdin);
        if (strcmp(buf,"_eof\n") != 0)
        {
            int nbytes = strlen(buf);
            totalNBytes += myWrite(fd, buf, nbytes);
        }
    } while(strcmp(buf,"_eof\n") != 0);

    return totalNBytes;
}

//mywrite behaves exactly the same as Unix's write(fd, buf, nbytes) syscall.
//It writes nbytes from buf[ ] to the file descriptor fd, extending the file 
//size as needed.
int myWrite(int fd, char buf[ ], int nbytes) 
{
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->mptr;
    INODE *ip = &mip->INODE;

    int ibuf[BLKSIZE/4];
    int itbuf[BLKSIZE/4];
    int blk = 0;
    int count = 0;
    int avil = ip->i_size - oftp->offset;   // number of bytes still available in file
    char *cq = buf;                         // cq points at buf[ ]

    while (nbytes > 0 ){
        //Computing LOGICAL BLOCK number lbk and startByte in that block from offset;
        int lbk       = oftp->offset / BLKSIZE;
        int startByte = oftp->offset % BLKSIZE;

        if (lbk < 12)
        {   //writing direct blocks
            if (ip->i_block[lbk] == 0)              // if no data block yet
            {
                ip->i_block[lbk] = balloc(mip->dev);// MUST ALLOCATE a block
            }
            blk = ip->i_block[lbk];                 // blk should be a disk block now
        }
        else if (lbk >= 12 && lbk < 256 + 12){      // INDIRECT blocks 
            if (ip->i_block[12] == 0)
            {
                ip->i_block[12] = balloc(mip->dev);
                char buf[BLKSIZE];
                memset(buf, 0, BLKSIZE);
                put_block(mip->dev, ip->i_block[12], buf);
            }
            get_block(mip->dev, ip->i_block[12], ibuf);
            blk = ibuf[lbk-12];

            if (blk==0){
                //allocate a disk block record it in i_block[12];
                ibuf[lbk-12] = balloc(mip->dev);
                put_block(mip->dev, ip->i_block[12], ibuf);
            }
            blk = ibuf[lbk-12];
        }
        else
        {
            if (ip->i_block[13] == 0)
            {
                ip->i_block[13] = balloc(mip->dev);
                char buf[BLKSIZE];
                memset(buf, 0, BLKSIZE);
                put_block(mip->dev, ip->i_block[13], buf);
            }
            get_block(mip->dev, ip->i_block[13], ibuf);
            blk = ibuf[(lbk-(256+12))/256];
            if (blk == 0){
                //allocate a disk block record it in i_block[13];
                ibuf[(lbk-(256+12))/256] = balloc(mip->dev);
                char buf[BLKSIZE];
                memset(buf, 0, BLKSIZE);
                put_block(mip->dev, ip->i_block[(lbk-(256+12))/256], buf);
            }
            get_block(mip->dev, ibuf[(lbk-(256+12))/256], itbuf);
            blk = itbuf[lbk-(256+12)%256];
            if (blk == 0)
            {
                //allocate a disk block record it in i_block[13][...];
                itbuf[lbk-(256+12)%256] = balloc(mip->dev);
                put_block(mip->dev, ibuf[(lbk-(256+12))/256], itbuf);
            }
            blk = itbuf[lbk-(256+12)%256];
        }

        /* all cases come to here : write to the data block */
        char wbuf[BLKSIZE];
        get_block(mip->dev, blk, wbuf);     // read disk block into wbuf[ ]  
        char *cp = wbuf + startByte;        // cp points at startByte in wbuf[]
        int remain = BLKSIZE - startByte;   // number of BYTEs remain in this block

        int premain = remain;
        if(nbytes < premain)
            premain = nbytes;
        if(writePrint == 1)
            printf("Writing block %d [%d] from %d, size: %d\n", lbk, blk, startByte, premain);

        strncpy(cp,cq,premain);             // copy byte from buf[] into wbuf[]
        cq += premain;
        if (oftp->offset + premain > ip->i_size)      // especially for RW|APPEND mode
            ip->i_size = oftp->offset + premain;  // inc file size (if offset > fileSize)
        oftp->offset += premain;            // advance offset 
        count += premain;                   // inc count as number of bytes read
        nbytes -= premain;                  // dec counts
        remain -= premain;



        /*while (remain > 0){                 // write as much as remain allows  
            *cp++ = *cq++;                  // cq points at buf[ ]
            nbytes--; remain--;             // dec counts
            count ++;
            oftp->offset++;                 // advance offset
            if (oftp->offset > ip->i_size)  // especially for RW|APPEND mode
                ip->i_size++;               // inc file size (if offset > fileSize)
            if (nbytes <= 0) break;         // if already nbytes, break
        }*/
        put_block(mip->dev, blk, wbuf);     // write wbuf[ ] to disk
        
        // loop back to outer while to write more .... until nbytes are written
    }

    mip->dirty = 1;       // mark mip dirty for iput() 
    if(writePrint == 1)
        printf("wrote %d char into file descriptor fd=%d\n", count, fd);
    iput(mip);      
    return count;
}

//copies file from one location to the second one
myCP(char *src, char *dst)
{
    int n = 0;
    char mode[256], buf[BLKSIZE]; 
    strcpy(mode, "R");
    int fd = myOpen(src, &mode);

    strcpy(mode, "W");
    int gd = myOpen(dst, &mode);
    myPfd();
    writePrint = 0;
    readPrint = 0;
    while( n=myRead(fd, buf, BLKSIZE) )
    {
       myWrite(gd, buf, n);  // notice the n in write()
    }
    writePrint = 1;
    readPrint = 1;
    myClose(fd);
    myClose(gd);
}

myMV(char *src, char *dst)
{
    int devs, devd;
    int ino = getino2(src, &devs);
    if(ino == -1)
    {
        printf("ERROR: %s doesnt exist!\n", src);
        return ino;
    }

    divide(dst);
    int dino = getino2(mydirname, &devd);
    if(dino == -1)
    {
        printf("ERROR: %s doesnt exist!\n", mydirname);
        return dino;
    }
    if(devs == devd)
    {
        myLink();
        myUnLink();
    }
    else
    {
        myCP(src, dst);
        myUnLink();
    }
}