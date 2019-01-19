#include <stdio.h>
#include <fcntl.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
int count;
int offset;
int os;

struct partition
{
    u8 drive; /* drive number FD=0, HD=0x80, etc. */

    u8 head;     /* starting head */
    u8 sector;   /* starting sector */
    u8 cylinder; /* starting cylinder */

    u8 sys_type; /* partition type: NTFS, LINUX, etc. */

    u8 end_head;     /* end head */
    u8 end_sector;   /* end sector */
    u8 end_cylinder; /* end cylinder */

    u32 start_sector; /* starting sector counting from 0 */
    u32 nr_sectors;   /* number of of sectors in partition */
};

int main()
{
    char buf[512];
    int fd = open("vdisk", O_RDONLY);
    count = 1; offset = 0; os = 0;
    read(fd, buf, 512);

    struct partition *p = (struct partition *)&buf[0x1BE];
    printf("Device\tBoot Start\tEnd\tSectors\tSize\tID\n");
    for (int i = 0; i < 4; i++, p++)
    {
        if(p->sys_type == 5)
        {
            printVDisk(p);
            count++;
            printf("Extended Disk Partisions Start\n");
            extVDisk(p, fd, buf);
            printf("Extended Disk Partisions End\n");
        }
        else
        {
            printVDisk(p);
            count++;
        }
    }

    return 0;
}

void printVDisk(struct partition *p)
{
    printf("vdisk%d\t", count);
    printf("%d\t\t%d\t%d\t%dk\t%x\n", p->start_sector, p->start_sector + p->nr_sectors - 1, p->nr_sectors, p->nr_sectors/2, p->sys_type);
}

void printVDiskExt(struct partition *p, u32 n)
{
    printf("vdisk%d\t", count);
    printf("%d\t\t%d\t%d\t%dk\t%x\n", n + p->start_sector, n + p->start_sector + p->nr_sectors - 1, p->nr_sectors, p->nr_sectors/2, p->sys_type);
}

void extVDisk(struct partition *p, int fd, char *buf)
{
    struct partition *q = p; int n;
    if(os == 0)
    {
        offset = p->start_sector;
        os = 1;
        n = offset;
    }
    else
    {
        n = offset + p->start_sector;
    }
    lseek(fd, (long)(n * 512), SEEK_SET);
    read(fd, buf, 512);
    q = (struct partition *)&buf[0x1BE];
    printVDiskExt(q, n);
    q++; count++;
    if(q->sys_type == 5 && count < 20)
    {
        extVDisk(q, fd, buf);
        os = 0;
    }
}