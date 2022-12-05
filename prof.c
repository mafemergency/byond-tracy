/* c11 minimum */
#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201112L)
#	pragma message("minimum supported c standard is c11")
#endif

#if defined(__cplusplus)
#	pragma message("compiling as c++ is ill-advised")
#endif

/* platform identification */
#if defined(_WIN32)
#	define UTRACY_WINDOWS
#	if defined(_WIN64)
#		error 64 bit not supported
#	endif
#	if !defined(_WIN32_WINNT)
#		define _WIN32_WINNT 0x0601
#	endif
#	if !defined(WINVER)
#		define WINVER 0x0601
#	endif
#elif defined(__linux__)
#	define UTRACY_LINUX
#	if defined(__x86_64__)
#		error 64 bit not supported
#	endif
#else
#	error platform not detected
#endif

/* compiler identification */
#if defined(__clang__)
#	define UTRACY_CLANG
#elif defined(__GNUC__)
#	define UTRACY_GCC
#elif defined(_MSC_VER)
#	define UTRACY_MSVC
#else
#	error compiler not detected
#endif

#if defined(UTRACY_CLANG) || defined(UTRACY_GCC)
#	define likely(expr) __builtin_expect(((expr) != 0), 1)
#	define unlikely(expr) __builtin_expect(((expr) != 0), 0)
#else
#	define likely(expr) (expr)
#	define unlikely(expr) (expr)
#endif

/* data type size check */
_Static_assert(sizeof(void *) == 4, "incorrect size");
_Static_assert(sizeof(int) == 4, "incorrect size");
_Static_assert(sizeof(long long) == 8, "incorrect size");

/* linkage and exports */
#if defined(UTRACY_WINDOWS)
#	if defined(UTRACY_CLANG) || defined(UTRACY_GCC)
#		define UTRACY_INTERNAL static
#		define UTRACY_EXTERNAL __attribute__((visibility("default"))) __attribute__((dllexport))
#		define UTRACY_INLINE inline __attribute__((always_inline))
#	elif defined(UTRACY_MSVC)
#		define UTRACY_INTERNAL static
#		define UTRACY_EXTERNAL __declspec(dllexport)
#		define UTRACY_INLINE inline __forceinline
#	endif
#elif defined(UTRACY_LINUX)
#	if defined(UTRACY_CLANG) || defined(UTRACY_GCC)
#		define UTRACY_INTERNAL static
#		define UTRACY_EXTERNAL __attribute__((visibility("default")))
#		define UTRACY_INLINE inline __attribute__((always_inline))
#	endif
#endif

/* calling conventions */
#if defined(UTRACY_WINDOWS)
#	define UTRACY_WINDOWS_CDECL __cdecl
#	define UTRACY_WINDOWS_STDCALL __stdcall
#	define UTRACY_WINDOWS_THISCALL __thiscall
#	define UTRACY_LINUX_CDECL
#	define UTRACY_LINUX_STDCALL
#	define UTRACY_LINUX_THISCALL
#	define UTRACY_LINUX_REGPARM(a)
#elif defined(UTRACY_LINUX)
#	define UTRACY_WINDOWS_CDECL
#	define UTRACY_WINDOWS_STDCALL
#	define UTRACY_WINDOWS_THISCALL
#	define UTRACY_LINUX_CDECL __attribute__((cdecl))
#	define UTRACY_LINUX_STDCALL __attribute__((stdcall))
#	define UTRACY_LINUX_THISCALL __attribute__((thiscall))
#	define UTRACY_LINUX_REGPARM(a) __attribute__((regparm(a)))
#endif

/* headers */
#if defined(UTRACY_WINDOWS)
#	define NOMINMAX
#	include <winsock2.h>
#	include <ws2tcpip.h>
#	include <windows.h>
#elif defined(UTRACY_LINUX)
#	define _GNU_SOURCE
#	include <errno.h>
#	include <unistd.h>
#	include <time.h>
#	include <sys/syscall.h>
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <sys/mman.h>
#	include <netdb.h>
#	include <pthread.h>
#	include <dlfcn.h>
#	include <link.h>
#	include <netinet/ip.h>
#	include <arpa/inet.h>
#	include <poll.h>
#endif

#include <stddef.h>
#include <assert.h>
#include <stdio.h>

#if (__STDC_HOSTED__ == 1)
#	include <stdlib.h>
#	include <string.h>
#endif

#if !defined(__STDC_NO_ATOMICS__)
#	include <stdatomic.h>
#endif

#if (__STDC_HOSTED__ == 0)
void *memset(void *const a, int value, size_t len) {
	for(size_t i=0; i<len; i++) {
		*((char *) a + i) = value;
	}
	return a;
}

void *memcpy(void *const restrict dst, void const *const restrict src, size_t len) {
	for(size_t i=0; i<len; i++) {
		*((char *) dst + i) = *((char *) src + i);
	}
	return dst;
}

int memcmp(void const *a, void const *b, size_t len) {
	for(size_t i=0; i<len; i++) {
		char unsigned _a = *(char unsigned *) a;
		char unsigned _b = *(char unsigned *) b;
		if(_a != _b) {
			return (_a - _b);
		}
	}
	return 0;
}

size_t strlen(char const *const a) {
	size_t len = 0;
	for(char const *p=a; *p; p++) {
		len++;
	}
	return len;
}
#endif

#if defined(max)
#	undef max
#endif
#if defined(UTRACY_CLANG) || defined(UTRACY_GCC)
#	define max(a, b) ({ \
		__typeof__(a) _a = (a); \
		__typeof__(b) _b = (b); \
		_a > _b ? _a : _b; \
	})
#elif defined(UTRACY_MSVC)
#	define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#if defined(min)
#	undef min
#endif
#if defined(UTRACY_CLANG) || defined(UTRACY_GCC)
#	define min(a, b) ({ \
		__typeof__(a) _a = (a); \
		__typeof__(b) _b = (b); \
		_a < _b ? _a : _b; \
	})
#elif defined(UTRACY_MSVC)
#	define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#if defined(UTRACY_CLANG) || defined(UTRACY_GCC)
#	if __has_builtin(__builtin_memcpy)
#		define UTRACY_MEMCPY __builtin_memcpy
#	else
#		define UTRACY_MEMCPY memcpy
#	endif
#	if __has_builtin(__builtin_memset)
#		define UTRACY_MEMSET __builtin_memset
#	else
#		define UTRACY_MEMSET memset
#	endif
#	if __has_builtin(__builtin_memcmp)
#		define UTRACY_MEMCMP __builtin_memcmp
#	else
#		define UTRACY_MEMCMP memcmp
#	endif
#else
#	define UTRACY_MEMCPY memcpy
#	define UTRACY_MEMSET memset
#	define UTRACY_MEMCMP memcmp
#endif


