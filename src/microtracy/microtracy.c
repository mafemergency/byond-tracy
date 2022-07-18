#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <emmintrin.h>
#include "lz4.h"
#include "queue.h"
#include "server.h"
#include "microtracy.h"
#include <stdio.h>

#define DEBUG_DIAGNOSTICS printf("%s %s:%d (wsa=%d)\n", __func__, __FILE__, __LINE__, WSAGetLastError())

struct utracy utracy;

/* high resolution x86 timestamp counter - assumes invariant tsc capability */
long long utracy_tsc(void) {
#if defined(__clang__) || defined(__GNUC__)
	return (long long) __builtin_ia32_rdtsc();
#elif defined(_MSC_VER)
	return (long long) __rdtsc();
#else
	int unsigned eax, edx;
	__asm__ __volatile__("rdtsc;" :"=a"(eax), "=d"(edx));
	return ((long long) edx << 32) + eax;
#endif
}

/* lower resolution counter */
static long long utracy_qpc(void) {
	LARGE_INTEGER li_counter;
	QueryPerformanceCounter(&li_counter);
	return li_counter.QuadPart;
}

/* x86 windows fs register points to teb/tib
   thread id is at offset 0x24 */
#if defined(__clang__) || defined(__GNUC__)
int unsigned utracy_tid(void) {
	int unsigned tid;
	__asm__("mov %%fs:0x24, %0;" :"=r"(tid));
	return tid;
}
#elif defined(_MSVC_VER)
int unsigned utracy_tid(void) {
	__asm {
		mov eax, fs:[0x24];
		ret;
	}
}
#else
int unsigned utracy_tid(void) {
	return GetCurrentThreadId();
}
#endif

static double utracy_calibrate_timer(void) {
	_mm_lfence();
	long long b0 = utracy_qpc();
	long long a0 = utracy_tsc();
	_mm_lfence();

	Sleep(250);

	_mm_lfence();
	long long b1 = utracy_qpc();
	long long a1 = utracy_tsc();
	_mm_lfence();

	LARGE_INTEGER li_freq;
	QueryPerformanceFrequency(&li_freq);

	long long dtsc = a1 - a0;
	long long dqpc = b1 - b0;
	double dt = ((double) dqpc * 1000000000.0) / (double) li_freq.QuadPart;
	double mul = (double) dt / (double) dtsc;
	return mul;
}

static long long utracy_calibrate_resolution(void) {
	int const iterations = 250000;
	long long min_diff = 0x7FFFFFFFFFFFFFFFll;

	for(long i=0; i<iterations; i++) {
		_mm_lfence();
		long long a = utracy_tsc();
		long long b = utracy_tsc();
		_mm_lfence();
		long long diff = b - a;
		if(diff > 0 && diff < min_diff) {
			min_diff = diff;
		}
	}

	return min_diff;
}

static long long utracy_calibrate_delay(void) {
	int unsigned const iterations = (EVENT_QUEUE_CAPACITY / 2u) - 1;

	struct utracy_source_location srcloc = {
		.name = NULL,
		.function = __func__,
		.file = __FILE__,
		.line = 0,
		.color = 0
	};

	_mm_lfence();
	long long begin_tsc = utracy_tsc();
	_mm_lfence();

	for(int unsigned i=0; i<iterations; i++) {
		_mm_lfence();
		utracy_emit_zone_begin(&srcloc);
		utracy_emit_zone_end();
		_mm_lfence();
	}

	_mm_lfence();
	long long end_tsc = utracy_tsc();
	_mm_lfence();
	long long dt = end_tsc - begin_tsc;

	struct event evt;
	while(0 == utracy_dequeue_event(&evt));

	return dt / (long long) (iterations * 2);
}

static void utracy_setup_time(long long *epoch, long long *exectime) {
	/* thanks Ian Boyd https://stackoverflow.com/a/46024468 */
	long long const UNIX_TIME_START = 0x019DB1DED53E8000ll;
	long long const TICKS_PER_SECOND = 10000000ll;

	FILETIME ft_timestamp;
	GetSystemTimeAsFileTime(&ft_timestamp);

	LARGE_INTEGER li_timestamp = {
		.LowPart = ft_timestamp.dwLowDateTime,
		.HighPart = ft_timestamp.dwHighDateTime
	};

	*epoch = (li_timestamp.QuadPart - UNIX_TIME_START) / TICKS_PER_SECOND;
	/* would like to pass mtime for the dmb here */
	*exectime = *epoch;
}

int utracy_dequeue_event(struct event *evt) {
	if(-1 != event_queue_pop(&utracy.event_queue, evt)) {
		return 0;
	}

	return -1;
}

