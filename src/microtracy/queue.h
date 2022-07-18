#ifndef QUEUE_H
#define QUEUE_H

#if !defined(__STDC_NO_ATOMICS__)
#	include <stdatomic.h>
#endif

struct event_zone_begin {
	int unsigned tid;
	void  *srcloc;
	long long timestamp;
};

struct event_zone_end {
	int unsigned tid;
	long long timestamp;
};

struct event_zone_color {
	int unsigned tid;
	int unsigned color;
};

struct event_frame_mark {
	void *name;
	long long timestamp;
};

struct event_plot {
	void *name;
	float f;
	long long timestamp;
};

struct event_system_time {
	float system_time;
	long long timestamp;
};

struct event {
	char unsigned type;
	union {
		struct event_system_time system_time;
		struct event_zone_begin zone_begin;
		struct event_zone_end zone_end;
		struct event_zone_color zone_color;
		struct event_frame_mark frame_mark;
		struct event_plot plot;
	};
};

#define EVENT_QUEUE_CAPACITY (1u << 18u)

struct event_queue {
	int unsigned producer_tail_cache;
	struct event buf[EVENT_QUEUE_CAPACITY];

#if defined(__STDC_NO_ATOMICS__)
	long volatile head;
	long volatile tail;
#else
	_Alignas(64) atomic_uint head;
	_Alignas(64) atomic_uint tail;
#endif
};

int event_queue_init(struct event_queue *);
int event_queue_destroy(struct event_queue *queue);
int event_queue_push(struct event_queue *const restrict queue, struct event const *const restrict event);
int event_queue_pop(struct event_queue *const restrict queue, struct event *const restrict event);

#endif