/* debugging */
#if defined(UTRACY_DEBUG) || defined(DEBUG)
#	include <stdio.h>
#	define LOG_DEBUG_ERROR fprintf(stderr, "err: %s %s:%d\n", __func__, __FILE__, __LINE__)
#	define LOG_INFO(...) fprintf(stdout, __VA_ARGS__)
#else
#	define LOG_DEBUG_ERROR
#	define LOG_INFO(...)
#endif

/* config */
#define EVENT_QUEUE_CAPACITY (1u << 18u)
_Static_assert((EVENT_QUEUE_CAPACITY & (EVENT_QUEUE_CAPACITY - 1)) == 0, "EVENT_QUEUE_CAPACITY must be a power of 2");

/* byond types */
struct object {
	union {
		int unsigned padding;
		char unsigned type;
	};
	union {
		int unsigned i;
		float f;
	};
};

struct string {
	char *data;
	int unsigned id;
	struct string *left;
	struct string *right;
	int unsigned refcount;
	int unsigned unk0;
	int unsigned len;
};

struct procdef {
	int unsigned path;
	int unsigned name;
	int unsigned desc;
	int unsigned category;
	int unsigned flags;
	int unsigned unk0;
	int unsigned bytecode;
	int unsigned locals;
	int unsigned parameters;
};

struct misc {
	struct {
		short unsigned len;
		int unsigned unk0;
		int unsigned *bytecode;
	} bytecode;
	struct {
		short unsigned len;
		int unsigned unk0;
		int unsigned *locals;
	} locals;
	struct {
		short unsigned len;
		int unsigned unk0;
		int unsigned *params;
	} params;
};

struct proc {
	int unsigned procdef;
	char unsigned flags;
	char unsigned supers;
	short unsigned unused;
	struct object usr;
	struct object src;
	struct execution_context *ctx;
	int unsigned sequence;
	void (*callback)(struct object, int unsigned);
	int unsigned callback_arg;
	int unsigned argc;
	struct object *argv;
	int unsigned unk0;
};

/* byond type size check */
_Static_assert(sizeof(struct object) == 8, "incorrect size");
_Static_assert(sizeof(struct string) == 28, "incorrect size");
_Static_assert(sizeof(struct procdef) == 36, "incorrect size");
_Static_assert(sizeof(struct misc) == 36, "incorrect size");
_Static_assert(sizeof(struct proc) >= 4, "incorrect size");

/* queue */
#if defined(__STDC_NO_ATOMICS__)
#	define atomic_load_relaxed(a) *(a)
#	define atomic_load_acquire(a) *(a)
#	define atomic_store_seqcst(a, b) *(a) = (b)
#	define atomic_store_release(a, b) *(a) = (b)
#else
#	define atomic_load_relaxed(a) atomic_load_explicit((a), memory_order_relaxed)
#	define atomic_load_acquire(a) atomic_load_explicit((a), memory_order_acquire)
#	define atomic_store_seqcst(a, b) atomic_store_explicit((a), (b), memory_order_seq_cst)
#	define atomic_store_release(a, b) atomic_store_explicit((a), (b), memory_order_release)
#endif

struct event_zone_begin {
	int unsigned tid;
	int unsigned srcloc;
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

struct event {
	char unsigned type;
	union {
		struct event_zone_begin zone_begin;
		struct event_zone_end zone_end;
		struct event_zone_color zone_color;
		struct event_frame_mark frame_mark;
		struct event_plot plot;
	};
};

/* data */
static struct {
	struct string ***strings;
	int unsigned *strings_len;
	struct misc ***miscs;
	int unsigned *miscs_len;
	struct procdef **procdefs;
	int unsigned *procdefs_len;
	void *exec_proc;
	struct object (UTRACY_WINDOWS_CDECL UTRACY_LINUX_REGPARM(3) *orig_exec_proc)(struct proc *);
	void *server_tick;
	int (UTRACY_WINDOWS_STDCALL UTRACY_LINUX_CDECL *orig_server_tick)(void);
	void *send_maps;
	void (UTRACY_WINDOWS_CDECL UTRACY_LINUX_CDECL *orig_send_maps)(void);
} byond;

static struct {
	struct {
		long long init_begin;
		long long init_end;
		double multiplier;
		long long resolution;
		long long delay;
		long long epoch;
		long long exec_time;
	} info;

	HANDLE thread;
	HANDLE quit;
	FILE *fstream;

