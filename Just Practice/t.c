#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int g;

int *A(void *vargp)
{
    g = g + 1;
    return NULL;
}

int main(int argc, char *argv[])
{
    g = 0;

    pthread_t id1, id2, id3;

    pthread_create(&id1, NULL, A, (void *)3);
    pthread_create(&id2, NULL, A, (void *)3);
    pthread_create(&id3, NULL, A, (void *)3);

    pthread_join(id3, NULL);
    pthread_join(id2, NULL);
    pthread_join(id1, NULL);

    printf("G = %d\n", g);

    return 0;
}