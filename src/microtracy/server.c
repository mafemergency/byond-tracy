#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#define SERVER_INTERNAL
#include "server.h"
#include "queue.h"
#include "microtracy.h"
#include <string.h>
#include <stdio.h>
#include <malloc.h>

#define DEBUG_DIAGNOSTICS printf("%s %s:%d (wsa=%d)\n", __func__, __FILE__, __LINE__, WSAGetLastError())

static int unsigned supported_protocols[] = {
	56, /* 0.8.1 */
	57, /* 0.8.2 */
	0 /* sentinel - do not remove */
};

int utracy_server_init(void) {
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
		0
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

int utracy_client_accept(void *_request_shutdown_event) {
	HANDLE request_shutdown_event = _request_shutdown_event;
	(void) request_shutdown_event;

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

	utracy.sock.connected = 1;

	return 0;
}

static int utracy_client_send(void *buf, int unsigned len) {
	DWORD tx;
	DWORD flags = 0;

	if(SOCKET_ERROR == WSASend(
		utracy.sock.client,
		(WSABUF[]) {{
			.len = len,
			.buf = (char *) buf
		}},
		1,
		&tx,
		flags,
		/*&send_ov,*/
		NULL,
		NULL
	)) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	if(len != tx) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	return 0;
}

static int utracy_client_recv(void *const buf, int unsigned len) {
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
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	return 0;
}

static int utracy_commit(void);
static int utracy_write_packet(void const *const buf, int unsigned len) {
	long long unsigned now = GetTickCount64();

	int unsigned current_frame_size = utracy.raw_buf_head - utracy.raw_buf_tail;
	if(current_frame_size + len > UTRACY_MAX_FRAME_SIZE || now - utracy.info.last_pump >= UTRACY_LATENCY) {
		utracy.info.last_pump = now;

		if(0 != utracy_commit()) {
			DEBUG_DIAGNOSTICS;
			return -1;
		}
	}

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
	size_t size = sizeof(struct network_query_stringdata) + len;

#if defined(_MSC_VER) && !defined(__clang__)
	struct network_query_stringdata *resp = _malloca(size);
	if(NULL == resp) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}
#else
	struct network_query_stringdata *resp = _alloca(size);
#endif

	resp->type = type;
	resp->ptr = ptr;
	resp->len = len;
	memcpy(resp->str, str, len);

	if(0 != utracy_write_packet(resp, size)) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	return 0;
}

