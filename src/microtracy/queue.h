#ifndef QUEUE_H
#define QUEUE_H

/* choose your poison */
#define USE_SPINLOCK

#if defined(USE_CRITICAL_SECTION)
#	define INIT_LOCK(lock) InitializeCriticalSection((lock))
#	define ACQUIRE_LOCK(lock) EnterCriticalSection((lock))
#	define RELEASE_LOCK(lock) LeaveCriticalSection((lock))
#elif defined(USE_SRW_LOCK)
#	define INIT_LOCK(lock) InitializeSRWLock((lock))
#	define ACQUIRE_LOCK(lock) AcquireSRWLockExclusive((lock))
#	define RELEASE_LOCK(lock) ReleaseSRWLockExclusive((lock))
#elif defined(USE_SPINLOCK)
#	define INIT_LOCK(lock) InterlockedExchange((lock), 0)
#	define ACQUIRE_LOCK(lock) while(InterlockedExchange((lock), 1)){}
#	define RELEASE_LOCK(lock) InterlockedExchange((lock), 0)
#endif

#include <windows.h>

struct event {
	char unsigned type;
	int unsigned tid;
	long long timestamp;
	union {
		void *srcloc;
		float system_time;
		int unsigned color;

		struct {
			char const *const name;
			char unsigned type;
			float value;
		} plot;

		struct {
			char const *const name;
		} framemark;
	};
};

struct event_queue {
#if defined(USE_CRITICAL_SECTION)
	CRITICAL_SECTION lock;
#elif defined(USE_SRW_LOCK)
	SRWLOCK lock;
#elif defined(USE_SPINLOCK)
	long volatile lock;
#endif
	struct event *buf;
	int unsigned capacity;
	int unsigned len;
	long volatile dropped;
	struct event *head;
	struct event *tail;
};

int event_queue_init(struct event_queue *, int unsigned);
void event_queue_destroy(struct event_queue *);
int event_queue_enqueue(struct event_queue *const restrict queue, struct event const *const restrict evt);
int event_queue_dequeue(struct event_queue *const restrict queue, struct event *const restrict evt);

#endif
