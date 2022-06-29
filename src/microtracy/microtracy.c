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
#define TARGET_FRAME_SIZE (256 * 1024)

struct utracy utracy;

/* high resolution x86 timestamp counter - assumes invariant tsc capability */
static inline long long utracy_tsc(void) {
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
static inline long long utracy_qpc(void) {
	LARGE_INTEGER li_counter;
	QueryPerformanceCounter(&li_counter);
	return li_counter.QuadPart;
}

/* x86 windows fs register points to teb/tib
   thread id is at offset 0x24 */
#if defined(__clang__) || defined(__GNUC__)
static inline int unsigned utracy_tid(void) {
	int unsigned tid;
	__asm__("mov %%fs:0x24, %0;" :"=r"(tid));
	return tid;
}
#elif defined(_MSVC_VER)
static inline int unsigned utracy_tid(void) {
	__asm {
		mov eax, fs:[0x24];
		ret;
	}
}
#else
static inline int unsigned utracy_tid(void) {
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
	int unsigned const iterations = 50000;

	struct utracy_source_location srcloc = {
		.name = NULL,
		.function = __func__,
		.file = __FILE__,
		.line = 0,
		.color = 0
	};

	struct event zone_begin = {
		.type = UTRACY_QUEUE_TYPE_ZONEBEGIN,
		.zone_begin.timestamp = utracy_tsc(),
		.zone_begin.srcloc = &srcloc
	};

	struct event zone_end = {
		.type = UTRACY_QUEUE_TYPE_ZONEEND,
		.zone_end.timestamp = utracy_tsc()
	};

	_mm_lfence();
	long long begin_tsc = utracy_tsc();

	for(int unsigned i=0; i<iterations; i++) {
		event_queue_enqueue(&utracy.event_queue, &zone_begin);
		event_queue_enqueue(&utracy.event_queue, &zone_end);
	}

	long long end_tsc = utracy_tsc();
	_mm_lfence();
	long long dt = end_tsc - begin_tsc;

	while(0 == event_queue_dequeue(&utracy.event_queue, &zone_begin));

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

static int utracy_recv_all(void *buf, int unsigned size) {
	char *_buf = buf;
	DWORD offset = 0;
	DWORD rx;
	DWORD flags = 0;

	while(offset < size) {
		if(SOCKET_ERROR == WSARecv(
			utracy.sock.client,
			(WSABUF[]) {{
				.len = size - offset,
				.buf = _buf + offset
			}},
			1,
			&rx,
			&flags,
			NULL,
			NULL
		)) {
			DEBUG_DIAGNOSTICS;
			return -1;
		}

		if(rx == 0) {
			DEBUG_DIAGNOSTICS;
			return -1;
		}

		offset += rx;
	}

	return 0;
}

static int utracy_send_all(void *buf, int unsigned size) {
	char *_buf = buf;
	DWORD offset = 0;
	DWORD tx;
	DWORD flags = 0;

	while(offset < size) {
		if(SOCKET_ERROR == WSASend(
			utracy.sock.client,
			(WSABUF[]) {{
				.len = size - offset,
				.buf = _buf + offset
			}},
			1,
			&tx,
			flags,
			NULL,
			NULL
		)) {
			DEBUG_DIAGNOSTICS;
			return -1;
		}

		if(tx == 0) {
			DEBUG_DIAGNOSTICS;
			return -1;
		}

		offset += tx;
	}

	return 0;
}

static int utracy_client_negotiate(void) {
	/* client sends shibboleth */
	char shibboleth[UTRACY_HANDSHAKE_SHIBBOLETH_SIZE];
	if(0 != utracy_recv_all(shibboleth, sizeof(shibboleth))) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	if(0 != memcmp(shibboleth, UTRACY_HANDSHAKE_SHIBBOLETH, sizeof(shibboleth))) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	/* client sends protocol version */
	int unsigned protocol_version;
	if(0 != utracy_recv_all(&protocol_version, sizeof(protocol_version))) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	if(UTRACY_PROTOCOL_VERSION != protocol_version) {
		DEBUG_DIAGNOSTICS;
		char unsigned response = UTRACY_HANDSHAKE_PROTOCOL_MISMATCH;
		if(0 != utracy_send_all(&response, sizeof(response))) {
			DEBUG_DIAGNOSTICS;
			return -1;
		}
		return -1;
	}

	/* server sends ack */
	char unsigned response = UTRACY_HANDSHAKE_WELCOME;
	if(0 != utracy_send_all(&response, sizeof(response))) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	/* server sends info */
	struct utracy_welcomemsg welcome_msg = {
		.timerMul = utracy.info.multiplier,
		.initBegin = utracy.info.init_begin,
		.initEnd = utracy.info.init_end,
		.delay = utracy.info.delay,
		.resolution = utracy.info.resolution,
		.epoch = utracy.info.epoch,
		.exectime = utracy.info.exectime,
		.pid = 0,
		.samplingPeriod = 0,
		.flags = 8,
		.cpuArch = 0,
		//.cpuManufacturer = "test",
		.cpuId = 0
		//.programName = "test",
		//.hostInfo = "test"
	};

	memset(welcome_msg.cpuManufacturer, 0, sizeof(welcome_msg.cpuManufacturer));
	memset(welcome_msg.programName, 0, sizeof(welcome_msg.programName));
	memset(welcome_msg.hostInfo, 0, sizeof(welcome_msg.hostInfo));
	memcpy(welcome_msg.cpuManufacturer, "???", 3);
	memcpy(welcome_msg.programName, "dreamdaemon.exe", 15);
	memcpy(welcome_msg.hostInfo, "???", 3);

	if(0 != utracy_send_all(&welcome_msg, sizeof(welcome_msg))) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	return 0;
}

static int utracy_raw_buf_append(void *buf, int unsigned len) {
	if(len >= utracy.raw_buf_len - utracy.raw_buf_head) {
		return -1;
	}

	memcpy(utracy.raw_buf + utracy.raw_buf_head, buf, len);
	utracy.raw_buf_head += len;

	return 0;
}

static int utracy_append_heartbeat(void) {
	DWORD now = GetTickCount();

	if(now - utracy.info.last_heartbeat >= 100) {
		utracy.info.last_heartbeat = now;

		struct event evt = {
			.type = UTRACY_QUEUE_TYPE_SYSTIMEREPORT,
			.system_time.timestamp = utracy_tsc(),
			.system_time.system_time = 0.0f
		};

		event_queue_enqueue(&utracy.event_queue, &evt);
	}

	return 0;
}

extern int utracy_commit(void) {
	if(0 < utracy.raw_buf_head - utracy.raw_buf_tail) {
		if(1 == 1) {
			utracy.thread_time = 0;

			char unsigned type = UTRACY_QUEUE_TYPE_THREADCONTEXT;
			if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			int unsigned tid = utracy.current_tid;
			if(0 != utracy_raw_buf_append(&tid, sizeof(tid))) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

		}
		{
			char unsigned type = UTRACY_QUEUE_TYPE_PLOTDATA;
			if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			long long unsigned name = (long long unsigned) (uintptr_t) "(debug) events remaining in queue";
			if(0 != utracy_raw_buf_append(&name, sizeof(name))) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			long long timestamp = utracy_tsc() - utracy.thread_time;
			utracy.thread_time = timestamp;
			if(0 != utracy_raw_buf_append(&timestamp, sizeof(timestamp))) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			char unsigned plot_type = 0;
			if(0 != utracy_raw_buf_append(&plot_type, sizeof(plot_type))) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			float value = (float) utracy.event_queue.len;
			if(0 != utracy_raw_buf_append(&value, sizeof(value))) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}
		}

		int unsigned pending_len;
		pending_len = utracy.raw_buf_head - utracy.raw_buf_tail;

		do {
			int unsigned raw_frame_len = min(pending_len, TARGET_FRAME_SIZE);

			/* write compressed buf */
			int unsigned compressed_len = LZ4_compress_fast_continue(
				&utracy.stream,
				/* src */
				utracy.raw_buf + utracy.raw_buf_tail,
				/* dst */
				utracy.buf,
				/* src len */
				raw_frame_len,
				/* dst max len */
				utracy.buf_len,
				1
			);

			/* write compressed buf len */
#if defined(__clang__)
			__builtin_memcpy_inline(
				utracy.frame_buf,
				&compressed_len,
				sizeof(compressed_len)
			);
#else
			memcpy(
				utracy.frame_buf,
				&compressed_len,
				sizeof(compressed_len)
			);
#endif

			/* transmit frame */
			if(0 != utracy_send_all(utracy.frame_buf, compressed_len + sizeof(compressed_len))) {
				return -1;
			}

			/* advance tail */
			utracy.raw_buf_tail += raw_frame_len;
			pending_len = utracy.raw_buf_head - utracy.raw_buf_tail;
		} while(0 < pending_len);

		/* previous 64kb of uncompressed data must remain unclobbered at the
		   same memory address! */
		if(utracy.raw_buf_head > TARGET_FRAME_SIZE * 2) {
			utracy.raw_buf_head = 0;
			utracy.raw_buf_tail = 0;
		}
	}

	return 0;
}

extern int utracy_dequeue_event(struct event *evt) {
	if(0 != event_queue_dequeue(&utracy.event_queue, evt)) {
		return -1;
	}

	return 0;
}

int utracy_init(HANDLE init_event) {
	event_queue_init(&utracy.event_queue, 65535 * 2);

	/* raw uncompressed buffer is fed into lz4 compessor */
	utracy.raw_buf_len = TARGET_FRAME_SIZE * 3;
	utracy.raw_buf_head = 0;
	utracy.raw_buf_tail = 0;
	utracy.raw_buf = VirtualAlloc(NULL, utracy.raw_buf_len, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if(NULL == utracy.raw_buf) {
		return -1;
	}

	/* compressed buffer that is to be transmitted */
	utracy.buf_len = LZ4_compressBound(TARGET_FRAME_SIZE);

	/* |==============frame buf==============|
	   [length][=========buf=================]
	      ^               ^
	   4 bytes       length bytes */

	size_t buf_offset = sizeof(int unsigned);
	size_t frame_len = utracy.buf_len + buf_offset;

	utracy.frame_buf = VirtualAlloc(NULL, frame_len, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if(NULL == utracy.frame_buf) {
		return -1;
	}

	utracy.buf = utracy.frame_buf + buf_offset;
	memset(utracy.frame_buf, 0, utracy.buf_len + sizeof(int unsigned));

	utracy.info.init_begin = utracy_tsc();
	utracy.info.multiplier = utracy_calibrate_timer();
	utracy.info.resolution = utracy_calibrate_resolution();
	utracy.info.delay = utracy_calibrate_delay();

	/* time since unix epoch */
	utracy_setup_time(&utracy.info.epoch, &utracy.info.exectime);
	utracy.info.last_heartbeat = 0;

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

	/* accept a client */
	if(0 != utracy_accept()) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	/* ensure client speaks the same protocol */
	if(0 != utracy_client_negotiate()) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	SetEvent(init_event);
	utracy.current_tid = 0;
	utracy.thread_time = 0;

	while(TRUE) {
		if(0 != utracy_append_heartbeat()) {
			DEBUG_DIAGNOSTICS;
			return -1;
		}

		if(0 != utracy_consume_queue()) {
			DEBUG_DIAGNOSTICS;
			return -1;
		}

		WSAPOLLFD descriptor = {
			.fd = utracy.sock.client,
			.events = POLLRDNORM,
			.revents = 0
		};

		while(0 < WSAPoll(&descriptor, 1, 1)) {
			if(0 != utracy_consume_request()) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}
		}

		if(0 != utracy_commit()) {
			DEBUG_DIAGNOSTICS;
			return -1;
		}
	}

	return 0;
}

DWORD WINAPI utracy_thread_start(PVOID user) {
	HANDLE init_event = (HANDLE) user;

	int r = utracy_init(init_event);
	if(0 != r) {
		ExitProcess(-1);
		return -1;
	}

	return 0;
}

extern void utracy_emit_zone_begin(struct utracy_source_location const *const srcloc) {
	event_queue_enqueue(&utracy.event_queue, &(struct event) {
		.type = UTRACY_QUEUE_TYPE_ZONEBEGIN,
		.zone_begin.tid = utracy_tid(),
		.zone_begin.timestamp = utracy_tsc(),
		.zone_begin.srcloc = (void *) srcloc
	});
}

extern void utracy_emit_zone_end(void) {
	event_queue_enqueue(&utracy.event_queue, &(struct event) {
		.type = UTRACY_QUEUE_TYPE_ZONEEND,
		.zone_end.tid = utracy_tid(),
		.zone_end.timestamp = utracy_tsc()
	});
}

extern void utracy_emit_zone_color(int unsigned color) {
	event_queue_enqueue(&utracy.event_queue, &(struct event) {
		.type = UTRACY_QUEUE_TYPE_ZONECOLOR,
		.zone_color.tid = utracy_tid(),
		.zone_color.color = color
	});
}

extern void utracy_emit_frame_mark(char const *const name) {
	event_queue_enqueue(&utracy.event_queue, &(struct event) {
		.type = UTRACY_QUEUE_TYPE_FRAMEMARKMSG,
		.frame_mark.name = name,
		.frame_mark.timestamp = utracy_tsc()
	});
}

extern void utracy_emit_plot(char const *const name, float value) {
	event_queue_enqueue(&utracy.event_queue, &(struct event) {
		.type = UTRACY_QUEUE_TYPE_PLOTDATA,
		.plot.name = name,
		.plot.timestamp = utracy_tsc(),
		.plot.f = value
	});
}

extern void utracy_emit_thread_name(char const *const name) {
	(void) name;
}
