#include <ws2tcpip.h>
#include <winsock2.h>
#include <windows.h>
#include "server.h"
#include "queue.h"
#include "microtracy.h"
#include <string.h>
#include <stdio.h>

#define DEBUG_DIAGNOSTICS printf("%s %s:%d (wsa=%d)\n", __func__, __FILE__, __LINE__, WSAGetLastError())
#define TARGET_FRAME_SIZE (256 * 1024)

extern int utracy_server_init(void) {
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

	if(0 != bind(utracy.sock.server, addrinfo->ai_addr, addrinfo->ai_addrlen)) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	FreeAddrInfoExW(addrinfo);

	if(0 != listen(utracy.sock.server, 2)) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	return 0;
}

extern int utracy_accept(void) {
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

static int utracy_server_recv(void *const buf, int unsigned len) {
	long unsigned rx;
	long unsigned flags = 0;

	if(SOCKET_ERROR == WSARecv(
		utracy.sock.client,
		(WSABUF[]) {{
			.len = len,
			.buf = buf
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

	if(len != rx) {
		return -1;
	}

	return 0;
}

static int utracy_buf_write(void const *const buf, int unsigned len) {
	if(len > utracy.raw_buf_len - utracy.raw_buf_head) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	memcpy(utracy.raw_buf + utracy.raw_buf_head, buf, len);
	utracy.raw_buf_head += len;

	return 0;
}

static int utracy_write_stringdata(char unsigned type, char const *const str, long long unsigned ptr) {
	short unsigned len = (short unsigned) strlen(str);

	struct network_query_stringdata resp = {
		.type = type,
		.ptr = ptr,
		.len = len
	};

	if(0 != utracy_buf_write(&resp, sizeof(resp))) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	if(0 != utracy_buf_write(str, len)) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	return 0;
}

extern int utracy_consume_request(void) {
	struct network_recv_request req;
	if(0 != utracy_server_recv(&req, sizeof(req))) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	switch(req.type) {
		case UTRACY_RECV_QUERY_STRING:
			if(0 != utracy_write_stringdata(UTRACY_QUEUE_TYPE_STRINGDATA, (char *) (uintptr_t) req.ptr, req.ptr)) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			break;

		case UTRACY_RECV_QUERY_PLOTNAME:
			if(0 != utracy_write_stringdata(UTRACY_QUEUE_TYPE_PLOTNAME, (char *) (uintptr_t) req.ptr, req.ptr)) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			break;

		case UTRACY_RECV_QUERY_THREADSTRING:
			if(0 != utracy_write_stringdata(UTRACY_QUEUE_TYPE_THREADNAME, "main", req.ptr)) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			break;

		case UTRACY_RECV_QUERY_SOURCELOCATION:;
			struct utracy_source_location const *const srcloc = (void *) (uintptr_t) req.ptr;

			if(0 != utracy_buf_write(&(struct network_srcloc) {
				.type = UTRACY_QUEUE_TYPE_SOURCELOCATION,
				.name = (long long unsigned) (uintptr_t) srcloc->name,
				.function = (long long unsigned) (uintptr_t) srcloc->function,
				.file = (long long unsigned) (uintptr_t) srcloc->file,
				.line = srcloc->line,
				.r = (srcloc->color >> 0) & 0xFF,
				.g = (srcloc->color >> 8) & 0xFF,
				.b = (srcloc->color >> 16) & 0xFF
			}, sizeof(struct network_srcloc))) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			break;

		case UTRACY_RECV_QUERY_SYMBOLCODE:;
			char unsigned symbol_type = UTRACY_QUEUE_TYPE_ACKSYMBOLCODENOTAVAILABLE;
			if(0 != utracy_buf_write(&symbol_type, sizeof(symbol_type))) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			break;

		case UTRACY_RECV_QUERY_SOURCECODE:;
			char unsigned sourcecode_type = UTRACY_QUEUE_TYPE_ACKSOURCECODENOTAVAILABLE;
			if(0 != utracy_buf_write(&sourcecode_type, sizeof(sourcecode_type))) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			break;

		case UTRACY_RECV_QUERY_DATATRANSFER:
		case UTRACY_RECV_QUERY_DATATRANSFERPART:;
			char unsigned datatransfer_type = UTRACY_QUEUE_TYPE_ACKSERVERQUERYNOOP;
			if(0 != utracy_buf_write(&datatransfer_type, sizeof(datatransfer_type))) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			break;

		case UTRACY_RECV_QUERY_TERMINATE:
		case UTRACY_RECV_QUERY_CALLSTACKFRAME:
		case UTRACY_RECV_QUERY_FRAMENAME:
		case UTRACY_RECV_QUERY_DISCONNECT:
		case UTRACY_RECV_QUERY_EXTERNALNAME:
		case UTRACY_RECV_QUERY_PARAMETER:
		case UTRACY_RECV_QUERY_SYMBOL:
		case UTRACY_RECV_QUERY_CODELOCATION:
		case UTRACY_RECV_QUERY_FIBERNAME:
		default:
			/* not implemented */
			break;
	}

	return 0;
}

extern int utracy_consume_queue(void) {
	struct event evt;

	/* timestamp relative to thread context -- possibly for better compression? */
	long long timestamp;

	while(0 == utracy_dequeue_event(&evt)) {
		switch(evt.type) {
			case UTRACY_QUEUE_TYPE_ZONEBEGIN:
				if(evt.zone_begin.tid != utracy.current_tid) {
					utracy.current_tid = evt.zone_begin.tid;
					utracy.thread_time = 0llu;

					if(0 != utracy_buf_write(
						&(struct network_thread_context) {
							.type = UTRACY_QUEUE_TYPE_THREADCONTEXT,
							.tid = evt.zone_begin.tid
						},
						sizeof(struct network_thread_context)
					)) {
						DEBUG_DIAGNOSTICS;
						return -1;
					}
				}

				timestamp = evt.zone_begin.timestamp - utracy.thread_time;
				utracy.thread_time = evt.zone_begin.timestamp;

				if(0 != utracy_buf_write(
					&(struct network_zone_begin) {
						.type = UTRACY_QUEUE_TYPE_ZONEBEGIN,
						.timestamp = timestamp,
						.srcloc = (long long unsigned) (uintptr_t) evt.zone_begin.srcloc
					},
					sizeof(struct network_zone_begin)
				)) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				break;

			case UTRACY_QUEUE_TYPE_ZONEEND:
				if(evt.zone_end.tid != utracy.current_tid) {
					utracy.current_tid = evt.zone_end.tid;
					utracy.thread_time = 0llu;

					if(0 != utracy_buf_write(
						&(struct network_thread_context) {
							.type = UTRACY_QUEUE_TYPE_THREADCONTEXT,
							.tid = evt.zone_end.tid
						},
						sizeof(struct network_thread_context)
					)) {
						DEBUG_DIAGNOSTICS;
						return -1;
					}
				}

				timestamp = evt.zone_end.timestamp - utracy.thread_time;
				utracy.thread_time = evt.zone_end.timestamp;

				if(0 != utracy_buf_write(
					&(struct network_zone_end) {
						.type = UTRACY_QUEUE_TYPE_ZONEEND,
						.timestamp = timestamp
					},
					sizeof(struct network_zone_end)
				)) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				break;
			case UTRACY_QUEUE_TYPE_ZONECOLOR:
				if(evt.zone_color.tid != utracy.current_tid) {
					utracy.current_tid = evt.zone_color.tid;
					utracy.thread_time = 0llu;

					if(0 != utracy_buf_write(
						&(struct network_thread_context) {
							.type = UTRACY_QUEUE_TYPE_THREADCONTEXT,
							.tid = evt.zone_color.tid
						},
						sizeof(struct network_thread_context)
					)) {
						DEBUG_DIAGNOSTICS;
						return -1;
					}
				}

				if(0 != utracy_buf_write(
					&(struct network_zone_color) {
						.type = UTRACY_QUEUE_TYPE_ZONECOLOR,
						.r = (evt.zone_color.color >> 0) & 0xFF,
						.g = (evt.zone_color.color >> 8) & 0xFF,
						.b = (evt.zone_color.color >> 16) & 0xFF
					},
					sizeof(struct network_zone_color)
				)) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				break;

			case UTRACY_QUEUE_TYPE_FRAMEMARKMSG:
				if(0 != utracy_buf_write(
					&(struct network_frame_mark) {
						.type = UTRACY_QUEUE_TYPE_FRAMEMARKMSG,
						.timestamp = evt.frame_mark.timestamp,
						.name = (long long unsigned) (uintptr_t) evt.frame_mark.name
					},
					sizeof(struct network_frame_mark)
				)) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				break;

			case UTRACY_QUEUE_TYPE_PLOTDATA:
				if(0 != utracy_buf_write(
					&(struct network_plot) {
						.type = UTRACY_QUEUE_TYPE_PLOTDATA,
						.name = (long long unsigned) (uintptr_t) evt.plot.name,
						.timestamp = evt.plot.timestamp,
						.plot_type = 0, /* float */
						.f = evt.plot.f
					},
					sizeof(struct network_plot)
				)) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				break;

			case UTRACY_QUEUE_TYPE_SYSTIMEREPORT:
				if(0 != utracy_buf_write(
					&(struct network_heartbeat) {
						.type = UTRACY_QUEUE_TYPE_SYSTIMEREPORT,
						.timestamp = evt.system_time.timestamp,
						.system_time = evt.system_time.system_time
					},
					sizeof(struct network_heartbeat)
				)) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				break;

			default:
				DEBUG_DIAGNOSTICS;
				return -1;
		}

		if(utracy.raw_buf_head - utracy.raw_buf_tail >= TARGET_FRAME_SIZE * 2) {
			if(0 != utracy_commit()) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}
		}
	}

	return 0;
}