int utracy_init(HANDLE initialized_event) {
	(void) event_queue_init(&utracy.event_queue);

	/* raw uncompressed buffer is fed into lz4 compessor */
	utracy.raw_buf_len = UTRACY_MAX_FRAME_SIZE * 3;
	utracy.raw_buf_head = 0;
	utracy.raw_buf_tail = 0;

	/* compressed buffer that is to be transmitted */
	utracy.buf_len = LZ4_COMPRESSBOUND(UTRACY_MAX_FRAME_SIZE);

	/* |==============frame buf==============|
	   [length][=========buf=================]
	      ^               ^
	   4 bytes       length bytes */

	size_t buf_offset = sizeof(int unsigned);
	memset(utracy.frame_buf, 0, utracy.buf_len + sizeof(int unsigned));
	utracy.buf = utracy.frame_buf + buf_offset;

	utracy.info.init_begin = utracy_tsc();
	utracy.info.multiplier = utracy_calibrate_timer();
	utracy.info.resolution = utracy_calibrate_resolution();
	utracy.info.delay = utracy_calibrate_delay();

	/* time since unix epoch */
	utracy_setup_time(&utracy.info.epoch, &utracy.info.exectime);
	utracy.info.last_heartbeat = 0;
	utracy.info.last_pump = GetTickCount64();

	utracy.current_tid = 0;
	utracy.thread_time = 0;

	/* setup server socket, bind, and listen */
	if(0 != utracy_server_init()) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	/* compression stream */
	void *stream = LZ4_initStream(&utracy.stream, sizeof(utracy.stream));
	if(NULL == stream) {
		return -1;
	}

	LZ4_resetStream_fast(&utracy.stream);

	utracy.info.init_end = utracy_tsc();
	(void) SetEvent(initialized_event);

	return 0;
}

static int utracy_run(HANDLE request_shutdown_event, HANDLE connected_event) {
	/* accept a client */
	if(0 != utracy_client_accept(request_shutdown_event)) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	/* ensure client speaks the same protocol */
	if(0 != utracy_client_negotiate()) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	(void) SetEvent(connected_event);

	int shutdown_requested = FALSE;
	while(!shutdown_requested && utracy.sock.connected) {
		if(0 != utracy_server_pump()) {
			DEBUG_DIAGNOSTICS;
			return -1;
		}

		switch(WaitForSingleObject(request_shutdown_event, 0)) {
			case WAIT_TIMEOUT:
				break;

			case WAIT_OBJECT_0:
			default:
				shutdown_requested = TRUE;
				break;
		}
	}

	return 0;
}

int utracy_destroy(void) {
	(void) utracy_server_destroy();
	(void) event_queue_destroy(&utracy.event_queue);

	return 0;
}

DWORD WINAPI utracy_thread_start(PVOID user) {
	struct {
		HANDLE initialized_event;
		HANDLE connected_event;
		HANDLE request_shutdown_event;
	} *params = user;

	switch(WaitForSingleObject(params->request_shutdown_event, 0)) {
		case WAIT_OBJECT_0:
			return 0;
		default:
			break;
	}

	if(0 == utracy_init(params->initialized_event)) {
		(void) utracy_run(params->request_shutdown_event, params->connected_event);
	}

	(void) utracy_destroy();

	return 0;
}

void utracy_emit_zone_begin(struct utracy_source_location const *const srcloc) {
	event_queue_push(&utracy.event_queue, &(struct event) {
		.type = UTRACY_EVT_ZONEBEGIN,
		.zone_begin.tid = utracy_tid(),
		.zone_begin.timestamp = utracy_tsc(),
		.zone_begin.srcloc = (void *) srcloc
	});
}

void utracy_emit_zone_end(void) {
	event_queue_push(&utracy.event_queue, &(struct event) {
		.type = UTRACY_EVT_ZONEEND,
		.zone_end.tid = utracy_tid(),
		.zone_end.timestamp = utracy_tsc()
	});
}

void utracy_emit_zone_color(int unsigned color) {
	event_queue_push(&utracy.event_queue, &(struct event) {
		.type = UTRACY_EVT_ZONECOLOR,
		.zone_color.tid = utracy_tid(),
		.zone_color.color = color
	});
}

void utracy_emit_frame_mark(char *const name) {
	event_queue_push(&utracy.event_queue, &(struct event) {
		.type = UTRACY_EVT_FRAMEMARKMSG,
		.frame_mark.name = name,
		.frame_mark.timestamp = utracy_tsc()
	});
}

void utracy_emit_plot(char *const name, float value) {
	event_queue_push(&utracy.event_queue, &(struct event) {
		.type = UTRACY_EVT_PLOTDATA,
		.plot.name = name,
		.plot.timestamp = utracy_tsc(),
		.plot.f = value
	});
}

void utracy_emit_thread_name(char *const name) {
	(void) name;
}
