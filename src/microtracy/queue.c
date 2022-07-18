#include "queue.h"

#if defined(__STDC_NO_ATOMICS__)
#	define atomic_load_relaxed(a) *(a)
#	define atomic_load_acquire(a) *(a)
#	define atomic_store_seqcst(a, b) *(a) = (b)
#	define atomic_store_release(a, b) *(a) = (b)
#else
#	include <stdatomic.h>
#	define atomic_load_relaxed(a) atomic_load_explicit((a), memory_order_relaxed)
#	define atomic_load_acquire(a) atomic_load_explicit((a), memory_order_acquire)
#	define atomic_store_seqcst(a, b) atomic_store_explicit((a), (b), memory_order_seq_cst)
#	define atomic_store_release(a, b) atomic_store_explicit((a), (b), memory_order_release)
#endif

int event_queue_init(struct event_queue *queue) {
	queue->producer_tail_cache = 0;
	atomic_store_seqcst(&queue->head, 1);
	atomic_store_seqcst(&queue->tail, 0);
	return 0;
}

int event_queue_destroy(struct event_queue *queue) {
	(void) queue;
	return 0;
}

int event_queue_push(struct event_queue *const restrict queue, struct event const *const restrict event) {
	int unsigned store = atomic_load_relaxed(&queue->head);
	int unsigned next_store = store + 1;
	if(next_store == EVENT_QUEUE_CAPACITY) {
		next_store = 0;
	}

	while(next_store == queue->producer_tail_cache) {
		queue->producer_tail_cache = atomic_load_acquire(&queue->tail);
	}

	queue->buf[store] = *event;
	atomic_store_release(&queue->head, next_store);
	return 0;
}

int event_queue_pop(struct event_queue *const restrict queue, struct event *const restrict event) {
	int unsigned load = atomic_load_relaxed(&queue->tail);
	int unsigned next_load = load + 1;
	if(atomic_load_acquire(&queue->head) == load) {
		return -1;
	}

	*event = queue->buf[load];
	if(next_load == EVENT_QUEUE_CAPACITY) {
		next_load = 0;
	}
	atomic_store_release(&queue->tail, next_load);
	return 0;
}
