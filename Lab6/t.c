#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ext2fs/ext2_fs.h>
#define BLKSIZE 1024

typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;
typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc GROUPDESC;
typedef struct ext2_inode INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GROUPDESC *gd;
INODE *ip;

char ibuf[BLKSIZE];
char *dirNames[64]={ NULL };

// get_block() reads a disk BLOCK into a char buf[BLKSIZE].   
int get_block(int dev, int blk, char *buf)
{   
    lseek(dev, blk*BLKSIZE, SEEK_SET);
    int rd = read(dev, buf, BLKSIZE);
    if(rd == -1)
    {
        printf("\tERROR: Block couldnt be read!\n\n");
        exit(EXIT_FAILURE);
    }
    //printf("\tBlock has been read\n\n");
    return rd;
}

void sl(void)
{
    printf("***********************************************************\n");
}

int search(INODE *ip, char *name, int dev)
{
    char sbuf[BLKSIZE], temp[256];
    DIR *dp;
    char *cp;

    for (int i=0; i < 12; i++){  // assume DIR at most 12 direct blocks
        if (ip->i_block[i] == 0)
            break;
        // YOU SHOULD print i_block[i] number here
        printf("i=%d i_block[0]=%d\n",ip->i_uid ,ip->i_block[i]);
        get_block(dev, ip->i_block[i], sbuf);

        dp = (DIR *)sbuf;
        cp = sbuf;

        while(cp < sbuf + BLKSIZE){
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            printf("%4d\t%4d\t%4d\t\t%s\n", 
                dp->inode, dp->rec_len, dp->name_len, temp);
            if (strcmp(temp, name)==0)
                return dp->inode;

            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    return 0;
}

show_dir(INODE *ip, int dev)
{
    char sbuf[BLKSIZE], temp[256];
    DIR *dp;
    char *cp;

    for (int i=0; i < 12; i++){  // assume DIR at most 12 direct blocks
        if (ip->i_block[i] == 0)
            break;
        // YOU SHOULD print i_block[i] number here
        printf("root inode data block = %d\n", ip->i_block[i]);
        get_block(dev, ip->i_block[i], sbuf);

        dp = (DIR *)sbuf;
        cp = sbuf;

        printf("i_number rec_len name_len\tname\n");
        while(cp < sbuf + BLKSIZE){
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            printf("%4d\t%4d\t%4d\t\t%s\n", 
                dp->inode, dp->rec_len, dp->name_len, temp);

            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
}

//tokenizer function
int tokenize(char *pathname)
{
    for (int i = 0; i < 64; i++)    //sets all values in array to null so a new path can be interpreted
    {
        dirNames[i] = NULL;
    }
    dirNames[0] = strtok(pathname, "/"); // first call to strtok()
    int i = 1;
    for (; dirNames[i-1]; i++)
    {
        dirNames[i] = strtok(0, "/"); // calling strtok() until it returns NULL
    }
    return i - 1;
}
 
int main(int argc, char *argv[], char *env[])
{
    char buf[BLKSIZE];
    printf("\n\n\n");
    
    sl();
    int dev = open("diskimage", O_RDONLY);
    if(dev == -1)
    {
        printf("ERROR: File couln't be open!\n");
        return -1;
    }
    printf("(1). File opened succesfully\n");

    get_block(dev, 1, buf);

    sl();
    sp = (SUPER *)buf;
    // check EXT2 FS magic number:
    //printf("%-30s = %8x \n\n", "s_magic", sp->s_magic);
    if (sp->s_magic != 0xEF53){
        printf("ERROR: NOT an EXT2 FS\n");
        return -1;
    }
    printf("(2). Verify it's an ext2 file system: OK\n\n");

    sl();
    printf("(3). Read group descriptor 0 to get bmap imap inodes_start\n");
    get_block(dev, 2, buf);
    gd = (GROUPDESC *)buf;

    printf("bmap = %u32 ", gd->bg_block_bitmap);
    printf("imap = %u32 ", gd->bg_inode_bitmap);
    printf("inodes_start = %u32\n", gd->bg_inode_table);
    sl();
    printf("*********** get root inode *************\n(3). Show root DIR contents\n");
    get_block(dev, gd->bg_inode_table, ibuf);
    ip = (INODE *)ibuf + 1;
    show_dir(ip, dev);
    sl();

    
    /********************************************/
    int ino, blk, offset;
    printf("%s\n",argv[1]);
    int n = tokenize(argv[1]);
    printf("Number of dirs: %d\n", n);

    for(int j = 0; dirNames[j]!=NULL; j++)
        printf("%s\t",dirNames[j]);
    printf("\n");

    for (int i=0; i < n; i++)
    {
        printf("Searching for %s in %x\n", dirNames[i], ip->i_block);
        ino = search(ip, dirNames[i], dev);

        if (ino==0)
        {
            printf("can't find %s\n", dirNames[i]); 
            exit(1);
        }
        printf("found %s : ino = %d\n", dirNames[i], ino);

        // Mailman's algorithm: Convert (dev, ino) to INODE pointer
        blk    = (ino - 1) / 8 + gd->bg_inode_table;
        offset = (ino - 1) % 8;        
        get_block(dev, blk, ibuf);
        ip = (INODE *)ibuf + offset;   // ip -> new INODE
    }

    printf("ino = %d\n", ino);
    printf("size = %d\n", ip->i_size);
    for(int i = 0; i < 15; i++)
    {
        printf("i_block[%d] = %d\n", i, ip->i_block[i]);
    }
    if(ip->i_block[12]!=0)
    {
        char sbuf[BLKSIZE];
        int blkNum;
        int *cp;
        printf("----------- INDIRECT BLOCKS ---------------\n");

        get_block(dev, ip->i_block[12], sbuf);
        cp = sbuf;
        while(cp < sbuf + BLKSIZE){
            blkNum = *cp;
            if (blkNum == 0)
                break;
            printf("%d ", blkNum);
            cp += 1;
        }
        printf("\n");
    }
    if(ip->i_block[13]!=0)
    {
        char sbuf[BLKSIZE], ssbuf[BLKSIZE];
        int blkNum, bblkNum;
        int *cp, *ccp;
        printf("----------- DOUBLE INDIRECT BLOCKS ---------------\n");
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
                if (bblkNum == 0)
                    break;
                printf("%d ", bblkNum);
                ccp += 1;
            }
            cp += 1;
        }
        printf("\n");
    }


    printf("\n\n\n");
    return 0;
}