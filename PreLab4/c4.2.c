/**** C4.1.c file: compute matrix sum by threads ***/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

#define  M   4
#define  N   500000

int A[M][N], sum[M];

int total;
pthread_mutex_t m;

//turns color red
void red(void)
{
    printf("\e[38;5;196m");
}

//turns color green
void green(void)
{
    printf("\e[38;5;046m");
}

//returns text color back to white
void back(void)
{
    printf("\e[0m");
}


void *func(void *arg)              // threads function
{
    int j, row, temp, mysum;
    pthread_t tid = pthread_self(); // get thread ID number

    row = (int)arg;                 // get row number from arg
    printf("thread %d computes sum of row %d \n", row, row);
    mysum = 0;

    for (j=0; j < N; j++)     // compute sum of A[row]in global sum[row]
        mysum += A[row][j];

    red();
    printf("thread %d done: sum[%d] = %ld\n", row, row, mysum);
    back();
    pthread_mutex_lock(&m);
    /************** A CRITICAL REGION *******************/
    temp = total;   // get total
    temp += mysum;  // add mysum to temp

    sleep(1);       // OR for (int i=0; i<100000000; i++); ==> switch threads

    total = temp;   //  write temp to total
    /************ end of CRITICAL REGION ***************/
    pthread_mutex_unlock(&m);
    green();
    printf("thread %d done, TOTAL: %d\n", row, total);
    back();
    pthread_exit((void*)0);  // thread exit: 0=normal termination
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

    pthread_mutex_init(&m, NULL);

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

    printf("\nTotal should be 1787793664\n");
    if (total == 1787793664)
    {
        green();
        printf("correct total = %ld\n", total);
    }
    else
    {
        red();
        printf("incorrect total = %ld\n", total);
    }
     back();

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