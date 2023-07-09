#include <stdio.h>
#include <stdlib.h>
#include <uthread.h>

// I have decided to use my own threading library since it creates user threads multiplexed on one OS thread.
// Since Peterson's algorithm is problematic in a multicore environment, if I just use one kernel thread,
// I can use Peterson's algorithm while avoiding all race conditions.
// (uthreadlib progresses the threads concurrently, but never simultaneously).

bool flag[2] = {false, false};
int turn = 0;
int counter = 0;

void process1();

void process0()
{
    uthread_create(process1, NULL);
    int pid = 0;
    int k = 0;
    while (k < 500)
    {
        k += 1;
        flag[pid] = true;
        turn = 1 - pid;
        while (flag[1 - pid] && turn == 1 - pid)
            ;

        // critical section here
        printf("Thread %d : %d\n", pid, counter);
        counter += 1;
        flag[pid] = false;
    }
}

void process1()
{
    int pid = 1;
    int k = 0;
    while (k < 500)
    {
        k += 1;
        flag[pid] = true;
        turn = 1 - pid;
        while (flag[1 - pid] && turn == 1 - pid)
            ;

        // critical section here
        printf("Thread %d : %d\n", pid, counter);
        counter += 1;
        flag[pid] = false;
    }
}

int main(void)
{
    uthread_run(true, process0, NULL);
    return 0;
}