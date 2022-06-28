#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <emmintrin.h>
#include <intrin.h>
#include "lz4.h"
#include "queue.h"
#include "microtracy.h"
#include <stdio.h>

#define DEBUG_DIAGNOSTICS printf("%s %s:%d (wsa=%d)\n", __func__, __FILE__, __LINE__, WSAGetLastError())
#define TARGET_FRAME_SIZE (256 * 1024)

static struct {
	struct {
		long long init_begin;
		long long init_end;
		double multiplier;
		long long resolution;
		long long delay;
		long long epoch;
		long long exectime;
		DWORD last_heartbeat;
	} info;
	struct {
		WSADATA wsadata;
		SOCKET server;
		SOCKET client;
	} sock;

	/* 16KB + 4 bytes for the default LZ4_MEMORY_USAGE 14 */
	LZ4_stream_t stream;

	int unsigned buf_len;
	char *frame_buf;
	/* lz4 compressed buffer */
	char *buf;

	int unsigned raw_buf_len;
	int unsigned raw_buf_head;
	int unsigned raw_buf_tail;
	char *raw_buf;

	struct event_queue event_queue;
	int unsigned current_tid;
	long long thread_time;
} utracy;

inline long long utracy_tsc(void) {
#if defined(__clang__) || defined(__GNUC__)
	return (long long) __builtin_ia32_rdtsc();
#elif defined(_MSC_VER)
	return (long long) __rdtsc();
#else
	int unsigned eax, edx;
	__asm__ __volatile__("rdtsc" :"=a"(eax), "=d"(edx));
	return ((long long) edx << 32) + eax;
#endif
}

inline long long utracy_qpc(void) {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
}

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

	LARGE_INTEGER lifreq;
	QueryPerformanceFrequency(&lifreq);

	long long dr = a1 - a0;
	double dt = (((double) (b1 - b0)) * 1000000000.0  / (double) lifreq.QuadPart);
	double mul = (double) dt / (double) dr;
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

	struct ___tracy_source_location_data srcloc = {
		.name = NULL,
		.function = __func__,
		.file = __FILE__,
		.line = 0,
		.color = 0
	};

	struct event zone_begin = {
		.type = UTRACY_QUEUE_TYPE_ZONEBEGIN,
		.timestamp = utracy_tsc(),
		.srcloc = &srcloc
	};

	struct event zone_end = {
		.type = UTRACY_QUEUE_TYPE_ZONEEND,
		.timestamp = utracy_tsc()
	};

	_mm_mfence();
	long long begin_tsc = utracy_tsc();

	for(int unsigned i=0; i<iterations; i++) {
		event_queue_enqueue(&utracy.event_queue, &zone_begin);
		event_queue_enqueue(&utracy.event_queue, &zone_end);
	}

	long long end_tsc = utracy_tsc();
	_mm_mfence();
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

static int utracy_server_init(void) {
	if(0 != WSAStartup(MAKEWORD(2, 2), &utracy.sock.wsadata)) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	ADDRINFOEXW *addrinfo;

	if(0 != GetAddrInfoExW(
		NULL,
		L"8086",
		NS_DNS,
		NULL,
		&(ADDRINFOEXW) {
			.ai_flags = 0,
			.ai_family = AF_INET,
			.ai_socktype = SOCK_STREAM,
			.ai_protocol = 0,
			.ai_addrlen = 0,
			.ai_canonname = NULL,
			.ai_addr = NULL,
			.ai_blob = NULL,
			.ai_bloblen = 0,
			.ai_provider = NULL,
			.ai_next = NULL,
		},
		&addrinfo,
		0,
		NULL,
		NULL,
		NULL
	)) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	utracy.sock.server = WSASocketW(
		addrinfo->ai_family,
		addrinfo->ai_socktype,
		addrinfo->ai_protocol,
		NULL,
		0,
		WSA_FLAG_NO_HANDLE_INHERIT
	);

	if(INVALID_SOCKET == utracy.sock.server) {
		DEBUG_DIAGNOSTICS;
		FreeAddrInfoExW(addrinfo);
		return -1;
	}

	FreeAddrInfoExW(addrinfo);

	if(0 != bind(utracy.sock.server, addrinfo->ai_addr, addrinfo->ai_addrlen)) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	if(0 != listen(utracy.sock.server, 2)) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	return 0;
}