	struct {
		int unsigned producer_tail_cache;
		int unsigned consumer_head_cache;
		struct event events[EVENT_QUEUE_CAPACITY];

#if defined(__STDC_NO_ATOMICS__)
		long volatile head;
		long volatile tail;
#else
		_Alignas(64) atomic_uint head;
		_Alignas(64) atomic_uint tail;
		_Alignas(64) int padding;
#endif
	} queue;
} utracy;

/* queue api */
UTRACY_INTERNAL UTRACY_INLINE
int event_queue_init(void) {
	utracy.queue.producer_tail_cache = 0;
	utracy.queue.consumer_head_cache = 0;
	atomic_store_seqcst(&utracy.queue.head, 1);
	atomic_store_seqcst(&utracy.queue.tail, 0);
	return 0;
}

UTRACY_INTERNAL UTRACY_INLINE
void event_queue_push(struct event const *const event) {
	int unsigned store = atomic_load_relaxed(&utracy.queue.head);
	int unsigned next_store = store + 1;

	if(next_store == EVENT_QUEUE_CAPACITY) {
		next_store = 0;
	}

	while(unlikely(next_store == utracy.queue.producer_tail_cache)) {
		utracy.queue.producer_tail_cache = atomic_load_acquire(&utracy.queue.tail);
	}

	utracy.queue.events[store] = *event;

	atomic_store_release(&utracy.queue.head, next_store);
}

UTRACY_INTERNAL UTRACY_INLINE
int event_queue_pop(struct event *const event) {
	int unsigned load = atomic_load_relaxed(&utracy.queue.tail);
	int unsigned next_load = load + 1;

	if(load == utracy.queue.consumer_head_cache) {
		utracy.queue.consumer_head_cache = atomic_load_acquire(&utracy.queue.head);
		if(load == utracy.queue.consumer_head_cache) {
			return -1;
		}
	}

	*event = utracy.queue.events[load];

	if(next_load == EVENT_QUEUE_CAPACITY) {
		next_load = 0;
	}

	atomic_store_release(&utracy.queue.tail, next_load);
	return 0;
}

/* profiler */
UTRACY_INTERNAL UTRACY_INLINE
long long utracy_tsc(void) {
#if defined(UTRACY_CLANG) || defined(UTRACY_GCC)
	return (long long) __builtin_ia32_rdtsc();
#elif defined(UTRACY_MSVC)
	return (long long) __rdtsc();
#else
	int unsigned eax, edx;
	__asm__ __volatile__("rdtsc;" :"=a"(eax), "=d"(edx));
	return ((long long) edx << 32) + eax;
#endif
}

#if defined(UTRACY_LINUX)
static int unsigned linux_main_tid;
#endif

UTRACY_INTERNAL UTRACY_INLINE
int unsigned utracy_tid(void) {
#if defined(UTRACY_WINDOWS)
#	if defined(UTRACY_CLANG) || defined(UTRACY_GCC)

	int unsigned tid;
	__asm__("mov %%fs:0x24, %0;" :"=r"(tid));
	return tid;

#	elif defined(UTRACY_MSVC)

	__asm {
		mov eax, fs:[0x24];
	}

#	else

	return GetCurrentThreadId();

#	endif
#elif defined(UTRACY_LINUX)
	/* too damn slow
	return syscall(__NR_gettid); */
	return linux_main_tid;
#endif
}

UTRACY_INTERNAL
double calibrate_multiplier(void) {
#if defined(UTRACY_WINDOWS)
	LARGE_INTEGER li_freq, li_t0, li_t1;
	if(0 == QueryPerformanceFrequency(&li_freq)) {
		LOG_DEBUG_ERROR;
		return 1.0;
	}

	if(0 == QueryPerformanceCounter(&li_t0)) {
		LOG_DEBUG_ERROR;
		return 1.0;
	}

	long long clk0 = utracy_tsc();

	Sleep(100);

	if(0 == QueryPerformanceCounter(&li_t1)) {
		LOG_DEBUG_ERROR;
		return 1.0;
	}

	long long clk1 = utracy_tsc();

	double const freq = li_freq.QuadPart;
	double const t0 = li_t0.QuadPart;
	double const t1 = li_t1.QuadPart;
	double const dt = ((t1 - t0) * 1000000000.0) / freq;
	double const dclk = clk1 - clk0;

	if(clk0 >= clk1) {
		LOG_DEBUG_ERROR;
		return 1.0;
	}

	if(t0 >= t1) {
		LOG_DEBUG_ERROR;
		return 1.0;
	}

	if(0.0 >= dclk) {
		LOG_DEBUG_ERROR;
		return 1.0;
	}

	return dt / dclk;

#elif defined(UTRACY_LINUX)
	struct timespec ts_t0, ts_t1;

interrupted:
	if(-1 == clock_gettime(CLOCK_MONOTONIC_RAW, &ts_t0)) {
		LOG_DEBUG_ERROR;
		return 1.0;
	}

	long long clk0 = utracy_tsc();

	if(-1 == usleep(100000)) {
		LOG_DEBUG_ERROR;

		if(EINTR == errno) {
			goto interrupted;
		}

		return 1.0;
	}

	if(-1 == clock_gettime(CLOCK_MONOTONIC_RAW, &ts_t1)) {
		LOG_DEBUG_ERROR;
		return 1.0;
	}

	long long clk1 = utracy_tsc();

	double const t0 = ts_t0.tv_sec * 1000000000.0 + ts_t0.tv_nsec;
	double const t1 = ts_t1.tv_sec * 1000000000.0 + ts_t1.tv_nsec;
	double const dt = t1 - t0;
	double const dclk = clk1 - clk0;

	if(clk0 >= clk1) {
		LOG_DEBUG_ERROR;
		return 1.0;
	}

	if(t0 >= t1) {
		LOG_DEBUG_ERROR;
		return 1.0;
	}

	if(0.0 >= dclk) {
		LOG_DEBUG_ERROR;
		return 1.0;
	}

	return dt / dclk;

#endif
}

UTRACY_INTERNAL
long long calibrate_resolution(void) {
	/* many iterations may be required to allow the thread time to migrate
	  to a suitable cpu / C-state / P-state */
	int const iterations = 1000000;
	long long resolution = 0x7FFFFFFFFFFFFFFFll;

	for(int i=0; i<iterations; i++) {
		long long clk0 = utracy_tsc();
		long long clk1 = utracy_tsc();
		long long dclk = clk1 - clk0;
		resolution = dclk < resolution ? dclk : resolution;
	}

	return resolution;
}

/* seconds since unix epoch */
UTRACY_INTERNAL
long long unix_timestamp(void) {
#if defined(UTRACY_WINDOWS)
	/* thanks Ian Boyd https://stackoverflow.com/a/46024468 */
	long long const UNIX_TIME_START = 0x019DB1DED53E8000ll;
	long long const TICKS_PER_SECOND = 10000000ll;

	FILETIME ft_timestamp;
	GetSystemTimeAsFileTime(&ft_timestamp);

	LARGE_INTEGER li_timestamp = {
		.LowPart = ft_timestamp.dwLowDateTime,
		.HighPart = ft_timestamp.dwHighDateTime
	};

	return (li_timestamp.QuadPart - UNIX_TIME_START) / TICKS_PER_SECOND;

#elif defined(UTRACY_LINUX)
	struct timespec t;

	if(-1 == clock_gettime(CLOCK_REALTIME, &t)) {
		LOG_DEBUG_ERROR;
		return 0;
	}

	return t.tv_sec;
#endif
}

#define UTRACY_EVT_ZONEBEGIN (15)
#define UTRACY_EVT_ZONEEND (17)
#define UTRACY_EVT_PLOTDATA (43)
#define UTRACY_EVT_THREADCONTEXT (57)
#define UTRACY_EVT_ZONECOLOR (62)
#define UTRACY_EVT_FRAMEMARKMSG (64)

struct utracy_source_location {
	char const *name;
	char const *function;
	char const *file;
	int unsigned line;
	int unsigned color;
};

static struct utracy_source_location srclocs[0x10002];

UTRACY_INTERNAL UTRACY_INLINE
void utracy_emit_zone_begin(int unsigned proc) {
	event_queue_push(&(struct event) {
		.type = UTRACY_EVT_ZONEBEGIN,
		.zone_begin.tid = utracy_tid(),
		.zone_begin.timestamp = utracy_tsc(),
		.zone_begin.srcloc = proc/*(void *) srcloc*/
	});
}

UTRACY_INTERNAL UTRACY_INLINE
void utracy_emit_zone_end(void) {
	event_queue_push(&(struct event) {
		.type = UTRACY_EVT_ZONEEND,
		.zone_end.tid = utracy_tid(),
		.zone_end.timestamp = utracy_tsc()
	});
}

UTRACY_INTERNAL UTRACY_INLINE
void utracy_emit_zone_color(int unsigned color) {
	event_queue_push(&(struct event) {
		.type = UTRACY_EVT_ZONECOLOR,
		.zone_color.tid = utracy_tid(),
		.zone_color.color = color
	});
}

UTRACY_INTERNAL UTRACY_INLINE
void utracy_emit_frame_mark(char *const name) {
	event_queue_push(&(struct event) {
		.type = UTRACY_EVT_FRAMEMARKMSG,
		.frame_mark.name = name,
		.frame_mark.timestamp = utracy_tsc()
	});
}

UTRACY_INTERNAL
long long calibrate_delay(void) {
	(void) UTRACY_MEMSET(utracy.queue.events, 0, sizeof(utracy.queue.events));

	int unsigned const iterations = (EVENT_QUEUE_CAPACITY / 2u) - 1u;

	long long clk0 = utracy_tsc();

	for(int unsigned i=0; i<iterations; i++) {
		utracy_emit_zone_begin(0);
		utracy_emit_zone_end();
	}

	long long clk1 = utracy_tsc();

	long long dclk = clk1 - clk0;

	struct event evt;
	while(0 == event_queue_pop(&evt));

	return dclk / (long long) (iterations * 2);
}

#if 0
static int utracy_write(void const *const buf, size_t size) {
	DWORD written;
	size_t offset = 0;
	while(offset < size) {
		if(FALSE == WriteFile(utracy.stream, (char *const) buf + offset, size - offset, &written, NULL)) {
			LOG_DEBUG_ERROR;
			return -1;
		}

		if(0 == written) {
			LOG_DEBUG_ERROR;
			return -1;
		}

		offset += written;
	}

	return 0;
}
#else
static int utracy_write(void const *const buf, size_t size) {
	fwrite(buf, 1, size, utracy.fstream);
	return 0;
}
#endif

UTRACY_INTERNAL
#if defined(UTRACY_WINDOWS)
DWORD WINAPI utracy_server_thread_start(PVOID user) {
#elif defined(UTRACY_LINUX)
void *utracy_server_thread_start(void *user) {
#endif
	(void) user;

	{
		struct {
			long long unsigned signature;
			int unsigned version;
			double multiplier;
			long long init_begin;
			long long init_end;
			long long delay;
			long long resolution;
			long long epoch;
			long long exec_time;
			long long pid;
			long long sampling_period;
			char unsigned flags;
			char unsigned cpu_arch;
			char cpu_manufacturer[12];
			int unsigned cpu_id;
			char program_name[64];
			char host_info[1024];
		} header = {0};

		header.signature = 0x6D64796361727475llu;
		header.version = 2;
		header.multiplier = utracy.info.multiplier;
		header.init_begin = utracy.info.init_begin;
		header.init_end = utracy.info.init_end;
		header.delay = utracy.info.delay;
		header.resolution = utracy.info.resolution;
		header.epoch = utracy.info.epoch;
		header.exec_time = utracy.info.exec_time;
		header.pid = 0;
		header.sampling_period = 0;
		header.flags = 0;
		header.cpu_arch = 0;
		(void) memcpy(header.cpu_manufacturer, "???", 3);
		header.cpu_id = 0;
		(void) memcpy(header.program_name, "dreamdaemon.exe", 15);
		(void) memcpy(header.host_info, "???", 3);

		(void) utracy_write(&header, sizeof(header));
	}

	int unsigned srclocs_len = sizeof(srclocs) / sizeof(*srclocs);
	(void) utracy_write(&srclocs_len, sizeof(srclocs_len));
	for(int unsigned i=0; i<srclocs_len; i++) {
		struct utracy_source_location srcloc = srclocs[i];

		if(NULL != srcloc.name) {
			int unsigned name_len = strlen(srcloc.name);
			(void) utracy_write(&name_len, sizeof(name_len));
			(void) utracy_write(srcloc.name, name_len);
		} else {
			int unsigned name_len = 0;
			(void) utracy_write(&name_len, sizeof(name_len));
		}

		if(NULL != srcloc.function) {
			int unsigned function_len = strlen(srcloc.function);
			(void) utracy_write(&function_len, sizeof(function_len));
			(void) utracy_write(srcloc.function, function_len);
		} else {
			int unsigned function_len = 0;
			(void) utracy_write(&function_len, sizeof(function_len));
		}

		if(NULL != srcloc.file) {
			int unsigned file_len = strlen(srcloc.file);
			(void) utracy_write(&file_len, sizeof(file_len));
			(void) utracy_write(srcloc.file, file_len);
		} else {
			int unsigned file_len = 0;
			(void) utracy_write(&file_len, sizeof(file_len));
		}

		(void) utracy_write(&srcloc.line, sizeof(srcloc.line));
		(void) utracy_write(&srcloc.color, sizeof(srcloc.color));
	}

	int quitting = 0;
	while(!quitting) {
		struct event evt;
		while(0 == event_queue_pop(&evt)) {
			(void) utracy_write(&evt, sizeof(evt));
		}

		switch(WaitForSingleObject(utracy.quit, 1)) {
			default:
				LOG_DEBUG_ERROR;
			case WAIT_OBJECT_0:
				quitting = 1;
				break;
			case WAIT_TIMEOUT:
				break;
		}
	}

#if defined(UTRACY_WINDOWS)
	ExitThread(0);
#elif defined(UTRACY_LINUX)
	pthread_exit(NULL);
#endif
}

/* byond hooks */
UTRACY_INTERNAL
struct object UTRACY_WINDOWS_CDECL UTRACY_LINUX_REGPARM(3) exec_proc(struct proc *proc) {
	if(likely(proc->procdef < 0x10000)) {
		utracy_emit_zone_begin(proc->procdef);

		/* procs with pre-existing contexts are resuming from sleep */
		if(unlikely(proc->ctx != NULL)) {
			utracy_emit_zone_color(0xAF4444);
		}

		struct object result = byond.orig_exec_proc(proc);

		utracy_emit_zone_end();

		return result;
	}

	return byond.orig_exec_proc(proc);
}

UTRACY_INTERNAL
int UTRACY_WINDOWS_STDCALL UTRACY_LINUX_CDECL server_tick(void) {
	/* server tick is the end of a frame and the beginning of the next frame */
	utracy_emit_frame_mark(NULL);

	utracy_emit_zone_begin(0x10000);

	int interval = byond.orig_server_tick();

	utracy_emit_zone_end();

	return interval;
}

UTRACY_INTERNAL
void UTRACY_WINDOWS_CDECL UTRACY_LINUX_CDECL send_maps(void) {
	utracy_emit_zone_begin(0x10001);

	byond.orig_send_maps();

	utracy_emit_zone_end();
}

/* hooking */
UTRACY_INTERNAL
void *hook(char *const restrict dst, char *const restrict src, char unsigned size) {
	char unsigned jmp[] = {
		0xE9, 0x00, 0x00, 0x00, 0x00
	};

#if defined(UTRACY_WINDOWS)
	char *trampoline = VirtualAlloc(NULL, 4096, MEM_COMMIT, PAGE_READWRITE);

#elif defined(UTRACY_LINUX)
	long page_size = sysconf(_SC_PAGE_SIZE);
	char *trampoline = aligned_alloc(page_size, page_size);

#endif

	if(NULL == trampoline) {
		return NULL;
	}

	uintptr_t jmp_from = (uintptr_t) trampoline + size + sizeof(jmp);
	uintptr_t jmp_to = (uintptr_t) src + size;
	uintptr_t offset = jmp_to - jmp_from;
	(void) UTRACY_MEMCPY(jmp + 1, &offset, sizeof(offset));
	(void) UTRACY_MEMCPY(trampoline, src, size);
	(void) UTRACY_MEMCPY(trampoline + size, jmp, sizeof(jmp));

#if defined(UTRACY_WINDOWS)
	DWORD old_protect;
	if(0 == VirtualProtect(trampoline, 4096, PAGE_EXECUTE_READWRITE, &old_protect)) {
		LOG_DEBUG_ERROR;
		(void) VirtualFree(trampoline, 0, MEM_RELEASE);
		return NULL;
	}

#elif defined(UTRACY_LINUX)
	if(0 != mprotect(trampoline, page_size, PROT_WRITE | PROT_READ | PROT_EXEC)) {
		LOG_DEBUG_ERROR;
		free(trampoline);
		return NULL;
	}

#endif

	jmp_from = (uintptr_t) src + sizeof(jmp);
	jmp_to = (uintptr_t) dst;
	offset = jmp_to - jmp_from;

#if defined(UTRACY_WINDOWS)
	if(0 == VirtualProtect(src, size, PAGE_READWRITE, &old_protect)) {
		LOG_DEBUG_ERROR;
		(void) VirtualFree(trampoline, 0, MEM_RELEASE);
		return NULL;
	}

#elif defined(UTRACY_LINUX)
	if(0 != mprotect((void *) ((uintptr_t) src & ((uintptr_t) -page_size)), page_size, PROT_WRITE | PROT_READ | PROT_EXEC)) {
		LOG_DEBUG_ERROR;
		free(trampoline);
		return NULL;
	}

#endif

	(void) UTRACY_MEMCPY(jmp + 1, &offset, sizeof(offset));
	(void) UTRACY_MEMCPY(src, &jmp, sizeof(jmp));

	if(size > sizeof(jmp)) {
		for(size_t i=0; i<(size - sizeof(jmp)); i++) {
			char unsigned nop = 0x90;
			(void) UTRACY_MEMCPY(src + sizeof(jmp) + i, &nop, 1);
		}
	}

#if defined(UTRACY_WINDOWS)
	if(0 == VirtualProtect(src, size, old_protect, &old_protect)) {
		LOG_DEBUG_ERROR;
		return NULL;
	}

#elif defined(UTRACY_LINUX)
	if(0 != mprotect((void *) ((uintptr_t) src & (uintptr_t) -page_size), page_size, PROT_WRITE | PROT_READ | PROT_EXEC)) {
		LOG_DEBUG_ERROR;
		return NULL;
	}

#endif

	return trampoline;
}

#if defined(UTRACY_WINDOWS)
#define BYOND_MAX_BUILD 1596
#define BYOND_MIN_BUILD 1543
#define BYOND_VERSION_ADJUSTED(a) ((a) - BYOND_MIN_BUILD)
int unsigned byond_offsets[][7] = {
	/*                                strings     miscs       procdefs    exec_proc   server_tick send_maps   prologue */
	[BYOND_VERSION_ADJUSTED(1543)] = {0x0035FC58, 0x0035FC68, 0x0035FC78, 0x001003B0, 0x001C7D20, 0x00187C80, 0x00050B06},
	[BYOND_VERSION_ADJUSTED(1544)] = {0x00360C58, 0x00360C68, 0x00360C78, 0x00100A10, 0x001C8420, 0x00188220, 0x00050B06},
	[BYOND_VERSION_ADJUSTED(1545)] = {0x00360C60, 0x00360C70, 0x00360C80, 0x00100980, 0x001C8400, 0x00188190, 0x00050B06},
	[BYOND_VERSION_ADJUSTED(1546)] = {0x00360C60, 0x00360C70, 0x00360C80, 0x00100830, 0x001C8280, 0x001880C0, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1547)] = {0x00362C68, 0x00362C78, 0x00362C88, 0x00101210, 0x001C9320, 0x001891F0, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1548)] = {0x00362C48, 0x00362C58, 0x00362C68, 0x00101640, 0x001C96D0, 0x00188E80, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1549)] = {0x00368DD4, 0x00368DEC, 0x00368E00, 0x001023B0, 0x001CB0A0, 0x0018AD80, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1550)] = {0x0036903C, 0x0036904C, 0x0036905C, 0x00102710, 0x001CB710, 0x0018B0B0, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1551)] = {0x00369034, 0x00369044, 0x00369054, 0x00102C30, 0x001CB830, 0x0018B120, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1552)] = {0x0036A054, 0x0036A064, 0x0036A074, 0x00102DE0, 0x001CBDE0, 0x0018B6B0, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1553)] = {0x0036E234, 0x0036E244, 0x0036E254, 0x00104FF0, 0x001CF780, 0x0018DE50, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1554)] = {0x0036DFF8, 0x0036E008, 0x0036E018, 0x00104ED0, 0x001CF650, 0x0018E000, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1555)] = {0x0036E0B0, 0x0036E0C0, 0x0036E0D0, 0x001064F0, 0x001CFD80, 0x0018EEB0, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1556)] = {0x0036E0AC, 0x0036E0BC, 0x0036E0CC, 0x00106560, 0x001CFD80, 0x0018EEE0, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1557)] = {0x0036E0C0, 0x0036E0D0, 0x0036E0E0, 0x001063B0, 0x001CFB60, 0x0018EC70, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1558)] = {0x0036F4F4, 0x0036F504, 0x0036F514, 0x00106DE0, 0x001D1160, 0x0018FD80, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1559)] = {0x0036F4F4, 0x0036F504, 0x0036F514, 0x00106DE0, 0x001D1160, 0x0018FD80, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1560)] = {0x0036F4F4, 0x0036F504, 0x0036F514, 0x00106AF0, 0x001D1120, 0x0018FA80, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1561)] = {0x0036F4F4, 0x0036F504, 0x0036F514, 0x00106AF0, 0x001D1120, 0x0018FA80, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1562)] = {0x0036F538, 0x0036F548, 0x0036F558, 0x00106960, 0x001D0F00, 0x0018F780, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1563)] = {0x0036F538, 0x0036F548, 0x0036F558, 0x001066A0, 0x001D1160, 0x0018F660, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1564)] = {0x0036F538, 0x0036F548, 0x0036F558, 0x00106310, 0x001D0F20, 0x0018F1E0, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1565)] = {0x00371538, 0x00371548, 0x00371558, 0x00106960, 0x001D15A0, 0x0018FCC0, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1566)] = {0x00371538, 0x00371548, 0x00371558, 0x00106160, 0x001D0A70, 0x0018EF80, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1567)] = {0x00370548, 0x00370560, 0x00370570, 0x00106220, 0x001D0B00, 0x0018F470, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1568)] = {0x00370548, 0x00370560, 0x00370570, 0x00106220, 0x001D0B30, 0x0018F470, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1569)] = {0x00370548, 0x00370560, 0x00370570, 0x00106220, 0x001D0B40, 0x0018F500, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1570)] = {0x00371548, 0x00371558, 0x00371568, 0x00106560, 0x001D0BF0, 0x0018F8F0, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1571)] = {0x00371548, 0x00371558, 0x00371568, 0x001061D0, 0x001D0A70, 0x0018F500, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1572)] = {0x00371540, 0x00371550, 0x00371560, 0x001066A0, 0x001D0F60, 0x0018FCC0, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1573)] = {0x00371608, 0x00371618, 0x00371628, 0x00106BD0, 0x001D13C0, 0x0018FC40, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1574)] = {0x00371550, 0x00371560, 0x00371570, 0x001065A0, 0x001D10E0, 0x0018FDC0, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1575)] = {0x00371550, 0x00371560, 0x00371570, 0x001065A0, 0x001D10E0, 0x0018FDC0, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1576)] = {0x003745BC, 0x003745CC, 0x003745DC, 0x001087B0, 0x001D30A0, 0x00191C60, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1577)] = {0x003745BC, 0x003745CC, 0x003745DC, 0x00107FC0, 0x001D2C90, 0x00191A60, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1578)] = {0x003745BC, 0x003745CC, 0x003745DC, 0x001083B0, 0x001D2E90, 0x00191910, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1579)] = {0x003745C8, 0x003745D8, 0x003745E8, 0x00108C20, 0x001D3940, 0x001925C0, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1580)] = {0x003745C8, 0x003745D8, 0x003745E8, 0x00108BD0, 0x001D38B0, 0x00192520, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1581)] = {0x003745C8, 0x003745D8, 0x003745E8, 0x001086A0, 0x001D3780, 0x001923A0, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1582)] = {0x003745C8, 0x003745D8, 0x003745E8, 0x001087B0, 0x001D3A40, 0x00191FF0, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1583)] = {0x003755C8, 0x003755D8, 0x003755E8, 0x00108240, 0x001D33F0, 0x001919E0, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1584)] = {0x003764B4, 0x003764C4, 0x003764D4, 0x00108460, 0x001D3A40, 0x001922C0, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1585)] = {0x003774AC, 0x003774BC, 0x003774CC, 0x001094D0, 0x001D49E0, 0x00192D80, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1586)] = {0x00378524, 0x00378534, 0x00378544, 0x00109AA0, 0x001D5160, 0x00193370, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1587)] = {0x00378524, 0x00378534, 0x00378544, 0x00109AA0, 0x001D5160, 0x00193370, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1588)] = {0x00378524, 0x00378534, 0x00378544, 0x00109B10, 0x001D5220, 0x00193840, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1589)] = {0x00378524, 0x00378534, 0x00378544, 0x00109AA0, 0x001D5190, 0x00193710, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1590)] = {0x00396974, 0x00396984, 0x00396994, 0x00118180, 0x001EA800, 0x001A6A80, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1591)] = {0x00396974, 0x00396984, 0x00396994, 0x001175E0, 0x001E9F00, 0x001A5F00, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1592)] = {0x00396974, 0x00396984, 0x00396994, 0x00117890, 0x001EA900, 0x001A6380, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1593)] = {0x00396974, 0x00396984, 0x00396994, 0x00118090, 0x001EAB30, 0x001A6920, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1594)] = {0x00397B6C, 0x00397B7C, 0x00397B8C, 0x00118590, 0x001EBBB0, 0x001A8140, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1595)] = {0x0039AB58, 0x0039AB68, 0x0039AB78, 0x0011A810, 0x001EED90, 0x001AB310, 0x00050606},
	[BYOND_VERSION_ADJUSTED(1596)] = {0x0039AB58, 0x0039AB68, 0x0039AB78, 0x0011A090, 0x001EE950, 0x001AAC20, 0x00050606},
};

