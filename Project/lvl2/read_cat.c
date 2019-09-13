/************* read_cat.c file **************/

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
extern int writePrint, readPrint;

#define MKDIR  1
#define CREAT  2
#define SYMLINK 3

//reads file
int myReadFile()
{
    /*Preparations: 
    ASSUME: file is opened for RD or RW;
    ask for a fd  and  nbytes to read;
    verify that fd is indeed opened for RD or RW;
    return(myread(fd, buf, nbytes));*/
    //gets fd
    pathname[0] = 0;
    printf("Please input fd to read:\n");
    fgets(pathname, 128, stdin);
    pathname[strlen(pathname)-1] = 0;
    int fd = atoi(pathname);
    if(fd < 0 || fd >= NFD || running->fd[fd] == 0)
    {
        printf("ERROR: not possible to retrieve fd: %d\n", fd);
        return -1;
    }
    if(running->fd[fd]->mode != 0 && running->fd[fd]->mode != 2)
    {
        printf("ERROR: not possible to read: %d; OPEN IN WRONG MODE!\n", fd);
        return -1;
    }
    
    printf("Please input number of bytes to read:\n");
    fgets(extra, 128, stdin);
    pathname[strlen(extra)-1] = 0;
    int nBytes = atoi(extra);

    char buf[nBytes];

    return myRead(fd, buf, nBytes);
}

//reads number fbytes into buf
int myRead(int fd, char *buf, int nbytes)
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

    while (nbytes && avil){
       //Computing LOGICAL BLOCK number lbk and startByte in that block from offset;
        int lbk       = oftp->offset / BLKSIZE;
        int startByte = oftp->offset % BLKSIZE;
     
       //reading direct blocks
        if (lbk < 12)
        {                     // lbk is a direct block
            blk = mip->INODE.i_block[lbk]; // map LOGICAL lbk to PHYSICAL blk
        }
        else if (lbk >= 12 && lbk < 256 + 12)
        {
            get_block(mip->dev, ip->i_block[12], ibuf);
            blk = ibuf[lbk-12];
        }
        else{ 
            get_block(mip->dev, ip->i_block[13], ibuf);
            get_block(mip->dev, ibuf[(lbk-(256+12))/256], itbuf);
            blk = ibuf[lbk-(256+12)%256];
        } 

        // get the data block into readbuf[BLKSIZE]
        char readbuf[BLKSIZE];
        get_block(mip->dev, blk, readbuf);

        /* copy from startByte to buf[ ], at most remain bytes in this block */
        char *cp = readbuf + startByte;   
        int remain = BLKSIZE - startByte;   // number of bytes remain in readbuf[]

        int premain = remain;
        if(nbytes < premain)
            premain = nbytes;
        if(avil < premain)
            premain = avil;
        
        if(readPrint == 1)
            printf("Reading block %d [%d] from %d, size: %d\n", lbk, blk, startByte, premain);
        
        strncpy(cq,cp,premain);         // copy byte from readbuf[] into buf[]
        cq += premain;                  
        oftp->offset += premain;        // advance offset 
        count += premain;               // inc count as number of bytes read
        avil -= premain;
        nbytes -= premain;
        remain -= premain;
        /*while (remain > 0)
        {
            *cq++ = *cp++;            // copy byte from readbuf[] into buf[]
            oftp->offset++;           // advance offset 
            count++;                  // inc count as number of bytes read
            avil--;
            nbytes--;
            remain--;
            if (nbytes <= 0 || avil <= 0) 
                break;
        }*/
        //if one data block is not enough, loops back to OUTER while for more

    }
    if(readPrint == 1)
        printf("myread: read %d char from file descriptor %d\n", count, fd);  
    return count;   // count is the actual number of bytes read
}

//cat function
int myCat()
{
    char mybuf[1024], dummy = 0;  //a null char at end of mybuf[ ]
    int n;
    char mode[256]; 
    strcpy(mode, "R");
    int fd = myOpen(pathname, &mode);

    readPrint = 0;
    while(n = myRead(fd, &mybuf, 1024))
    {
        mybuf[n] = 0;
        //printf("%s", mybuf);
        for(int i = 0; i<strlen(mybuf); i++)
        {   
            if(mybuf[i] != '\n')
                putchar(mybuf[i]);
            else
            {
                putchar('\r');
                putchar('\n');
            }
        }
    }
    readPrint = 1;

    myClose(fd);
}