static int utracy_accept(void) {
	utracy.sock.client = WSAAccept(
		utracy.sock.server,
		NULL,
		0,
		NULL,
		0
	);

	if(INVALID_SOCKET == utracy.sock.client) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	return 0;
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
		.pid = 19768,
		.samplingPeriod = 0,
		.flags = 8,
		.cpuArch = 1,
		//.cpuManufacturer = "test",
		.cpuId = 0x80F82
		//.programName = "test",
		//.hostInfo = "test"
	};

	memset(welcome_msg.cpuManufacturer, 0, sizeof(welcome_msg.cpuManufacturer));
	memset(welcome_msg.programName, 0, sizeof(welcome_msg.programName));
	memset(welcome_msg.hostInfo, 0, sizeof(welcome_msg.hostInfo));
	memcpy(welcome_msg.cpuManufacturer, "???", 12);
	memcpy(welcome_msg.programName, "dreamdaemon.exe", 15);
	memcpy(welcome_msg.hostInfo, "???", 22);

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

	if(now - utracy.info.last_heartbeat >= 1000) {
		utracy.info.last_heartbeat = now;

		struct event evt = {
			.type = UTRACY_QUEUE_TYPE_SYSTIMEREPORT,
			.timestamp = utracy_tsc(),
			.system_time = -1.0f
		};

		event_queue_enqueue(&utracy.event_queue, &evt);
	}

	return 0;
}

