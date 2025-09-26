#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/timerfd.h>

int main()
{
    // int timerfd_create(int clockid, int flags);
    int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd < 0) 
    {
        perror("timerfd_create error");
        return -1;
    }

    // int timerfd_settime(int fd, int flags,
    //                    const struct itimerspec *new_value,
    //                    struct itimerspec *old_value);
    struct itimerspec itime;
    itime.it_value.tv_sec = 3;        // timer starts in 1 second
    itime.it_value.tv_nsec = 0;
    itime.it_interval.tv_sec = 3;    // timer interval is 1 second
    itime.it_interval.tv_nsec = 0;
    timerfd_settime(timerfd, 0, &itime, NULL);

    while (1)
    {
        uint64_t times;
        int ret = read(timerfd, &times, sizeof(times));
        if (ret < 0)
        {
            perror("read error");
            return -1;
        }
        printf("timer expired %llu times\n", (unsigned long long)times);
    }
    close(timerfd);
    return 0;
}