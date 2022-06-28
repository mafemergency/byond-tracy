#include <windows.h>
#include "queue.h"

int event_queue_init(struct event_queue *queue, int unsigned capacity) {
	queue->buf = VirtualAlloc(NULL, sizeof(struct event) * capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	queue->capacity = capacity;
	queue->len = 0;
	queue->dropped = 0;
	queue->head = queue->buf;
	queue->tail = queue->buf;
	INIT_LOCK(&queue->lock);
	return 0;
}

void event_queue_destroy(struct event_queue *queue) {
	(void) queue;
}

int event_queue_enqueue(struct event_queue *const restrict queue, struct event const *const restrict evt) {
	ACQUIRE_LOCK(&queue->lock);
	int unsigned i = queue->len;

	/* queue full */
	if(i == queue->capacity) {
		RELEASE_LOCK(&queue->lock);
		InterlockedIncrement(&queue->dropped);
		return -1;
	}

	queue->head++;

	/* wrap around */
	if(queue->head == queue->buf + queue->capacity) {
		queue->head = queue->buf;
	}

#if defined(__GNUC__) || defined(__clang__)
	(void) __builtin_memcpy_inline(queue->head + 0, evt + 0, sizeof(struct event));
#else
	(void) memcpy(queue->head, evt, sizeof(struct event));
#endif

	queue->len++;

	RELEASE_LOCK(&queue->lock);
	return 0;
}

int event_queue_dequeue(struct event_queue *const restrict queue, struct event *const restrict evt) {
	ACQUIRE_LOCK(&queue->lock);
	int unsigned i = queue->len;

	/* queue empty */
	if(i == 0) {
		RELEASE_LOCK(&queue->lock);
		return -1;
	}

	queue->tail++;

	/* wrap around */
	if(queue->tail == queue->buf + queue->capacity) {
		queue->tail = queue->buf;
	}

#if defined(__GNUC__) || defined(__clang__)
	(void) __builtin_memcpy_inline(evt + 0, queue->tail + 0, sizeof(struct event));
#else
	(void) memcpy(evt, queue->tail, sizeof(struct event));
#endif

	queue->len--;

	RELEASE_LOCK(&queue->lock);
	return 0;
}