#elif defined(UTRACY_LINUX)
#	define BYOND_MAX_BUILD 1592
#	define BYOND_MIN_BUILD 1543
#	define BYOND_VERSION_ADJUSTED(a) ((a) - BYOND_MIN_BUILD)

static int unsigned const byond_offsets[][7] = {
	/*                                strings     miscs       procdefs    exec_proc   server_tick send_maps   prologue */
	[BYOND_VERSION_ADJUSTED(1543)] = {0x0063F9B8, 0x0063F9D0, 0x0063FA0C, 0x002E31E0, 0x002B7710, 0x002B28D0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1544)] = {0x00640BB8, 0x00640BD0, 0x00640C0C, 0x002E3A60, 0x002B7F90, 0x002B3150, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1545)] = {0x006409D8, 0x006409F0, 0x00640A2C, 0x002E3D00, 0x002B8230, 0x002B33F0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1546)] = {0x006409D8, 0x006409F0, 0x00640A2C, 0x002E3ED0, 0x002B83F0, 0x002B3570, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1547)] = {0x00642A38, 0x00642A50, 0x00642A8C, 0x002E4D30, 0x002B8F40, 0x002B4320, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1548)] = {0x00643A38, 0x00643A50, 0x00643A8C, 0x002E5CB0, 0x002B9ED0, 0x002B52B0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1549)] = {0x006459D8, 0x006459F0, 0x00645A2C, 0x002E6C30, 0x002BADD0, 0x002B5F10, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1550)] = {0x006469D8, 0x006469F0, 0x00646A2C, 0x002E7B80, 0x002BB910, 0x002B6A50, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1551)] = {0x006469D8, 0x006469F0, 0x00646A2C, 0x002E77C0, 0x002BB520, 0x002B6660, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1552)] = {0x006469D8, 0x006469F0, 0x00646A2C, 0x002E7D20, 0x002BBA70, 0x002B6BB0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1553)] = {0x00651B18, 0x00651B30, 0x00651B6C, 0x002F1490, 0x002C51E0, 0x002BCE30, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1554)] = {0x00651B18, 0x00651B30, 0x00651B6C, 0x002F1D10, 0x002C5280, 0x002BCED0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1555)] = {0x00653B28, 0x00653B40, 0x00653B8C, 0x002F2EA0, 0x002C5F90, 0x002BE1A0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1556)] = {0x00653B28, 0x00653B40, 0x00653B8C, 0x002F2BE0, 0x002C5CD0, 0x002BDEE0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1557)] = {0x00653B28, 0x00653B40, 0x00653B8C, 0x002F2A40, 0x002C5B40, 0x002BDD50, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1558)] = {0x00656B48, 0x00656B60, 0x00656BAC, 0x002F5020, 0x002C8070, 0x002C0280, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1559)] = {0x00656B48, 0x00656B60, 0x00656BAC, 0x002F5020, 0x002C8070, 0x002C0280, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1560)] = {0x00656B48, 0x00656B60, 0x00656BAC, 0x002F5040, 0x002C8090, 0x002C02A0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1562)] = {0x0065AB48, 0x0065AB60, 0x0065ABAC, 0x002F89B0, 0x002CBA20, 0x002C3C30, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1563)] = {0x0065ABC8, 0x0065ABE0, 0x0065AC2C, 0x002F87E0, 0x002CB850, 0x002C3A60, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1564)] = {0x00659B48, 0x00659B60, 0x00659BAC, 0x002F8680, 0x002CB6F0, 0x002C3900, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1565)] = {0x0065FB48, 0x0065FB60, 0x0065FBAC, 0x002F9990, 0x002CCA00, 0x002C4C10, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1566)] = {0x0065EB48, 0x0065EB60, 0x0065EBAC, 0x002F8830, 0x002CB8A0, 0x002C3AB0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1567)] = {0x0065CB48, 0x0065CB60, 0x0065CBAC, 0x002F74D0, 0x002CA480, 0x002C2690, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1568)] = {0x0065CB48, 0x0065CB60, 0x0065CBAC, 0x002F74D0, 0x002CA480, 0x002C2690, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1569)] = {0x0065CB48, 0x0065CB60, 0x0065CBAC, 0x002F74C0, 0x002CA470, 0x002C2680, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1570)] = {0x0065CB48, 0x0065CB60, 0x0065CBAC, 0x002F78E0, 0x002CA870, 0x002C2A90, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1571)] = {0x0065CB48, 0x0065CB60, 0x0065CBAC, 0x002F7900, 0x002CA890, 0x002C2AB0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1572)] = {0x0065DB48, 0x0065DB60, 0x0065DBAC, 0x002F8110, 0x002CB0A0, 0x002C32C0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1573)] = {0x0065DC28, 0x0065DC40, 0x0065DC8C, 0x002F7EE0, 0x002CAE70, 0x002C3090, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1574)] = {0x0065DB68, 0x0065DB80, 0x0065DBCC, 0x002F8280, 0x002CB210, 0x002C3430, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1575)] = {0x0065DB68, 0x0065DB80, 0x0065DBCC, 0x002F8280, 0x002CB210, 0x002C3430, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1576)] = {0x00664BC8, 0x00664BE0, 0x00664C2C, 0x002FCFC0, 0x002CFF50, 0x002C8170, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1577)] = {0x00664BC8, 0x00664BE0, 0x00664C2C, 0x002FCFD0, 0x002CFF60, 0x002C8180, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1578)] = {0x00664D08, 0x00664D20, 0x00664D6C, 0x002FC5D0, 0x002CF550, 0x002C7770, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1579)] = {0x00664BCC, 0x00664BE4, 0x00664C2C, 0x002FC740, 0x002CF590, 0x002C77B0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1580)] = {0x00664BCC, 0x00664BE4, 0x00664C2C, 0x002FC760, 0x002CF5A0, 0x002C77C0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1581)] = {0x00664BCC, 0x00664BE4, 0x00664C2C, 0x002FC740, 0x002CF580, 0x002C77A0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1582)] = {0x00666C2C, 0x00666C44, 0x00666C8C, 0x002FCEF0, 0x002CFBE0, 0x002C7E00, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1583)] = {0x00666BCC, 0x00666BE4, 0x00666C2C, 0x002FCEF0, 0x002CFBE0, 0x002C7E00, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1584)] = {0x00668BCC, 0x00668BE4, 0x00668C2C, 0x002FD510, 0x002D0200, 0x002C85D0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1585)] = {0x0066BBEC, 0x0066BC04, 0x0066BC4C, 0x00300350, 0x002D2E90, 0x002CB2B0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1586)] = {0x0066FC0C, 0x0066FC24, 0x0066FC6C, 0x00303C40, 0x002D6770, 0x002CE3D0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1587)] = {0x0066FC0C, 0x0066FC24, 0x0066FC6C, 0x00303CF0, 0x002D6820, 0x002CE480, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1588)] = {0x0066FC0C, 0x0066FC24, 0x0066FC6C, 0x00303CC0, 0x002D67F0, 0x002CE450, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1589)] = {0x00671C0C, 0x00671C24, 0x00671C6C, 0x00305550, 0x002D80A0, 0x002CFD50, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1590)] = {0x006B15E8, 0x006B1600, 0x006B164C, 0x00313220, 0x002FFBA0, 0x002F5DA0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1591)] = {0x006B17C8, 0x006B17E0, 0x006B182C, 0x00313440, 0x002FFDC0, 0x002F5DC0, 0x00050505},
	[BYOND_VERSION_ADJUSTED(1592)] = {0x006B19C8, 0x006B19E0, 0x006B1A2C, 0x003135F0, 0x002FFF70, 0x002F5F70, 0x00050505},
};

