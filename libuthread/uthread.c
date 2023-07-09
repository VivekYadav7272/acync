#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "private.h"
#include "uthread.h"
#include "queue.h"

typedef enum
{
	UTHREAD_STATE_READY,
	UTHREAD_STATE_RUNNING,
	UTHREAD_STATE_BLOCKED,
	UTHREAD_STATE_ZOMBIE,
} uthread_state_t;

typedef int uthread_id;

typedef struct uthread_tcb
{
	uthread_ctx_t *ctx;
	uthread_state_t state;
	uthread_id tid;
	void *stack;
} uthread_tcb;

uthread_tcb *curr_thd = NULL;
uthread_tcb *placeholder_zombie = NULL;
uthread_id next_tid = 0;
queue_t ready_q;
bool to_preempt = false;

struct uthread_tcb *
uthread_current(void)
{
	return curr_thd;
}

// Returns a shallow copy of the TCB block (allocated on heap)
uthread_tcb *clone_tcb(uthread_tcb *tcb)
{
	uthread_tcb *new_tcb = malloc(sizeof(uthread_tcb));
	if (!new_tcb)
		return NULL;
	// Return a shallow copy of tcb.
	*new_tcb = *tcb;
	return new_tcb;
}

void uthread_yield(void)
{
	fprintf(stderr, "uthread_yield called! for %d thread_id\n", uthread_current());
	uthread_tcb *old_curr = uthread_current();

	// If the current thread has still not finished, we need to add it to the ready queue.
	if (old_curr->state == UTHREAD_STATE_RUNNING)
		old_curr->state = UTHREAD_STATE_READY;

	uthread_tcb *first_ready = NULL;

	if (to_preempt)
		preempt_disable();

	if (queue_dequeue(ready_q, (void **)&first_ready) == -1)
		return;

	assert(first_ready->state == UTHREAD_STATE_READY);
	first_ready->state = UTHREAD_STATE_RUNNING;
	curr_thd = first_ready;

	// If the old thread was ready, we need to add it to the ready queue.
	if (old_curr->state == UTHREAD_STATE_READY)
		queue_enqueue(ready_q, old_curr);

	// Earlier zombie threads were not actually being freed, leading to a memory leak.
	// the problem was the threads were context switching before the cleanup could happen
	// (before this if-block)
	// If the old thread was a zombie, we need to kill it and prevent the apocalypse.
	if (old_curr->state == UTHREAD_STATE_ZOMBIE)
	{
		fprintf(stderr, "Zombie found!\n");
		if (placeholder_zombie != NULL)
		{
			uthread_ctx_destroy_stack(placeholder_zombie->stack);
			// fprintf(stderr, "Destroyed stack!");
			free(placeholder_zombie->ctx);
			// fprintf(stderr, "Destroyed context!");
			free(placeholder_zombie);
			// fprintf(stderr, "Destroyed thread!");
		}
		placeholder_zombie = old_curr;
	}
	preempt_enable();
	uthread_ctx_switch(old_curr->ctx, first_ready->ctx);
}

void uthread_exit(void)
{
	uthread_tcb *old_curr = uthread_current();
	old_curr->state = UTHREAD_STATE_ZOMBIE;
	uthread_yield();
}

int uthread_create(uthread_func_t func, void *arg)
{
	uthread_tcb *new_thd = malloc(sizeof(uthread_tcb));
	if (!new_thd)
		return -1;

	new_thd->ctx = malloc(sizeof(uthread_ctx_t));
	if (!new_thd->ctx)
	{
		free(new_thd);
		return -1;
	}

	new_thd->stack = uthread_ctx_alloc_stack();
	if (!new_thd->stack)
	{
		free(new_thd->ctx);
		free(new_thd);
		return -1;
	}

	new_thd->state = UTHREAD_STATE_READY;

	// If two threads are created at the same time, we need to make sure that they have different thread IDs.
	if (to_preempt)
		preempt_disable();
	new_thd->tid = next_tid++;

	if (uthread_ctx_init(new_thd->ctx, new_thd->stack, func, arg) == -1)
	{
		uthread_ctx_destroy_stack(new_thd->stack);
		free(new_thd->ctx);
		free(new_thd);
		return -1;
	}

	queue_enqueue(ready_q, new_thd);

	preempt_enable();

	return 0;
}

int uthread_run(bool preempt, uthread_func_t func, void *arg)
{
	to_preempt = preempt;
	// preempt_start(preempt);

	if (preempt)
		preempt_disable();

	ready_q = queue_create();
	if (!ready_q)
		return -1;

	preempt_enable();

	uthread_tcb *main_thd = malloc(sizeof(uthread_tcb));
	if (!main_thd)
		return -1;
	main_thd->ctx = malloc(sizeof(uthread_ctx_t));
	if (!main_thd->ctx)
	{
		free(main_thd);
		return -1;
	}
	main_thd->state = UTHREAD_STATE_RUNNING;
	main_thd->tid = next_tid++;

	curr_thd = main_thd;

	if (uthread_create(func, arg) == -1)
		return -1;

	while (queue_length(ready_q) != 0)
		uthread_yield();

	// Now, free the main_thread and the ready queue.
	free(main_thd->ctx);
	free(main_thd);
	queue_destroy(ready_q);
	// Also free the zombie thread if it exists.
	if (placeholder_zombie != NULL)
	{
		uthread_ctx_destroy_stack(placeholder_zombie->stack);
		free(placeholder_zombie->ctx);
		free(placeholder_zombie);
	}

	// At the end after all the multithreading shenanigans, we restore the alarms signals back before preemption.
	preempt_stop();

	return 0;
}

void uthread_block(void)
{
	uthread_tcb *old_curr = uthread_current();
	old_curr->state = UTHREAD_STATE_BLOCKED;
	uthread_yield();
}

void uthread_unblock(struct uthread_tcb *uthread)
{
	uthread->state = UTHREAD_STATE_READY;
	if (to_preempt)
		preempt_disable();
	queue_enqueue(ready_q, uthread);
	preempt_enable();
}