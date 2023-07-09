#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

typedef struct node_t
{
	void *data;
	struct node_t *next;
} node_t;

typedef struct queue
{
	node_t *head;
	node_t *tail;
	int length;
} queue;

queue_t queue_create(void)
{
	queue *q = malloc(sizeof(queue));
	if (q == NULL)
		return NULL;
	q->head = NULL;
	q->tail = NULL;
	q->length = 0;
	return q;
}

int queue_destroy(queue_t queue)
{
	if (queue == NULL || queue->length != 0)
		return -1;
	free(queue);
	return 0;
}

int queue_enqueue(queue_t queue, void *data)
{
	if (queue == NULL || data == NULL)
		return -1;
	node_t *node = malloc(sizeof(node_t));
	if (node == NULL)
		return -1;
	node->data = data;
	node->next = NULL;
	if (queue->length == 0)
		queue->head = node;
	else
		queue->tail->next = node;
	queue->tail = node;
	queue->length++;
	return 0;
}

int queue_dequeue(queue_t queue, void **data)
{
	if (queue == NULL || data == NULL || queue->length == 0)
		return -1;
	*data = queue->head->data;
	node_t *node = queue->head;
	queue->head = queue->head->next;
	free(node);
	queue->length--;
	return 0;
}

int queue_delete(queue_t queue, void *data)
{
	if (queue == NULL || data == NULL)
		return -1;
	node_t *node = queue->head;
	node_t *prev = NULL;
	while (node != NULL)
	{
		if (node->data == data)
		{
			if (prev == NULL)
				queue->head = node->next;
			else
				prev->next = node->next;
			if (node == queue->tail)
				queue->tail = prev;
			free(node);
			queue->length--;
			return 0;
		}
		prev = node;
		node = node->next;
	}
	return -1;
}

int queue_iterate(queue_t queue, queue_func_t func)
{
	if (queue == NULL || func == NULL)
		return -1;
	node_t *node = queue->head;
	while (node != NULL)
	{
		func(queue, node->data);
		node = node->next;
	}
	return 0;
}

int queue_length(queue_t queue)
{
	if (queue == NULL)
		return -1;
	return queue->length;
}
