/*************** type.h file ************************/
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;   

#define FREE        0
#define READY       1

#define BLKSIZE  1024
#define NMINODE    64
#define NFD         8
#define NPROC       2
#define NMTABLE    10


typedef struct minode{
  INODE INODE;
  int dev, ino;
  int refCount;
  int dirty;
  // for level-3
  int mounted;
  struct mtable *mptr;
}MINODE;

//open file table
typedef struct oft{
  int  mode;
  int  refCount;
  MINODE *mptr;
  int  offset;
}OFT;

typedef struct proc{
  struct proc *next;
  int          pid;
  int          uid;
  int          gid;
  int          status;
  MINODE      *cwd;
  OFT         *fd[NFD];
}PROC;

//Mount Table structure
typedef struct mtable{
  int dev;            //device number, 0 for free
  int ninodes;        //from superblock
  int nblocks;
  int free_blocks;    //from superblock and GD
  int free_inodes;
  int bmap;           //from group descriptor
  int imap;
  int iblock;         //inode start block
  MINODE *mntDirPtr;  //mount point DIR pointer
  char devName[64];   //device name
  char mntName[64];   //mount point DIR name
}MTABLE;
