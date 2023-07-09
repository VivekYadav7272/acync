#define _POSIX_SOURCE
#include <signal.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "private.h"
#include "uthread.h"

/*
 * Frequency of preemption
 * 100Hz is 100 times per second
 */
#define HZ 100

struct sigaction prev_sa;
struct itimerval prev_timer;

void on_preemption(int sig)
{
	(int)sig;
	uthread_yield();
}

void preempt_disable(void)
{
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGVTALRM);
	sigprocmask(SIG_BLOCK, &mask, NULL);
}

void preempt_enable(void)
{
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGVTALRM);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void preempt_start(bool preempt)
{
	if (!preempt)
		return;

	struct sigaction sa;
	struct itimerval timer;

	sa.sa_handler = on_preemption;
	sigemptyset(&sa.sa_mask);
	/* Make functions such as read() or write() to restart instead of
	 * failing when interrupted */
	// sa.sa_flags = SA_RESTART;

	// configure itimerval
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 1000000 / HZ; // in microseconds
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 1000000 / HZ;

	// save the previous state of sigaction and itimerval
	// sigaction(SIGVTALRM, NULL, &prev_sa);
	// getitimer(ITIMER_VIRTUAL, &prev_timer);

	if (sigaction(SIGVTALRM, &sa, NULL) == -1)
	{
		exit(1);
	}

	if (setitimer(ITIMER_VIRTUAL, &timer, NULL) == -1)
	{
		exit(1);
	}
}

void preempt_stop(void)
{
	// restore the previous state of sigaction and itimerval
	// sigaction(SIGVTALRM, &prev_sa, NULL);
	// setitimer(ITIMER_VIRTUAL, &prev_timer, NULL);
}