int utracy_consume_request(void) {
	struct network_recv_request req;
	if(0 != utracy_client_recv(&req, sizeof(req))) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	switch(req.type) {
		case UTRACY_QUERY_STRING:
			if(0 != utracy_write_stringdata(UTRACY_EVT_STRINGDATA, (char *) (uintptr_t) req.ptr, req.ptr)) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			break;

		case UTRACY_QUERY_PLOTNAME:
			if(0 != utracy_write_stringdata(UTRACY_EVT_PLOTNAME, (char *) (uintptr_t) req.ptr, req.ptr)) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			break;

		case UTRACY_QUERY_THREADSTRING:
			if(0 != utracy_write_stringdata(UTRACY_EVT_THREADNAME, "main", req.ptr)) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			break;

		case UTRACY_QUERY_SOURCELOCATION:;
			struct utracy_source_location const *const srcloc = (void *) (uintptr_t) req.ptr;

			if(0 != utracy_write_packet(&(struct network_srcloc) {
				.type = UTRACY_EVT_SOURCELOCATION,
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

		case UTRACY_QUERY_SYMBOLCODE:;
			char unsigned symbol_type = UTRACY_EVT_ACKSYMBOLCODENOTAVAILABLE;
			if(0 != utracy_write_packet(&symbol_type, sizeof(symbol_type))) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			break;

		case UTRACY_QUERY_SOURCECODE:;
			char unsigned sourcecode_type = UTRACY_EVT_ACKSOURCECODENOTAVAILABLE;
			if(0 != utracy_write_packet(&sourcecode_type, sizeof(sourcecode_type))) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			break;

		case UTRACY_QUERY_DATATRANSFER:
		case UTRACY_QUERY_DATATRANSFERPART:;
			char unsigned datatransfer_type = UTRACY_EVT_ACKSERVERQUERYNOOP;
			if(0 != utracy_write_packet(&datatransfer_type, sizeof(datatransfer_type))) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			break;

		case UTRACY_QUERY_TERMINATE:
		case UTRACY_QUERY_CALLSTACKFRAME:
		case UTRACY_QUERY_FRAMENAME:
		case UTRACY_QUERY_DISCONNECT:
		case UTRACY_QUERY_EXTERNALNAME:
		case UTRACY_QUERY_PARAMETER:
		case UTRACY_QUERY_SYMBOL:
		case UTRACY_QUERY_CODELOCATION:
		case UTRACY_QUERY_FIBERNAME:
		default:
			/* not implemented */
			break;
	}

	return 0;
}

static int utracy_commit(void) {
	if(0 < utracy.raw_buf_head - utracy.raw_buf_tail) {
		int unsigned pending_len;
		pending_len = utracy.raw_buf_head - utracy.raw_buf_tail;

		do {
			int unsigned raw_frame_len = min(pending_len, UTRACY_MAX_FRAME_SIZE);

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
				utracy.frame_buf + 0,
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
			if(0 != utracy_client_send(utracy.frame_buf, compressed_len + sizeof(compressed_len))) {
				DEBUG_DIAGNOSTICS;
				return -1;
			}

			/* advance tail */
			utracy.raw_buf_tail += raw_frame_len;
			pending_len = utracy.raw_buf_head - utracy.raw_buf_tail;
		} while(0 < pending_len);

		/* previous 64kb of uncompressed data must remain unclobbered at the
		   same memory address! */
		if(utracy.raw_buf_head >= UTRACY_MAX_FRAME_SIZE * 2) {
			utracy.raw_buf_head = 0;
			utracy.raw_buf_tail = 0;
		}
	}

	return 0;
}

static int utracy_consume_queue(void) {
	struct event evt;

	/* timestamp relative to thread context -- possibly for better compression? */
	long long timestamp;

	while(0 == utracy_dequeue_event(&evt)) {
		switch(evt.type) {
			case UTRACY_EVT_ZONEBEGIN:
				if(evt.zone_begin.tid != utracy.current_tid) {
					utracy.current_tid = evt.zone_begin.tid;
					utracy.thread_time = 0llu;

					if(0 != utracy_write_packet(
						&(struct network_thread_context) {
							.type = UTRACY_EVT_THREADCONTEXT,
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

				if(0 != utracy_write_packet(
					&(struct network_zone_begin) {
						.type = UTRACY_EVT_ZONEBEGIN,
						.timestamp = timestamp,
						.srcloc = (long long unsigned) (uintptr_t) evt.zone_begin.srcloc
					},
					sizeof(struct network_zone_begin)
				)) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				break;

			case UTRACY_EVT_ZONEEND:
				if(evt.zone_end.tid != utracy.current_tid) {
					utracy.current_tid = evt.zone_end.tid;
					utracy.thread_time = 0llu;

					if(0 != utracy_write_packet(
						&(struct network_thread_context) {
							.type = UTRACY_EVT_THREADCONTEXT,
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

				if(0 != utracy_write_packet(
					&(struct network_zone_end) {
						.type = UTRACY_EVT_ZONEEND,
						.timestamp = timestamp
					},
					sizeof(struct network_zone_end)
				)) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				break;
			case UTRACY_EVT_ZONECOLOR:
				if(evt.zone_color.tid != utracy.current_tid) {
					utracy.current_tid = evt.zone_color.tid;
					utracy.thread_time = 0llu;

					if(0 != utracy_write_packet(
						&(struct network_thread_context) {
							.type = UTRACY_EVT_THREADCONTEXT,
							.tid = evt.zone_color.tid
						},
						sizeof(struct network_thread_context)
					)) {
						DEBUG_DIAGNOSTICS;
						return -1;
					}
				}

				if(0 != utracy_write_packet(
					&(struct network_zone_color) {
						.type = UTRACY_EVT_ZONECOLOR,
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

			case UTRACY_EVT_FRAMEMARKMSG:
				if(0 != utracy_write_packet(
					&(struct network_frame_mark) {
						.type = UTRACY_EVT_FRAMEMARKMSG,
						.timestamp = evt.frame_mark.timestamp,
						.name = (long long unsigned) (uintptr_t) evt.frame_mark.name
					},
					sizeof(struct network_frame_mark)
				)) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				break;

			case UTRACY_EVT_PLOTDATA:
				timestamp = evt.zone_begin.timestamp - utracy.thread_time;
				utracy.thread_time = evt.zone_begin.timestamp;

				if(0 != utracy_write_packet(
					&(struct network_plot) {
						.type = UTRACY_EVT_PLOTDATA,
						.name = (long long unsigned) (uintptr_t) evt.plot.name,
						.timestamp = timestamp,
						.plot_type = 0, /* float */
						.f = evt.plot.f
					},
					sizeof(struct network_plot)
				)) {
					DEBUG_DIAGNOSTICS;
					return -1;
				}

				break;

			case UTRACY_EVT_SYSTIMEREPORT:
				if(0 != utracy_write_packet(
					&(struct network_heartbeat) {
						.type = UTRACY_EVT_SYSTIMEREPORT,
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

	}

	return 0;
}

static int utracy_append_heartbeat(void) {
	long long unsigned now = GetTickCount64();

	if(now - utracy.info.last_heartbeat >= 250llu) {
		utracy.info.last_heartbeat = now;

		if(0 != utracy_write_packet(
			&(struct network_heartbeat) {
				.type = UTRACY_EVT_SYSTIMEREPORT,
				.timestamp = utracy_tsc(),
				.system_time = 0.0f
			},
			sizeof(struct network_heartbeat)
		)) {
			DEBUG_DIAGNOSTICS;
			return -1;
		}
	}

	return 0;
}

int utracy_client_negotiate(void) {
	/* client sends shibboleth */
	char shibboleth[UTRACY_HANDSHAKE_SHIBBOLETH_SIZE];
	if(0 != utracy_client_recv(shibboleth, sizeof(shibboleth))) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	if(0 != memcmp(shibboleth, UTRACY_HANDSHAKE_SHIBBOLETH, sizeof(shibboleth))) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	/* client sends protocol version */
	int unsigned protocol_version;
	if(0 != utracy_client_recv(&protocol_version, sizeof(protocol_version))) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	int compatible_protocol = 0;

	for(int unsigned *protocol=supported_protocols; *protocol!=0; protocol++) {
		if(*protocol == protocol_version) {
			compatible_protocol = 1;
			break;
		}
	}

	if(0 == compatible_protocol) {
		DEBUG_DIAGNOSTICS;
		char unsigned response = UTRACY_HANDSHAKE_PROTOCOL_MISMATCH;
		if(0 != utracy_client_send(&response, sizeof(response))) {
			DEBUG_DIAGNOSTICS;
			return -1;
		}
		return -1;
	}

	/* server sends ack */
	char unsigned response = UTRACY_HANDSHAKE_WELCOME;
	if(0 != utracy_client_send(&response, sizeof(response))) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	/* server sends info */
	struct network_welcome welcome = {
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

	memset(welcome.cpuManufacturer, 0, sizeof(welcome.cpuManufacturer));
	memset(welcome.programName, 0, sizeof(welcome.programName));
	memset(welcome.hostInfo, 0, sizeof(welcome.hostInfo));
	memcpy(welcome.cpuManufacturer, "???", 3);
	memcpy(welcome.programName, "dreamdaemon.exe", 15);
	memcpy(welcome.hostInfo, "???", 3);

	if(0 != utracy_client_send(&welcome, sizeof(welcome))) {
		DEBUG_DIAGNOSTICS;
		return -1;
	}

	return 0;
}

int utracy_server_pump(void) {
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

	if(0 < WSAPoll(&descriptor, 1, 1)) {
		if(0 != utracy_consume_request()) {
			DEBUG_DIAGNOSTICS;
			return -1;
		}
	}

	return 0;
}

static int utracy_client_disconnect(void) {
	if(utracy.sock.connected) {
		char unsigned type = UTRACY_QUERY_TERMINATE;
		(void) utracy_write_packet(&type, sizeof(type));
		(void) utracy_commit();

		utracy.sock.connected = 0;

		(void) shutdown(utracy.sock.client, SD_BOTH);
		if(SOCKET_ERROR == closesocket(utracy.sock.client)) {
			return -1;
		}
	}

	return 0;
}

int utracy_server_destroy(void) {
	(void) utracy_client_disconnect();
	(void) shutdown(utracy.sock.server, SD_BOTH);
	(void) closesocket(utracy.sock.server);
	(void) WSACleanup();

	return 0;
}
