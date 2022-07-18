#ifndef MICROTRACY_H
#define MICROTRACY_H

#include "queue.h"
#include "lz4.h"

/* delay sending data until buffer is full or UTRACY_LATENCY milliseconds
   elapses, whichever comes first */
#define UTRACY_LATENCY (100)

/* maximum uncompressed frame tracy profiler will accept */
#define UTRACY_MAX_FRAME_SIZE (256 * 1024)

#define UTRACY_EVT_ZONETEXT (0)
#define UTRACY_EVT_ZONENAME (1)
#define UTRACY_EVT_MESSAGE (2)
#define UTRACY_EVT_MESSAGECOLOR (3)
#define UTRACY_EVT_MESSAGECALLSTACK (4)
#define UTRACY_EVT_MESSAGECOLORCALLSTACK (5)
#define UTRACY_EVT_MESSAGEAPPINFO (6)
#define UTRACY_EVT_ZONEBEGINALLOCSRCLOC (7)
#define UTRACY_EVT_ZONEBEGINALLOCSRCLOCCALLSTACK (8)
#define UTRACY_EVT_CALLSTACKSERIAL (9)
#define UTRACY_EVT_CALLSTACK (10)
#define UTRACY_EVT_CALLSTACKALLOC (11)
#define UTRACY_EVT_CALLSTACKSAMPLE (12)
#define UTRACY_EVT_CALLSTACKSAMPLECONTEXTSWITCH (13)
#define UTRACY_EVT_FRAMEIMAGE (14)
#define UTRACY_EVT_ZONEBEGIN (15)
#define UTRACY_EVT_ZONEBEGINCALLSTACK (16)
#define UTRACY_EVT_ZONEEND (17)
#define UTRACY_EVT_LOCKWAIT (18)
#define UTRACY_EVT_LOCKOBTAIN (19)
#define UTRACY_EVT_LOCKRELEASE (20)
#define UTRACY_EVT_LOCKSHAREDWAIT (21)
#define UTRACY_EVT_LOCKSHAREDOBTAIN (22)
#define UTRACY_EVT_LOCKSHAREDRELEASE (23)
#define UTRACY_EVT_LOCKNAME (24)
#define UTRACY_EVT_MEMALLOC (25)
#define UTRACY_EVT_MEMALLOCNAMED (26)
#define UTRACY_EVT_MEMFREE (27)
#define UTRACY_EVT_MEMFREENAMED (28)
#define UTRACY_EVT_MEMALLOCCALLSTACK (29)
#define UTRACY_EVT_MEMALLOCCALLSTACKNAMED (30)
#define UTRACY_EVT_MEMFREECALLSTACK (31)
#define UTRACY_EVT_MEMFREECALLSTACKNAMED (32)
#define UTRACY_EVT_GPUZONEBEGIN (33)
#define UTRACY_EVT_GPUZONEBEGINCALLSTACK (34)
#define UTRACY_EVT_GPUZONEBEGINALLOCSRCLOC (35)
#define UTRACY_EVT_GPUZONEBEGINALLOCSRCLOCCALLSTACK (36)
#define UTRACY_EVT_GPUZONEEND (37)
#define UTRACY_EVT_GPUZONEBEGINSERIAL (38)
#define UTRACY_EVT_GPUZONEBEGINCALLSTACKSERIAL (39)
#define UTRACY_EVT_GPUZONEBEGINALLOCSRCLOCSERIAL (40)
#define UTRACY_EVT_GPUZONEBEGINALLOCSRCLOCCALLSTACKSERIAL (41)
#define UTRACY_EVT_GPUZONEENDSERIAL (42)
#define UTRACY_EVT_PLOTDATA (43)
#define UTRACY_EVT_CONTEXTSWITCH (44)
#define UTRACY_EVT_THREADWAKEUP (45)
#define UTRACY_EVT_GPUTIME (46)
#define UTRACY_EVT_GPUCONTEXTNAME (47)
#define UTRACY_EVT_CALLSTACKFRAMESIZE (48)
#define UTRACY_EVT_SYMBOLINFORMATION (49)
#define UTRACY_EVT_CODEINFORMATION (50)
#define UTRACY_EVT_EXTERNALNAMEMETADATA (51)
#define UTRACY_EVT_SYMBOLCODEMETADATA (52)
#define UTRACY_EVT_FIBERENTER (53)
#define UTRACY_EVT_FIBERLEAVE (54)
#define UTRACY_EVT_TERMINATE (55)
#define UTRACY_EVT_KEEPALIVE (56)
#define UTRACY_EVT_THREADCONTEXT (57)
#define UTRACY_EVT_GPUCALIBRATION (58)
#define UTRACY_EVT_CRASH (59)
#define UTRACY_EVT_CRASHREPORT (60)
#define UTRACY_EVT_ZONEVALIDATION (61)
#define UTRACY_EVT_ZONECOLOR (62)
#define UTRACY_EVT_ZONEVALUE (63)
#define UTRACY_EVT_FRAMEMARKMSG (64)
#define UTRACY_EVT_FRAMEMARKMSGSTART (65)
#define UTRACY_EVT_FRAMEMARKMSGEND (66)
#define UTRACY_EVT_SOURCELOCATION (67)
#define UTRACY_EVT_LOCKANNOUNCE (68)
#define UTRACY_EVT_LOCKTERMINATE (69)
#define UTRACY_EVT_LOCKMARK (70)
#define UTRACY_EVT_MESSAGELITERAL (71)
#define UTRACY_EVT_MESSAGELITERALCOLOR (72)
#define UTRACY_EVT_MESSAGELITERALCALLSTACK (73)
#define UTRACY_EVT_MESSAGELITERALCOLORCALLSTACK (74)
#define UTRACY_EVT_GPUNEWCONTEXT (75)
#define UTRACY_EVT_CALLSTACKFRAME (76)
#define UTRACY_EVT_SYSTIMEREPORT (77)
#define UTRACY_EVT_TIDTOPID (78)
#define UTRACY_EVT_HWSAMPLECPUCYCLE (79)
#define UTRACY_EVT_HWSAMPLEINSTRUCTIONRETIRED (80)
#define UTRACY_EVT_HWSAMPLECACHEREFERENCE (81)
#define UTRACY_EVT_HWSAMPLECACHEMISS (82)
#define UTRACY_EVT_HWSAMPLEBRANCHRETIRED (83)
#define UTRACY_EVT_HWSAMPLEBRANCHMISS (84)
#define UTRACY_EVT_PLOTCONFIG (85)
#define UTRACY_EVT_PARAMSETUP (86)
#define UTRACY_EVT_ACKSERVERQUERYNOOP (87)
#define UTRACY_EVT_ACKSOURCECODENOTAVAILABLE (88)
#define UTRACY_EVT_ACKSYMBOLCODENOTAVAILABLE (89)
#define UTRACY_EVT_CPUTOPOLOGY (90)
#define UTRACY_EVT_SINGLESTRINGDATA (91)
#define UTRACY_EVT_SECONDSTRINGDATA (92)
#define UTRACY_EVT_MEMNAMEPAYLOAD (93)
#define UTRACY_EVT_STRINGDATA (94)
#define UTRACY_EVT_THREADNAME (95)
#define UTRACY_EVT_PLOTNAME (96)
#define UTRACY_EVT_SOURCELOCATIONPAYLOAD (97)
#define UTRACY_EVT_CALLSTACKPAYLOAD (98)
#define UTRACY_EVT_CALLSTACKALLOCPAYLOAD (99)
#define UTRACY_EVT_FRAMENAME (100)
#define UTRACY_EVT_FRAMEIMAGEDATA (101)
#define UTRACY_EVT_EXTERNALNAME (102)
#define UTRACY_EVT_EXTERNALTHREADNAME (103)
#define UTRACY_EVT_SYMBOLCODE (104)
#define UTRACY_EVT_SOURCECODE (105)
#define UTRACY_EVT_FIBERNAME (106)

