/**** C4.1.c file: compute matrix sum by threads ***/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

#define  M   4
#define  N   500000

int A[M][N], sum[M];

int total;

void *func(void *arg)              // threads function
{
    int j, row, mysum;
    pthread_t tid = pthread_self(); // get thread ID number

    row = (int)arg;                 // get row number from arg
    printf("thread %d computes sum of row %d : ", row, row);
    mysum = 0;

    for (j=0; j < N; j++)     // compute sum of A[row]in global sum[row]
        mysum += A[row][j];

    sum[row] = mysum;

    printf("thread %d done: sum[%d] = %ld\n", row, row, sum[row]);
    pthread_exit((void *)row); // thread exit: 0=normal termination
}

// print the matrix (if N is small, do NOT print for large N)
int print()
{
    int i, j;
    for (i=0; i < M; i++){
        for (j=0; j < N; j++){
            printf("%4d ", A[i][j]);
        }
        printf("\n");
    }
}

int main (int argc, char *argv[])
{
    //running time of program
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    pthread_t thread[M];      // thread IDs
    int i, j, status;

    printf("main: initialize A matrix\n");

    for (i=0; i < M; i++){
        for (j=0; j < N; j++){
            A[i][j] = i + j + 1;
        }
    }

    //print();

    printf("main: create %d threads\n", M);
    for(i=0; i < M; i++) {
        pthread_create(&thread[i], NULL, func, (void *)i);
    }

    printf("main: try to join with threads\n");
    for(i=0; i < M; i++) {
        pthread_join(thread[i], (void *)&status);
        printf("main: joined with thread %d : status=%d\n", i, status);
    }

    printf("main: compute and print total : ");
    for (i=0; i < M; i++)
        total += sum[i];
    printf("total = %ld\n", total);

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

    printf("\n\n------------------------\nTime Elapsed: %.6lfs\n", eSec);
    printf("Total time of running program code\nsec=%1d\tusec=%.0lf\n", sec2, usec2);
    printf("------------------------\n");
    pthread_exit(NULL);
}