/* itimer.c program */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

/*************************
 struct timeval {
    time_t      tv_sec;         // seconds
    suseconds_t tv_usec;        // microseconds
 };
 struct itimerval {
    struct timeval it_interval; // Interval of periodic timer
    struct timeval it_value;    // Time until next expiration
 };
*********************/

int hh, mm, ss, tick;

void timer_handler (int sig)
{
    //printf("timer_handler: signal=%d\n", sig);
    if(tick == 1000000 )
    {
        if(ss == 59)
        {
            ss = 0;
            if (mm == 59)
            {
                mm = 0;
                if(hh == 23)
                {
                    hh = 0;
                }
                else hh++;
            }
            else mm++;
        }
        else ss++;

        if ((ss%2) == 1)
            printf("\e[38;5;196m");
        printf("\r\t%02d : %02d : %02d", hh, mm, ss);
        printf("\e[38;5;046m");
        fflush(stdout);
        tick = 0;
    }
    tick += 1000;
}

int main ()
{
    struct itimerval itimer;
    struct timeval t1, t2;
    hh = mm = ss = 0;
    tick = 500000;

    signal(SIGALRM, &timer_handler);

    /* Configure the timer to expire after 0.5 sec */
    itimer.it_value.tv_sec  = 0;
    itimer.it_value.tv_usec = 500000;

    /* and every 1000 usec after that */
    itimer.it_interval.tv_sec  = 0;
    itimer.it_interval.tv_usec = 1000;

    setitimer (ITIMER_REAL, &itimer, NULL);

    while (1);
}