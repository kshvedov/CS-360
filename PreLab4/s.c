#include <stdio.h>
#include <stdlib.h>

#define  M   4
#define  N   500000

int A[M][N];

int total;

main(int argc, char *argv[])
{
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    int i, j, status;

    printf("main: initialize A matrix\n");

    //initializes and counts at the same time
    total = 0;
    for (i=0; i < M; i++)
    {
        int rowTot = 0;
        for (j=0; j < N; j++){
            A[i][j] = i + j + 1;
            rowTot += A[i][j];
        }
        printf("Row %d total = %d\n", i, rowTot);
        total += rowTot;
    }

    printf("\nAll rows total: %d\n", total);

    gettimeofday(&t2, NULL);

    double eSec = (t2.tv_sec - t1.tv_sec) +
                  ((t2.tv_usec - t1.tv_usec)/1000000.0);
    int sec2 = t2.tv_sec - t1.tv_sec;
    double usec2 = t2.tv_usec - t1.tv_usec;

    if (usec2 < 0)
    {
        sec2--;
        usec2 += 1000000;
    }

    printf("\n\n------------------------\nTime Elapsed: %.6lf\n", eSec);
    printf("Total time of running program code\nsec=%d\tusec=%.0lf\n", sec2, usec2);
    printf("------------------------\n");
};