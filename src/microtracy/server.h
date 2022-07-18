#ifndef SERVER_H
#define SERVER_H

extern int utracy_server_init(void);
extern int utracy_server_destroy(void);
extern int utracy_client_accept(void *);
extern int utracy_client_negotiate(void);
extern int utracy_server_pump(void);

#ifdef SERVER_INTERNAL

#define UTRACY_HANDSHAKE_SHIBBOLETH_SIZE (8)
#define UTRACY_HANDSHAKE_SHIBBOLETH ("TracyPrf")
#define UTRACY_HANDSHAKE_PENDING (0)
#define UTRACY_HANDSHAKE_WELCOME (1)
#define UTRACY_HANDSHAKE_PROTOCOL_MISMATCH (2)
#define UTRACY_HANDSHAKE_NOT_AVAILABLE (3)
#define UTRACY_HANDSHAKE_DROPPED (4)

#define UTRACY_QUERY_TERMINATE (0)
#define UTRACY_QUERY_STRING (1)
#define UTRACY_QUERY_THREADSTRING (2)
#define UTRACY_QUERY_SOURCELOCATION (3)
#define UTRACY_QUERY_PLOTNAME (4)
#define UTRACY_QUERY_FRAMENAME (5)
#define UTRACY_QUERY_PARAMETER (6)
#define UTRACY_QUERY_FIBERNAME (7)
#define UTRACY_QUERY_DISCONNECT (8)
#define UTRACY_QUERY_CALLSTACKFRAME (9)
#define UTRACY_QUERY_EXTERNALNAME (10)
#define UTRACY_QUERY_SYMBOL (11)
#define UTRACY_QUERY_SYMBOLCODE (12)
#define UTRACY_QUERY_CODELOCATION (13)
#define UTRACY_QUERY_SOURCECODE (14)
#define UTRACY_QUERY_DATATRANSFER (15)
#define UTRACY_QUERY_DATATRANSFERPART (16)

#include <assert.h>

#if defined(__clang__) || defined(__GNUC__)
#	define PACKED __attribute__((packed))
#else
#	define PACKED
#	pragma pack(push, 1)
#endif

struct network_welcome {
	double timerMul;
	long long initBegin;
	long long initEnd;
	long long delay;
	long long resolution;
	long long epoch;
	long long exectime;
	long long pid;
	long long samplingPeriod;
	char unsigned flags;
	char unsigned cpuArch;
	char cpuManufacturer[12];
	int unsigned cpuId;
	char programName[64];
	char hostInfo[1024];
} PACKED;
_Static_assert(1178 == sizeof(struct network_welcome), "incorrect size");

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

#endif
#endif