struct utracy {
	struct {
		long long init_begin;
		long long init_end;
		double multiplier;
		long long resolution;
		long long delay;
		long long epoch;
		long long exectime;
		long long unsigned last_heartbeat;
		long long unsigned last_pump;
	} info;

	struct {
		int connected;
		WSADATA wsadata;
		SOCKET server;
		SOCKET client;
	} sock;

	/* 16KB + 4 bytes for the default LZ4_MEMORY_USAGE 14 */
	LZ4_stream_t stream;

	/* lz4 compressed buffer */
	int unsigned buf_len;
	char frame_buf[sizeof(int unsigned) + LZ4_COMPRESSBOUND(UTRACY_MAX_FRAME_SIZE)];
	char *buf;

	int unsigned raw_buf_len;
	int unsigned raw_buf_head;
	int unsigned raw_buf_tail;
	char raw_buf[UTRACY_MAX_FRAME_SIZE * 3];

	struct event_queue event_queue;
	int unsigned current_tid;
	long long thread_time;
};

extern struct utracy utracy;
DWORD WINAPI utracy_thread_start(PVOID user);
long long utracy_tsc(void);
int utracy_dequeue_event(struct event *evt);

/* profiled program api */
struct utracy_source_location {
	char const *name;
	char const *function;
	char const *file;
	int unsigned line;
	int unsigned color;
};

void utracy_emit_zone_begin(struct utracy_source_location const *const srcloc);
void utracy_emit_zone_end(void);
void utracy_emit_zone_color(int unsigned color);
void utracy_emit_frame_mark(char *const name);
void utracy_emit_plot(char *const name, float value);
void utracy_emit_thread_name(char *const name);

#endif