#endif

UTRACY_INTERNAL
void build_srclocs(void) {
#define byond_get_string(id) (id < *byond.strings_len ? *(*byond.strings + id) : NULL)
#define byond_get_misc(id) (id < *byond.miscs_len ? *(*byond.miscs + id) : NULL)
#define byond_get_procdef(id) (id < *byond.procdefs_len ? *byond.procdefs + id : NULL)

	for(int unsigned i=0; i<0x10000; i++) {
		char const *name = NULL;
		char const *function = "<?>";
		char const *file = "<?.dm>";
		int unsigned line = 0xFFFFFFFFu;
		int unsigned color = 0x4444AF;

		struct procdef const *const procdef = byond_get_procdef(i);
		if(procdef != NULL) {
			struct string const *const str = byond_get_string(procdef->path);
			if(str != NULL && str->data != NULL) {
				function = str->data;
			}

			struct misc const *const misc = byond_get_misc(procdef->bytecode);
			if(misc != NULL) {
				int unsigned bytecode_len = misc->bytecode.len;
				int unsigned *bytecode = misc->bytecode.bytecode;
				if(bytecode_len >= 2) {
					if(bytecode[0x00] == 0x84) {
						int unsigned file_id = bytecode[0x01];
						struct string const *const file_str = byond_get_string(file_id);
						if(file_str != NULL && file_str->data != NULL) {
							file = file_str->data;
						}

						if(bytecode_len >= 4) {
							if(bytecode[0x02] == 0x85) {
								line = bytecode[0x03];
							}
						}
					}
				}
			}
		}

		srclocs[i] = (struct utracy_source_location) {
			.name = name,
			.function = function,
			.file = file,
			.line = line,
			.color = color
		};
	}

	srclocs[0x10000] = (struct utracy_source_location) {
		.name = NULL,
		.function = "ServerTick",
		.file = __FILE__,
		.line = __LINE__,
		.color = 0x44AF44
	};

	srclocs[0x10001] = (struct utracy_source_location) {
		.name = NULL,
		.function = "SendMaps",
		.file = __FILE__,
		.line = __LINE__,
		.color = 0x44AF44
	};

#undef byond_get_string
#undef byond_get_misc
#undef byond_get_procdef
}