static int utracy_commit(void) {
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

			long long unsigned name = (long long unsigned) "(debug) events remaining in queue";
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
#if defined(__GNUC__) || defined(__clang__)
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

static int utracy_handle_request(void) {
	char unsigned recv_type;
	if(0 != utracy_recv_all(&recv_type, sizeof(recv_type))) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	long long unsigned recv_ptr;
	if(0 != utracy_recv_all(&recv_ptr, sizeof(recv_ptr))) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	int unsigned recv_extra;
	if(0 != utracy_recv_all(&recv_extra, sizeof(recv_extra))) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	switch(recv_type) {
		case UTRACY_RECV_QUERY_STRING:
		case UTRACY_RECV_QUERY_PLOTNAME:
			{
				char unsigned type;

				switch(recv_type) {
					case UTRACY_RECV_QUERY_STRING:
						type = UTRACY_QUEUE_TYPE_STRINGDATA;
						break;
					case UTRACY_RECV_QUERY_PLOTNAME:
						type = UTRACY_QUEUE_TYPE_PLOTNAME;
						break;
					default:
						DEBUG_DIAGNOSTICS;
						return -1;
				}

				if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				long long unsigned ptr = recv_ptr;
				if(0 != utracy_raw_buf_append(&ptr, sizeof(ptr))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				char *str = (char *) (uintptr_t) (ptr & 0x00000000FFFFFFFFllu);
				short unsigned str_len = (short unsigned) strlen(str);
				if(0 != utracy_raw_buf_append(&str_len, sizeof(str_len))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				if(0 != utracy_raw_buf_append(str, str_len)) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

			}

			break;
		case UTRACY_RECV_QUERY_THREADSTRING:
			{
				char unsigned type = UTRACY_QUEUE_TYPE_THREADNAME;
				if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				long long unsigned ptr = recv_ptr;
				if(0 != utracy_raw_buf_append(&ptr, sizeof(ptr))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				char *str = "main";
				short unsigned str_len = (short unsigned) strlen(str);
				if(0 != utracy_raw_buf_append(&str_len, sizeof(str_len))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				if(0 != utracy_raw_buf_append(str, str_len)) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

			}

			break;
		case UTRACY_RECV_QUERY_SOURCELOCATION:
			{
				struct ___tracy_source_location_data const *const srcloc = (void *) recv_ptr;

				char unsigned type = UTRACY_QUEUE_TYPE_SOURCELOCATION;
				if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				long long unsigned name = ((long long unsigned) ((uintptr_t) (srcloc->name)));
				if(0 != utracy_raw_buf_append(&name, sizeof(name))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				long long unsigned function = ((long long unsigned) ((uintptr_t) srcloc->function));
				if(0 != utracy_raw_buf_append(&function, sizeof(function))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				long long unsigned file = ((long long unsigned) ((uintptr_t) srcloc->file));
				if(0 != utracy_raw_buf_append(&file, sizeof(file))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				int unsigned line = srcloc->line;
				if(0 != utracy_raw_buf_append(&line, sizeof(line))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				char unsigned r = srcloc->color & 0xFF;
				if(0 != utracy_raw_buf_append(&r, sizeof(r))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				char unsigned g = (srcloc->color >> 8) & 0xFF;
				if(0 != utracy_raw_buf_append(&g, sizeof(g))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				char unsigned b = (srcloc->color >> 16) & 0xFF;
				if(0 != utracy_raw_buf_append(&b, sizeof(b))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}
			}

			break;
		/*case UTRACY_RECV_QUERY_PLOTNAME:
			DEBUG_PRINTF_1("UTRACY_RECV_QUERY_PLOTNAME\n");
			break;*/
		case UTRACY_RECV_QUERY_TERMINATE:
			//printf("UTRACY_RECV_QUERY_TERMINATE\n");
			break;
		case UTRACY_RECV_QUERY_CALLSTACKFRAME:
			//printf("UTRACY_RECV_QUERY_CALLSTACKFRAME\n");
			break;
		case UTRACY_RECV_QUERY_FRAMENAME:
			//printf("UTRACY_RECV_QUERY_FRAMENAME\n");
			break;
		case UTRACY_RECV_QUERY_DISCONNECT:
			//printf("UTRACY_RECV_QUERY_DISCONNECT\n");
			break;
		case UTRACY_RECV_QUERY_EXTERNALNAME:
			//printf("UTRACY_RECV_QUERY_EXTERNALNAME\n");
			break;
		case UTRACY_RECV_QUERY_PARAMETER:
			//printf("UTRACY_RECV_QUERY_PARAMETER\n");
			break;
		case UTRACY_RECV_QUERY_SYMBOL:
			//printf("UTRACY_RECV_QUERY_SYMBOL\n");
			break;
		case UTRACY_RECV_QUERY_SYMBOLCODE:
			//printf("UTRACY_RECV_QUERY_SYMBOLCODE\n");

			{
				char unsigned type = UTRACY_QUEUE_TYPE_ACKSYMBOLCODENOTAVAILABLE;
				if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}
			}

			break;
		case UTRACY_RECV_QUERY_CODELOCATION:
			//printf("UTRACY_RECV_QUERY_CODELOCATION\n");
			break;
		case UTRACY_RECV_QUERY_SOURCECODE:
			//printf("UTRACY_RECV_QUERY_SOURCECODE\n");

			{
				char unsigned type = UTRACY_QUEUE_TYPE_ACKSOURCECODENOTAVAILABLE;
				if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}
			}

			break;
		case UTRACY_RECV_QUERY_DATATRANSFER:
			//printf("UTRACY_RECV_QUERY_DATATRANSFER\n");
			//printf("    ptr = %llu; extra = %u\n", recv_ptr, recv_extra);

			{
				char unsigned type = UTRACY_QUEUE_TYPE_ACKSERVERQUERYNOOP;
				if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}
			}

			break;
		case UTRACY_RECV_QUERY_DATATRANSFERPART:
			//printf("UTRACY_RECV_QUERY_DATATRANSFERPART\n");
			//printf("    ptr = %llu; extra = %u\n", recv_ptr, recv_extra);

			{
				char unsigned type = UTRACY_QUEUE_TYPE_ACKSERVERQUERYNOOP;
				if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}
			}

			break;
		case UTRACY_RECV_QUERY_FIBERNAME:
			//printf("UTRACY_RECV_QUERY_FIBERNAME\n");
			break;
		default:
			//printf("UTRACY_RECV_QUERY_UNKNOWN\n");
			break;
	}

	return 0;
}

int utracy_dequeue_event(struct event *evt) {
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
	if(utracy_server_init()) {
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
			return -1;
		}

		struct event evt;
		while(0 == utracy_dequeue_event(&evt)) {
			switch(evt.type) {
				case UTRACY_QUEUE_TYPE_ZONEBEGIN:
					{

						if(evt.tid != utracy.current_tid) {
							utracy.current_tid = evt.tid;
							utracy.thread_time = 0;

							char unsigned type = UTRACY_QUEUE_TYPE_THREADCONTEXT;
							if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
								DEBUG_DIAGNOSTICS;
								return -1;
							}

							int unsigned tid = evt.tid;
							if(0 != utracy_raw_buf_append(&tid, sizeof(tid))) {
								DEBUG_DIAGNOSTICS;
								return -1;
							}
						}

						char unsigned type = evt.type;
						if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}

						long long timestamp = evt.timestamp - utracy.thread_time;
						utracy.thread_time = evt.timestamp;
						if(0 != utracy_raw_buf_append(&timestamp, sizeof(timestamp))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}

						long long unsigned srcloc = (long long unsigned) evt.srcloc;
						if(0 != utracy_raw_buf_append(&srcloc, sizeof(srcloc))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}
					}

					break;

				case UTRACY_QUEUE_TYPE_ZONEEND:
					{

						if(evt.tid != utracy.current_tid) {
							utracy.current_tid = evt.tid;
							utracy.thread_time = 0;

							char unsigned type = UTRACY_QUEUE_TYPE_THREADCONTEXT;
							if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
								DEBUG_DIAGNOSTICS;
								return -1;
							}

							int unsigned tid = evt.tid;
							if(0 != utracy_raw_buf_append(&tid, sizeof(tid))) {
								DEBUG_DIAGNOSTICS;
								return -1;
							}
						}

						char unsigned type = evt.type;
						if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}

						long long timestamp = evt.timestamp - utracy.thread_time;
						utracy.thread_time = evt.timestamp;
						if(0 != utracy_raw_buf_append(&timestamp, sizeof(timestamp))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}
					}

					break;

				case UTRACY_QUEUE_TYPE_ZONECOLOR:
					{

						if(evt.tid != utracy.current_tid) {
							utracy.current_tid = evt.tid;
							utracy.thread_time = 0;

							char unsigned type = UTRACY_QUEUE_TYPE_THREADCONTEXT;
							if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
								DEBUG_DIAGNOSTICS;
								return -1;
							}

							int unsigned tid = evt.tid;
							if(0 != utracy_raw_buf_append(&tid, sizeof(tid))) {
								DEBUG_DIAGNOSTICS;
								return -1;
							}
						}

						char unsigned type = evt.type;
						if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}

						char unsigned r = evt.color & 0xFF;
						if(0 != utracy_raw_buf_append(&r, sizeof(r))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}

						char unsigned g = (evt.color >> 8) & 0xFF;
						if(0 != utracy_raw_buf_append(&g, sizeof(g))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}

						char unsigned b = (evt.color >> 16) & 0xFF;
						if(0 != utracy_raw_buf_append(&b, sizeof(b))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}
					}

					break;

				case UTRACY_QUEUE_TYPE_FRAMEMARKMSG:
					{

						char unsigned type = evt.type;
						if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}

						long long timestamp = evt.timestamp;
						if(0 != utracy_raw_buf_append(&timestamp, sizeof(timestamp))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}

						long long unsigned name = (long long unsigned) evt.framemark.name;
						if(0 != utracy_raw_buf_append(&name, sizeof(name))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}

					}

					break;

				case UTRACY_QUEUE_TYPE_PLOTDATA:
					{

						char unsigned type = evt.type;
						if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}

						long long unsigned name = (long long unsigned) evt.plot.name;
						if(0 != utracy_raw_buf_append(&name, sizeof(name))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}

						long long timestamp = evt.timestamp - utracy.thread_time;
						utracy.thread_time = evt.timestamp;
						if(0 != utracy_raw_buf_append(&timestamp, sizeof(timestamp))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}

						char unsigned plot_type = evt.plot.type;
						if(0 != utracy_raw_buf_append(&plot_type, sizeof(plot_type))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}

						float value = evt.plot.value;
						if(0 != utracy_raw_buf_append(&value, sizeof(value))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}

					}

					break;

				case UTRACY_QUEUE_TYPE_SYSTIMEREPORT:
					{
						char unsigned type = evt.type;
						if(0 != utracy_raw_buf_append(&type, sizeof(type))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}

						//long long timestamp = evt.timestamp - utracy.thread_time;
						//utracy.thread_time = evt.timestamp;
						long long timestamp = evt.timestamp;
						if(0 != utracy_raw_buf_append(&timestamp, sizeof(timestamp))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}

						float system_time = evt.system_time;
						if(0 != utracy_raw_buf_append(&system_time, sizeof(system_time))) {
							DEBUG_DIAGNOSTICS;
							return -1;
						}
					}

					break;

				default:
					DEBUG_DIAGNOSTICS;
					return -1;
			}

			if(utracy.raw_buf_head - utracy.raw_buf_tail >= TARGET_FRAME_SIZE * 0.5) {
				if(0 != utracy_commit()) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}
			}
		}

		WSAPOLLFD descriptor = {
			.fd = utracy.sock.client,
			.events = POLLRDNORM,
			.revents = 0
		};

		while(0 < WSAPoll(&descriptor, 1, 10)) {
			if(0 != utracy_handle_request()) {
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

/* x86 windows fs register points to teb/tib
   thread id is at offset 0x24 */
#if defined(__GNUC__) || defined(__clang__)
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

struct ___tracy_c_zone_context ___tracy_emit_zone_begin(struct ___tracy_source_location_data const *const srcloc, int active) {
	struct event evt = {
		.type = UTRACY_QUEUE_TYPE_ZONEBEGIN,
		.tid = utracy_tid(),
		.timestamp = utracy_tsc(),
		.srcloc = (void *) srcloc
	};

	event_queue_enqueue(&utracy.event_queue, &evt);

	return (struct ___tracy_c_zone_context) {
		.active = active
	};
}

void ___tracy_emit_zone_end(struct ___tracy_c_zone_context ctx) {
	(void) ctx;

	struct event evt = {
		.type = UTRACY_QUEUE_TYPE_ZONEEND,
		.tid = utracy_tid(),
		.timestamp = utracy_tsc()
	};

	event_queue_enqueue(&utracy.event_queue, &evt);
}

void ___tracy_emit_zone_color(struct ___tracy_c_zone_context ctx, int unsigned color) {
	(void) ctx;

	struct event evt = {
		.type = UTRACY_QUEUE_TYPE_ZONECOLOR,
		.tid = utracy_tid(),
		.color = color
	};

	event_queue_enqueue(&utracy.event_queue, &evt);
}

void ___tracy_emit_frame_mark(char const *const name) {
	struct event evt = {
		.type = UTRACY_QUEUE_TYPE_FRAMEMARKMSG,
		.timestamp = utracy_tsc(),
		.framemark = {
			.name = name
		}
	};

	event_queue_enqueue(&utracy.event_queue, &evt);
}

void ___tracy_set_thread_name(char const *const name) {
	(void) name;
}

void ___tracy_emit_plot(char const *const name, float value) {
	event_queue_enqueue(&utracy.event_queue, &(struct event) {
		.type = UTRACY_QUEUE_TYPE_PLOTDATA,
		.timestamp = utracy_tsc(),
		.plot = {
			.name = name,
			.type = 0, /* float */
			.value = value
		}
	});
}
