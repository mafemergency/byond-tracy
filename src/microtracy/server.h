#ifndef SERVER_H
#define SERVER_H

#include "microtracy.h"
#include <assert.h>

#if defined(__clang__) || defined(__GNUC__)
#	define PACKED __attribute__((packed))
#else
#	define PACKED
#	pragma pack(push, 1)
#endif

struct network_recv_request {
	char unsigned type;
	long long unsigned ptr;
	int unsigned extra;
} PACKED;
_Static_assert(13 == sizeof(struct network_recv_request), "incorrect size");

struct network_query_stringdata {
	char unsigned type;
	long long unsigned ptr;
	short unsigned len;
	char str[];
} PACKED;
_Static_assert(11 == sizeof(struct network_query_stringdata), "incorrect size");

struct network_thread_context {
	char unsigned type;
	int unsigned tid;
} PACKED;
_Static_assert(5 == sizeof(struct network_thread_context), "incorrect size");

struct network_zone_begin {
	char unsigned type;
	long long timestamp;
	long long unsigned srcloc;
} PACKED;
_Static_assert(17 == sizeof(struct network_zone_begin), "incorrect size");

struct network_zone_end {
	char unsigned type;
	long long timestamp;
} PACKED;
_Static_assert(9 == sizeof(struct network_zone_end), "incorrect size");

struct network_zone_color {
	char unsigned type;
	char unsigned r;
	char unsigned g;
	char unsigned b;
} PACKED;
_Static_assert(4 == sizeof(struct network_zone_color), "incorrect size");

struct network_frame_mark {
	char unsigned type;
	long long timestamp;
	long long unsigned name;
} PACKED;
_Static_assert(17 == sizeof(struct network_frame_mark), "incorrect size");

struct network_plot {
	char unsigned type;
	long long unsigned name;
	long long timestamp;
	char unsigned plot_type;
	union {
		float f;
		double d;
		long long i;
	};
} PACKED;
_Static_assert(26 == sizeof(struct network_plot), "incorrect size");

struct network_srcloc {
	char unsigned type;
	long long unsigned name;
	long long unsigned function;
	long long unsigned file;
	int unsigned line;
	char unsigned r;
	char unsigned g;
	char unsigned b;
} PACKED;
_Static_assert(32 == sizeof(struct network_srcloc), "incorrect size");

struct network_heartbeat {
	char unsigned type;
	long long timestamp;
	float system_time;
} PACKED;
_Static_assert(13 == sizeof(struct network_heartbeat), "incorrect size");

#if !defined(__GNUC__) && !defined(__clang__)
#	pragma pack(pop)
#endif

extern int utracy_server_init(void);
extern int utracy_consume_request(void);
extern int utracy_consume_queue(void);
extern int utracy_accept(void);

#endif