/* byond api */
static int initialized = 0;

UTRACY_EXTERNAL
char *UTRACY_WINDOWS_CDECL UTRACY_LINUX_CDECL init(int argc, char **argv) {
	(void) argc;
	(void) argv;

	printf("hello world?\n");

	if(0 != initialized) {
		return "already initialized";
	}

	(void) UTRACY_MEMSET(&byond, 0, sizeof(byond));
	(void) UTRACY_MEMSET(&utracy, 0, sizeof(utracy));

	utracy.info.init_begin = utracy_tsc();

	if(0 != event_queue_init()) {
		LOG_DEBUG_ERROR;
		return "event_queue_init failed";
	}

	typedef int (*PFN_GETBYONDBUILD)(void);
	PFN_GETBYONDBUILD GetByondBuild;

#if defined(UTRACY_WINDOWS)
	char *byondcore = (char *) GetModuleHandleA("byondcore.dll");
	if(NULL == byondcore) {
		LOG_DEBUG_ERROR;
		return "unable to find base address of byondcore.dll";
	}

	GetByondBuild = (PFN_GETBYONDBUILD) GetProcAddress(
		(HMODULE) byondcore,
		"?GetByondBuild@ByondLib@@QAEJXZ"
	);
	if(NULL == GetByondBuild) {
		LOG_DEBUG_ERROR;
		return "unable to find GetByondBuild";
	}

#elif defined(UTRACY_LINUX)
	struct link_map *libbyond = dlopen("libbyond.so", RTLD_NOLOAD);
	if(NULL == libbyond) {
		LOG_DEBUG_ERROR;
		return "unable to find base address of libbyond.so";
	}

	GetByondBuild = dlsym(libbyond, "_ZN8ByondLib13GetByondBuildEv");
	if(NULL == GetByondBuild) {
		LOG_DEBUG_ERROR;
		return "unable to find GetByondBuild";
	}

#endif

	int byond_build = GetByondBuild();
	if(byond_build < BYOND_MIN_BUILD || byond_build > BYOND_MAX_BUILD) {
		LOG_DEBUG_ERROR;
		return "byond version unsupported";
	}

	int unsigned const *const offsets = byond_offsets[BYOND_VERSION_ADJUSTED(byond_build)];

	for(int i=0; i<7; i++) {
		if(offsets[i] == 0) {
			LOG_DEBUG_ERROR;
			return "byond version unsupported";
		}
	}

	char unsigned prologues[3];

#if defined(UTRACY_WINDOWS)
	byond.strings = (void *) (byondcore + offsets[0]);
	byond.strings_len = (void *) (byondcore + offsets[0] + 0x04);
	byond.miscs = (void *) (byondcore + offsets[1]);
	byond.miscs_len = (void *) (byondcore + offsets[1] + 0x04);
	byond.procdefs = (void *) (byondcore + offsets[2]);
	byond.procdefs_len = (void *) (byondcore + offsets[2] + 0x04);
	byond.exec_proc = (void *) (byondcore + offsets[3]);
	byond.server_tick = (void *) (byondcore + offsets[4]);
	byond.send_maps = (void *) (byondcore + offsets[5]);
	prologues[0] = (offsets[6] >> 0) & 0xFF;
	prologues[1] = (offsets[6] >> 8) & 0xFF;
	prologues[2] = (offsets[6] >> 16) & 0xFF;

#elif defined(UTRACY_LINUX)
	byond.strings = (void *) (libbyond->l_addr + offsets[0]);
	byond.strings_len = (void *) (libbyond->l_addr + offsets[0] + 0x04);
	byond.miscs = (void *) (libbyond->l_addr + offsets[1]);
	byond.miscs_len = (void *) (libbyond->l_addr + offsets[1] + 0x04);
	byond.procdefs = (void *) (libbyond->l_addr + offsets[2]);
	byond.procdefs_len = (void *) (libbyond->l_addr + offsets[2] + 0x04);
	byond.exec_proc = (void *) (libbyond->l_addr + offsets[3]);
	byond.server_tick = (void *) (libbyond->l_addr + offsets[4]);
	byond.send_maps = (void *) (libbyond->l_addr + offsets[5]);
	prologues[0] = (offsets[6] >> 0) & 0xFF;
	prologues[1] = (offsets[6] >> 8) & 0xFF;
	prologues[2] = (offsets[6] >> 16) & 0xFF;

#endif

	LOG_INFO("byond build = %d\n", byond_build);

	byond.orig_exec_proc = hook((void *) exec_proc, byond.exec_proc, prologues[0]);
	if(NULL == byond.orig_exec_proc) {
		LOG_DEBUG_ERROR;
		return "failed to hook exec_proc";
	}

	byond.orig_server_tick = hook((void *) server_tick, byond.server_tick, prologues[1]);
	if(NULL == byond.orig_server_tick) {
		LOG_DEBUG_ERROR;
		return "failed to hook server_tick";
	}

	byond.orig_send_maps = hook((void *) send_maps, byond.send_maps, prologues[2]);
	if(NULL == byond.orig_send_maps) {
		LOG_DEBUG_ERROR;
		return "failed to hook send_maps";
	}

	build_srclocs();

#if defined(UTRACY_LINUX)
	linux_main_tid = syscall(__NR_gettid);
#endif

	utracy.quit = CreateEventA(NULL, TRUE, FALSE, NULL);
	if(NULL == utracy.quit) {
		LOG_DEBUG_ERROR;
		return "CreateEventA failed";
	}

	(void) CreateDirectoryW(L".\\data", NULL);
	(void) CreateDirectoryW(L".\\data\\profiler", NULL);

	char ffilename[MAX_PATH];
	snprintf(ffilename, MAX_PATH, ".\\data\\profiler\\%llu.utracy", utracy_tsc());
	utracy.fstream = fopen(ffilename, "wb");
	if(NULL == utracy.fstream) {
		LOG_DEBUG_ERROR;
		return "fopen failed";
	}

	utracy.info.resolution = calibrate_resolution();
	utracy.info.delay = calibrate_delay();
	utracy.info.multiplier = calibrate_multiplier();
	utracy.info.epoch = unix_timestamp();
	utracy.info.exec_time = unix_timestamp();
	utracy.info.init_end = utracy_tsc();

#if defined(UTRACY_WINDOWS)
	if(NULL == (utracy.thread = CreateThread(NULL, 0, utracy_server_thread_start, NULL, 0, NULL))) {
		LOG_DEBUG_ERROR;
		fclose(utracy.fstream);
		return "CreateThread failed";
	}

#elif defined(UTRACY_LINUX)
	pthread_t thr;
	if(0 != pthread_create(&thr, NULL, utracy_server_thread_start, NULL)) {
		LOG_DEBUG_ERROR;
		return "pthread_create failed";
	}

#endif

	initialized = 1;
	return "0";
}

UTRACY_EXTERNAL
char *UTRACY_WINDOWS_CDECL UTRACY_LINUX_CDECL destroy(int argc, char **argv) {
	(void) argc;
	(void) argv;

	/* not yet implemented */
	if(1 != initialized) {
		return "not initialized";
	}

	(void) SetEvent(utracy.quit);

	switch(WaitForSingleObject(utracy.thread, INFINITE)) {
		case WAIT_OBJECT_0:
			break;
		default:
			LOG_DEBUG_ERROR;
	}

	fclose(utracy.fstream);
	initialized = 0;

	return "0";
}

#if (__STDC_HOSTED__ == 0)
BOOL WINAPI DllMainCRTStartup(HINSTANCE instance, DWORD reason, LPVOID reserved) {
	return TRUE;
}
#endif
