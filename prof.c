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
#define UTRACY_LATENCY (100000000ll)
#define UTRACY_MAX_FRAME_SIZE (256u * 1024u)

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

/*
 *  LZ4 - Fast LZ compression algorithm
 *  Header File
 *  Copyright (C) 2011-2020, Yann Collet.
   BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
   You can contact the author at :
    - LZ4 homepage : http://www.lz4.org
    - LZ4 source repository : https://github.com/lz4/lz4
*/

#define LZ4_MEMORY_USAGE_MIN 10
//#define LZ4_MEMORY_USAGE_DEFAULT 14
#define LZ4_MEMORY_USAGE_DEFAULT 20
#define LZ4_MEMORY_USAGE_MAX 20

#if !defined(LZ4_MEMORY_USAGE)
#	define LZ4_MEMORY_USAGE LZ4_MEMORY_USAGE_DEFAULT
#endif

#if (LZ4_MEMORY_USAGE < LZ4_MEMORY_USAGE_MIN)
#	error "LZ4_MEMORY_USAGE is too small !"
#endif

#if (LZ4_MEMORY_USAGE > LZ4_MEMORY_USAGE_MAX)
#	error "LZ4_MEMORY_USAGE is too large !"
#endif

#define LZ4_MAX_INPUT_SIZE 0x7E000000
#define LZ4_COMPRESSBOUND(isize) ((unsigned) (isize) > (unsigned) LZ4_MAX_INPUT_SIZE ? 0 : (isize) + ((isize) / 255) + 16)
#define LZ4_ACCELERATION_DEFAULT 1
#define LZ4_ACCELERATION_MAX 65537

#if !defined(LZ4_DISTANCE_MAX)
#	define LZ4_DISTANCE_MAX 65535
#endif
#define LZ4_DISTANCE_ABSOLUTE_MAX 65535
#if (LZ4_DISTANCE_MAX > LZ4_DISTANCE_ABSOLUTE_MAX)
#	error "LZ4_DISTANCE_MAX is too big : must be <= 65535"
#endif

#define ML_BITS 4u
#define ML_MASK  ((1U << ML_BITS) - 1u)
#define RUN_BITS (8u - ML_BITS)
#define RUN_MASK ((1U << RUN_BITS) - 1u)

#define MINMATCH 4
#define WILDCOPYLENGTH 8
#define LASTLITERALS 5
#define MFLIMIT 12
#define MATCH_SAFEGUARD_DISTANCE ((2 * WILDCOPYLENGTH) - MINMATCH)
#define FASTLOOP_SAFE_DISTANCE 64
#define LZ4_minLength (MFLIMIT + 1)

#define LZ4_64Klimit (65536 + (MFLIMIT - 1))
#define LZ4_skipTrigger ((LZ4_u32) 6u)

#define LZ4_HASHLOG (LZ4_MEMORY_USAGE - 2)
#define LZ4_HASHTABLESIZE (1 << LZ4_MEMORY_USAGE)
#define LZ4_HASH_SIZE_U32 (1 << LZ4_HASHLOG)

#define LZ4_STREAMSIZE ((1ul << LZ4_MEMORY_USAGE) + 32u)
#define LZ4_STREAMSIZE_VOIDP (LZ4_STREAMSIZE / sizeof(void *))

#define LZ4_STATIC_ASSERT(a) _Static_assert((a), "")

typedef char signed LZ4_i8;
typedef char unsigned LZ4_byte;
typedef short unsigned LZ4_u16;
typedef int unsigned LZ4_u32;
typedef long long unsigned LZ4_u64;
typedef union LZ4_stream_u LZ4_stream_t;
typedef struct LZ4_stream_t_internal LZ4_stream_t_internal;
typedef size_t reg_t;

typedef enum {
	LZ4_clearedTable = 0,
	LZ4_byPtr,
	LZ4_byU32,
	LZ4_byU16
} LZ4_tableType_t;

typedef enum {
	LZ4_notLimited = 0,
	LZ4_limitedOutput = 1,
	LZ4_fillOutput = 2
} LZ4_limitedOutput_directive;

typedef enum {
	LZ4_noDict = 0,
	LZ4_withPrefix64k,
	LZ4_usingExtDict,
	LZ4_usingDictCtx
} LZ4_dict_directive;

typedef enum {
	LZ4_noDictIssue = 0,
	LZ4_dictSmall
} LZ4_dictIssue_directive;

struct LZ4_stream_t_internal {
	LZ4_u32 hashTable[LZ4_HASH_SIZE_U32];
	LZ4_u32 currentOffset;
	LZ4_u32 tableType;
	LZ4_byte const *dictionary;
	LZ4_stream_t_internal const *dictCtx;
	LZ4_u32 dictSize;
};

union LZ4_stream_u {
	void *table[LZ4_STREAMSIZE_VOIDP];
	LZ4_stream_t_internal internal_donotuse;
};

#pragma pack(push, 1)
typedef union {
	LZ4_u16 u16;
	LZ4_u32 u32;
	reg_t uArch;
} LZ4_unalign;
#pragma pack(pop)

UTRACY_INTERNAL UTRACY_INLINE
LZ4_u16 LZ4_read16(void const *ptr) {
	return ((LZ4_unalign const *) ptr)->u16;
}

UTRACY_INTERNAL UTRACY_INLINE
LZ4_u32 LZ4_read32(void const *ptr) {
	return ((LZ4_unalign const *) ptr)->u32;
}

UTRACY_INTERNAL UTRACY_INLINE
reg_t LZ4_read_ARCH(void const *ptr) {
	return ((LZ4_unalign const *) ptr)->uArch;
}

UTRACY_INTERNAL UTRACY_INLINE
void LZ4_write16(void *memPtr, LZ4_u16 value) {
	((LZ4_unalign *) memPtr)->u16 = value;
}

UTRACY_INTERNAL UTRACY_INLINE
void LZ4_write32(void *memPtr, LZ4_u32 value) {
	((LZ4_unalign *) memPtr)->u32 = value;
}

#define LZ4_writeLE16 LZ4_write16

UTRACY_INTERNAL UTRACY_INLINE
void LZ4_wildCopy8(void *dstPtr, const void *srcPtr, void *dstEnd) {
	LZ4_byte *d = (LZ4_byte *) dstPtr;
	LZ4_byte const *s = (LZ4_byte const *) srcPtr;
	LZ4_byte *const e = (LZ4_byte *) dstEnd;

	do {
		(void) UTRACY_MEMCPY(d, s, 8);
		d += 8;
		s += 8;
	} while (d < e);
}

UTRACY_INTERNAL UTRACY_INLINE
unsigned LZ4_NbCommonBytes(reg_t val) {
	assert(val != 0);

#if defined(UTRACY_MSVC)
	long unsigned r;
	_BitScanForward(&r, (LZ4_u32) val);
	return (unsigned) r >> 3u;

#elif __has_builtin(__builtin_ctx)
	return (unsigned) __builtin_ctz((LZ4_u32) val) >> 3u;

#else
	LZ4_u32 const m = 0x01010101u;
	return (unsigned) ((((val - 1) ^ val) & (m - 1)) * m) >> 24u;

#endif
}

UTRACY_INTERNAL UTRACY_INLINE
unsigned LZ4_count(LZ4_byte const *pIn, LZ4_byte const *pMatch, LZ4_byte const *pInLimit) {
	LZ4_byte const *const pStart = pIn;

	if(likely(pIn < pInLimit - (sizeof(reg_t) - 1))) {
		reg_t const diff = LZ4_read_ARCH(pMatch) ^ LZ4_read_ARCH(pIn);
		if(!diff) {
			pIn += sizeof(reg_t);
			pMatch += sizeof(reg_t);
		} else {
			return LZ4_NbCommonBytes(diff);
		}
	}

	while(likely(pIn < pInLimit - (sizeof(reg_t) - 1))) {
		reg_t const diff = LZ4_read_ARCH(pMatch) ^ LZ4_read_ARCH(pIn);
		if(!diff) {
			pIn += sizeof(reg_t);
			pMatch += sizeof(reg_t);
			continue;
		}
		pIn += LZ4_NbCommonBytes(diff);
		return (unsigned) (pIn - pStart);
	}

	if((sizeof(reg_t) == 8) && (pIn < (pInLimit - 3)) && (LZ4_read32(pMatch) == LZ4_read32(pIn))) {
		pIn += 4;
		pMatch += 4;
	}

	if((pIn < (pInLimit - 1)) && (LZ4_read16(pMatch) == LZ4_read16(pIn))) {
		pIn += 2;
		pMatch += 2;
	}

	if((pIn < pInLimit) && (*pMatch == *pIn)) {
		pIn++;
	}

	return (unsigned)(pIn - pStart);
}


UTRACY_INTERNAL UTRACY_INLINE
LZ4_u32 LZ4_hash4(LZ4_u32 sequence, LZ4_tableType_t const tableType) {
	if(tableType == LZ4_byU16) {
		return ((sequence * 2654435761u) >> ((MINMATCH * 8) - (LZ4_HASHLOG + 1)));
	} else {
		return ((sequence * 2654435761u) >> ((MINMATCH * 8) - LZ4_HASHLOG));
	}
}

UTRACY_INTERNAL UTRACY_INLINE
LZ4_u32 LZ4_hash5(LZ4_u64 sequence, LZ4_tableType_t const tableType) {
	LZ4_u32 const hashLog = (tableType == LZ4_byU16) ? LZ4_HASHLOG + 1 : LZ4_HASHLOG;
	LZ4_u64 const prime5bytes = 889523592379llu;
	return (LZ4_u32) (((sequence << 24) * prime5bytes) >> (64 - hashLog));
}

UTRACY_INTERNAL UTRACY_INLINE
LZ4_u32 LZ4_hashPosition(void const *const p, LZ4_tableType_t const tableType) {
	if((sizeof(reg_t) == 8) && (tableType != LZ4_byU16)) return LZ4_hash5(LZ4_read_ARCH(p), tableType);
	return LZ4_hash4(LZ4_read32(p), tableType);
}

UTRACY_INTERNAL UTRACY_INLINE
void LZ4_clearHash(LZ4_u32 h, void *tableBase, LZ4_tableType_t const tableType) {
	switch(tableType) {
		default:
		case LZ4_clearedTable: {assert(0); return;}
		case LZ4_byPtr: {LZ4_byte const **hashTable = (LZ4_byte const **) tableBase; hashTable[h] = NULL; return;}
		case LZ4_byU32: {LZ4_u32 *hashTable = (LZ4_u32 *) tableBase; hashTable[h] = 0; return;}
		case LZ4_byU16: {LZ4_u16 *hashTable = (LZ4_u16 *) tableBase; hashTable[h] = 0; return;}
	}
}

UTRACY_INTERNAL UTRACY_INLINE
void LZ4_putIndexOnHash(LZ4_u32 idx, LZ4_u32 h, void *tableBase, LZ4_tableType_t const tableType) {
	switch(tableType) {
		default:
		case LZ4_clearedTable:
		case LZ4_byPtr: {assert(0); return;}
		case LZ4_byU32: {LZ4_u32 *hashTable = (LZ4_u32 *) tableBase; hashTable[h] = idx; return;}
		case LZ4_byU16: {LZ4_u16 *hashTable = (LZ4_u16 *) tableBase; assert(idx < 65536u); hashTable[h] = (LZ4_u16) idx; return;}
	}
}

UTRACY_INTERNAL UTRACY_INLINE
void LZ4_putPositionOnHash(LZ4_byte const *p, LZ4_u32 h, void *tableBase, LZ4_tableType_t const tableType, LZ4_byte const *srcBase) {
	switch(tableType) {
		case LZ4_clearedTable: {assert(0); return;}
		case LZ4_byPtr: {LZ4_byte const **hashTable = (LZ4_byte const **) tableBase; hashTable[h] = p; return;}
		case LZ4_byU32: {LZ4_u32 *hashTable = (LZ4_u32 *) tableBase; hashTable[h] = (LZ4_u32) (p - srcBase); return;}
		case LZ4_byU16: {LZ4_u16 *hashTable = (LZ4_u16 *) tableBase; hashTable[h] = (LZ4_u16) (p - srcBase); return;}
	}
}

UTRACY_INTERNAL UTRACY_INLINE
void LZ4_putPosition(LZ4_byte const *p, void *tableBase, LZ4_tableType_t tableType, LZ4_byte const *srcBase) {
	LZ4_u32 const h = LZ4_hashPosition(p, tableType);
	LZ4_putPositionOnHash(p, h, tableBase, tableType, srcBase);
}

UTRACY_INTERNAL UTRACY_INLINE
LZ4_u32 LZ4_getIndexOnHash(LZ4_u32 h, void const *tableBase, LZ4_tableType_t tableType) {
	LZ4_STATIC_ASSERT(LZ4_MEMORY_USAGE > 2);
	if(tableType == LZ4_byU32) {
		const LZ4_u32 *const hashTable = (const LZ4_u32 *) tableBase;
		assert(h < (1U << (LZ4_MEMORY_USAGE - 2)));
		return hashTable[h];
	}
	if(tableType == LZ4_byU16) {
		const LZ4_u16 *const hashTable = (const LZ4_u16 *) tableBase;
		assert(h < (1U << (LZ4_MEMORY_USAGE - 1)));
		return hashTable[h];
	}
	assert(0);
	return 0;
}

UTRACY_INTERNAL UTRACY_INLINE
LZ4_byte const *LZ4_getPositionOnHash(LZ4_u32 h, void const *tableBase, LZ4_tableType_t tableType, LZ4_byte const *srcBase) {
	if(tableType == LZ4_byPtr) {LZ4_byte const *const *hashTable = (LZ4_byte const *const *) tableBase; return hashTable[h];}
	if(tableType == LZ4_byU32) {LZ4_u32 const *const hashTable = (LZ4_u32 const *) tableBase; return hashTable[h] + srcBase;}
	{
		LZ4_u16 const *const hashTable = (LZ4_u16 const *) tableBase;
		return hashTable[h] + srcBase;
	}
}

UTRACY_INTERNAL UTRACY_INLINE
LZ4_byte const *LZ4_getPosition(LZ4_byte const *p, void const *tableBase, LZ4_tableType_t tableType, LZ4_byte const *srcBase) {
	LZ4_u32 const h = LZ4_hashPosition(p, tableType);
	return LZ4_getPositionOnHash(h, tableBase, tableType, srcBase);
}

UTRACY_INTERNAL UTRACY_INLINE
void LZ4_prepareTable(LZ4_stream_t_internal *const cctx, int const inputSize, LZ4_tableType_t const tableType) {
	if((LZ4_tableType_t) cctx->tableType != LZ4_clearedTable) {
		assert(inputSize >= 0);

		if(
			(LZ4_tableType_t) cctx->tableType != tableType ||
			((tableType == LZ4_byU16) && cctx->currentOffset + (unsigned) inputSize >= 0xFFFFU) ||
			((tableType == LZ4_byU32) && cctx->currentOffset > 1073741824u) ||
			tableType == LZ4_byPtr ||
			inputSize >= 4096
		) {
			(void) UTRACY_MEMSET(cctx->hashTable, 0, LZ4_HASHTABLESIZE);
			cctx->currentOffset = 0;
			cctx->tableType = (LZ4_u32) LZ4_clearedTable;
		}
	}

	if(cctx->currentOffset != 0 && tableType == LZ4_byU32) {
		cctx->currentOffset += 65536u;
	}

	cctx->dictCtx = NULL;
	cctx->dictionary = NULL;
	cctx->dictSize = 0;
}

UTRACY_INTERNAL UTRACY_INLINE
void LZ4_renormDictT(LZ4_stream_t_internal *LZ4_dict, int nextSize) {
	assert(nextSize >= 0);
	if(LZ4_dict->currentOffset + (unsigned) nextSize > 0x80000000u) {
		LZ4_u32 const delta = LZ4_dict->currentOffset - 65536u;
		LZ4_byte const *dictEnd = LZ4_dict->dictionary + LZ4_dict->dictSize;
		for(int i=0; i<LZ4_HASH_SIZE_U32; i++) {
			if(LZ4_dict->hashTable[i] < delta) LZ4_dict->hashTable[i] = 0;
			else LZ4_dict->hashTable[i] -= delta;
		}

		LZ4_dict->currentOffset = 65536u;
		if(LZ4_dict->dictSize > 65536u) LZ4_dict->dictSize = 65536u;
		LZ4_dict->dictionary = dictEnd - LZ4_dict->dictSize;
	}
}

UTRACY_INTERNAL UTRACY_INLINE
int LZ4_compress_generic_validated(LZ4_stream_t_internal *const cctx, char const *const source, char *const dest, int const inputSize, int *inputConsumed, int const maxOutputSize, LZ4_limitedOutput_directive const outputDirective, LZ4_tableType_t const tableType, LZ4_dict_directive const dictDirective, LZ4_dictIssue_directive const dictIssue, int const acceleration) {
	int result;
	LZ4_byte const *ip = (LZ4_byte const *) source;

	LZ4_u32 const startIndex = cctx->currentOffset;
	LZ4_byte const *base = (LZ4_byte const *) source - startIndex;
	LZ4_byte const *lowLimit;

	LZ4_stream_t_internal const *dictCtx = (LZ4_stream_t_internal const *) cctx->dictCtx;
	LZ4_byte const *const dictionary = dictDirective == LZ4_usingDictCtx ? dictCtx->dictionary : cctx->dictionary;
	LZ4_u32 const dictSize = dictDirective == LZ4_usingDictCtx ? dictCtx->dictSize : cctx->dictSize;
	LZ4_u32 const dictDelta = (dictDirective == LZ4_usingDictCtx) ? startIndex - dictCtx->currentOffset : 0;

	int const maybe_extMem = (dictDirective == LZ4_usingExtDict) || (dictDirective == LZ4_usingDictCtx);
	LZ4_u32 const prefixIdxLimit = startIndex - dictSize;
	LZ4_byte const *const dictEnd = dictionary ? dictionary + dictSize : dictionary;
	LZ4_byte const *anchor = (LZ4_byte const *) source;
	LZ4_byte const *const iend = ip + inputSize;
	LZ4_byte const *const mflimitPlusOne = iend - MFLIMIT + 1;
	LZ4_byte const *const matchlimit = iend - LASTLITERALS;

	LZ4_byte const *dictBase = (dictionary == NULL) ? NULL : (dictDirective == LZ4_usingDictCtx) ? dictionary + dictSize - dictCtx->currentOffset : dictionary + dictSize - startIndex;

	LZ4_byte *op = (LZ4_byte *) dest;
	LZ4_byte *const olimit = op + maxOutputSize;

	LZ4_u32 offset = 0;
	LZ4_u32 forwardH;

	assert(ip != NULL);
	if(outputDirective == LZ4_fillOutput && maxOutputSize < 1) {
		return 0;
	}
	if((tableType == LZ4_byU16) && (inputSize >= LZ4_64Klimit)) {
		return 0;
	}
	if(tableType == LZ4_byPtr) assert(dictDirective == LZ4_noDict);
	assert(acceleration >= 1);

	lowLimit = (LZ4_byte const *) source - (dictDirective == LZ4_withPrefix64k ? dictSize : 0);

	if(dictDirective == LZ4_usingDictCtx) {
		cctx->dictCtx = NULL;
		cctx->dictSize = (LZ4_u32) inputSize;
	} else {
		cctx->dictSize += (LZ4_u32) inputSize;
	}

	cctx->currentOffset += (LZ4_u32) inputSize;
	cctx->tableType = (LZ4_u32) tableType;

	if(inputSize < LZ4_minLength) goto _last_literals;

	LZ4_putPosition(ip, cctx->hashTable, tableType, base);
	ip++;
	forwardH = LZ4_hashPosition(ip, tableType);

	for(;;) {
		LZ4_byte const *match;
		LZ4_byte *token;
		LZ4_byte const *filledIp;

		if(tableType == LZ4_byPtr) {
			LZ4_byte const *forwardIp = ip;
			int step = 1;
			int searchMatchNb = acceleration << LZ4_skipTrigger;
			do {
				LZ4_u32 const h = forwardH;
				ip = forwardIp;
				forwardIp += step;
				step = (searchMatchNb++ >> LZ4_skipTrigger);

				if(unlikely(forwardIp > mflimitPlusOne)) goto _last_literals;
				assert(ip < mflimitPlusOne);

				match = LZ4_getPositionOnHash(h, cctx->hashTable, tableType, base);
				forwardH = LZ4_hashPosition(forwardIp, tableType);
				LZ4_putPositionOnHash(ip, h, cctx->hashTable, tableType, base);

			} while((match + LZ4_DISTANCE_MAX < ip) || (LZ4_read32(match) != LZ4_read32(ip)));

		} else {
			LZ4_byte const *forwardIp = ip;
			int step = 1;
			int searchMatchNb = acceleration << LZ4_skipTrigger;
			do {
				LZ4_u32 const h = forwardH;
				LZ4_u32 const current = (LZ4_u32) (forwardIp - base);
				LZ4_u32 matchIndex = LZ4_getIndexOnHash(h, cctx->hashTable, tableType);
				assert(matchIndex <= current);
				assert(forwardIp - base < (ptrdiff_t) (2147483648u - 1u));
				ip = forwardIp;
				forwardIp += step;
				step = (searchMatchNb++ >> LZ4_skipTrigger);

				if(unlikely(forwardIp > mflimitPlusOne)) goto _last_literals;
				assert(ip < mflimitPlusOne);

				if(dictDirective == LZ4_usingDictCtx) {
					if(matchIndex < startIndex) {
						assert(tableType == LZ4_byU32);
						matchIndex = LZ4_getIndexOnHash(h, dictCtx->hashTable, LZ4_byU32);
						match = dictBase + matchIndex;
						matchIndex += dictDelta;
						lowLimit = dictionary;
					} else {
						match = base + matchIndex;
						lowLimit = (LZ4_byte const *) source;
					}
				} else if(dictDirective == LZ4_usingExtDict) {
					if(matchIndex < startIndex) {
						assert(startIndex - matchIndex >= MINMATCH);
						assert(dictBase);
						match = dictBase + matchIndex;
						lowLimit = dictionary;
					} else {
						match = base + matchIndex;
						lowLimit = (LZ4_byte const *)source;
					}
				} else {
					match = base + matchIndex;
				}

				forwardH = LZ4_hashPosition(forwardIp, tableType);
				LZ4_putIndexOnHash(current, h, cctx->hashTable, tableType);

				if((dictIssue == LZ4_dictSmall) && (matchIndex < prefixIdxLimit)) {
					continue;
				}

				assert(matchIndex < current);

				if(((tableType != LZ4_byU16) || (LZ4_DISTANCE_MAX < LZ4_DISTANCE_ABSOLUTE_MAX)) && (matchIndex + LZ4_DISTANCE_MAX < current)) {
					continue;
				}

				assert((current - matchIndex) <= LZ4_DISTANCE_MAX);

				if(LZ4_read32(match) == LZ4_read32(ip)) {
					if(maybe_extMem) offset = current - matchIndex;
					break;
				}

			} while(1);
		}

		filledIp = ip;
		while(((ip > anchor) & (match > lowLimit)) && (unlikely(ip[-1] == match[-1]))) {
			ip--;
			match--;
		}

		{
			unsigned const litLength = (unsigned) (ip - anchor);
			token = op++;
			if((outputDirective == LZ4_limitedOutput) && (unlikely(op + litLength + (2 + 1 + LASTLITERALS) + (litLength / 255) > olimit))) {
				return 0;
			}

			if((outputDirective == LZ4_fillOutput) && (unlikely(op + (litLength + 240) / 255 + litLength + 2 + 1 + MFLIMIT - MINMATCH > olimit))) {
				op--;
				goto _last_literals;
			}

			if(litLength >= RUN_MASK) {
				int len = (int) (litLength - RUN_MASK);
				*token = (RUN_MASK << ML_BITS);
				for(; len >= 255; len -= 255) *op++ = 255;
				*op++ = (LZ4_byte) len;
			} else {
				*token = (LZ4_byte) (litLength << ML_BITS);
			}

			LZ4_wildCopy8(op, anchor, op + litLength);
			op += litLength;
		}

_next_match:
		if((outputDirective == LZ4_fillOutput) && (op + 2 + 1 + MFLIMIT - MINMATCH > olimit)) {
			op = token;
			goto _last_literals;
		}

		if(maybe_extMem) {
			assert(offset <= LZ4_DISTANCE_MAX && offset > 0);
			LZ4_writeLE16(op, (LZ4_u16) offset);
			op += 2;
		} else  {
			assert(ip - match <= LZ4_DISTANCE_MAX);
			LZ4_writeLE16(op, (LZ4_u16) (ip - match));
			op += 2;
		}

		{
			unsigned matchCode;

			if((dictDirective == LZ4_usingExtDict || dictDirective == LZ4_usingDictCtx) && (lowLimit == dictionary)) {
				LZ4_byte const *limit = ip + (dictEnd - match);
				assert(dictEnd > match);
				if(limit > matchlimit) limit = matchlimit;
				matchCode = LZ4_count(ip + MINMATCH, match + MINMATCH, limit);
				ip += (size_t) matchCode + MINMATCH;
				if(ip == limit) {
					unsigned const more = LZ4_count(limit, (LZ4_byte const *) source, matchlimit);
					matchCode += more;
					ip += more;
				}
			} else {
				matchCode = LZ4_count(ip + MINMATCH, match + MINMATCH, matchlimit);
				ip += (size_t) matchCode + MINMATCH;
			}

			if((outputDirective) && (unlikely(op + (1 + LASTLITERALS) + (matchCode + 240) / 255 > olimit))) {
				if(outputDirective == LZ4_fillOutput) {
					LZ4_u32 newMatchCode = 15 - 1 + ((LZ4_u32) (olimit - op) - 1 - LASTLITERALS) * 255;
					ip -= matchCode - newMatchCode;
					assert(newMatchCode < matchCode);
					matchCode = newMatchCode;
					if(unlikely(ip <= filledIp)) {
						LZ4_byte const *ptr;
						for(ptr = ip; ptr <= filledIp; ++ptr) {
							LZ4_u32 const h = LZ4_hashPosition(ptr, tableType);
							LZ4_clearHash(h, cctx->hashTable, tableType);
						}
					}
				} else {
					assert(outputDirective == LZ4_limitedOutput);
					return 0;
				}
			}

			if(matchCode >= ML_MASK) {
				*token += ML_MASK;
				matchCode -= ML_MASK;
				LZ4_write32(op, 0xFFFFFFFFu);
				while(matchCode >= 4 * 255) {
					op += 4;
					LZ4_write32(op, 0xFFFFFFFFu);
					matchCode -= 4 * 255;
				}
				op += matchCode / 255;
				*op++ = (LZ4_byte) (matchCode % 255);
			} else {
				*token += (LZ4_byte)(matchCode);
			}
		}

		assert(!(outputDirective == LZ4_fillOutput && op + 1 + LASTLITERALS > olimit));

		anchor = ip;

		if(ip >= mflimitPlusOne) {
			break;
		}

		LZ4_putPosition(ip - 2, cctx->hashTable, tableType, base);

		if(tableType == LZ4_byPtr) {
			match = LZ4_getPosition(ip, cctx->hashTable, tableType, base);
			LZ4_putPosition(ip, cctx->hashTable, tableType, base);
			if((match + LZ4_DISTANCE_MAX >= ip) && (LZ4_read32(match) == LZ4_read32(ip))) {
				token = op++;
				*token = 0;
				goto _next_match;
			}

		} else {
			LZ4_u32 const h = LZ4_hashPosition(ip, tableType);
			LZ4_u32 const current = (LZ4_u32) (ip-base);
			LZ4_u32 matchIndex = LZ4_getIndexOnHash(h, cctx->hashTable, tableType);
			assert(matchIndex < current);
			if(dictDirective == LZ4_usingDictCtx) {
				if(matchIndex < startIndex) {
					matchIndex = LZ4_getIndexOnHash(h, dictCtx->hashTable, LZ4_byU32);
					match = dictBase + matchIndex;
					lowLimit = dictionary;
					matchIndex += dictDelta;
				} else {
					match = base + matchIndex;
					lowLimit = (LZ4_byte const *) source;
				}
			} else if(dictDirective == LZ4_usingExtDict) {
				if(matchIndex < startIndex) {
					assert(dictBase);
					match = dictBase + matchIndex;
					lowLimit = dictionary;
				} else {
					match = base + matchIndex;
					lowLimit = (LZ4_byte const *) source;
				}
			} else {
				match = base + matchIndex;
			}

			LZ4_putIndexOnHash(current, h, cctx->hashTable, tableType);
			assert(matchIndex < current);
			if(((dictIssue == LZ4_dictSmall) ? (matchIndex >= prefixIdxLimit) : 1) && (((tableType == LZ4_byU16) && (LZ4_DISTANCE_MAX == LZ4_DISTANCE_ABSOLUTE_MAX)) ? 1 : (matchIndex + LZ4_DISTANCE_MAX >= current)) && (LZ4_read32(match) == LZ4_read32(ip))) {
				token = op++;
				*token = 0;
				if(maybe_extMem) offset = current - matchIndex;
				goto _next_match;
			}
		}

		forwardH = LZ4_hashPosition(++ip, tableType);
	}

_last_literals:
	{
		size_t lastRun = (size_t) (iend - anchor);
		if((outputDirective) && (op + lastRun + 1 + ((lastRun + 255 - RUN_MASK) / 255) > olimit)) {
			if(outputDirective == LZ4_fillOutput) {
				assert(olimit >= op);
				lastRun  = (size_t) (olimit-op) - 1;
				lastRun -= (lastRun + 256 - RUN_MASK) / 256;
			} else {
				assert(outputDirective == LZ4_limitedOutput);
				return 0;
			}
		}

		if(lastRun >= RUN_MASK) {
			size_t accumulator = lastRun - RUN_MASK;
			*op++ = RUN_MASK << ML_BITS;
			for(; accumulator >= 255; accumulator-=255) *op++ = 255;
			*op++ = (LZ4_byte) accumulator;
		} else {
			*op++ = (LZ4_byte) (lastRun << ML_BITS);
		}
		(void) UTRACY_MEMCPY(op, anchor, lastRun);
		ip = anchor + lastRun;
		op += lastRun;
	}

	if(outputDirective == LZ4_fillOutput) {
		*inputConsumed = (int) (((char const *) ip) - source);
	}
	result = (int) (((char *) op) - dest);
	assert(result > 0);
	return result;
}

UTRACY_INTERNAL UTRACY_INLINE
int LZ4_compress_generic(LZ4_stream_t_internal *const cctx, char const *const src, char *const dst, int const srcSize, int *inputConsumed, int const dstCapacity, LZ4_limitedOutput_directive const outputDirective, LZ4_tableType_t const tableType, LZ4_dict_directive const dictDirective, LZ4_dictIssue_directive const dictIssue, int const acceleration) {
	if((LZ4_u32) srcSize > (LZ4_u32) LZ4_MAX_INPUT_SIZE) {
		return 0;
	}

	if(srcSize == 0) {
		if(outputDirective != LZ4_notLimited && dstCapacity <= 0) return 0;
		assert(outputDirective == LZ4_notLimited || dstCapacity >= 1);
		assert(dst != NULL);
		dst[0] = 0;
		if(outputDirective == LZ4_fillOutput) {
			assert(inputConsumed != NULL);
			*inputConsumed = 0;
		}
		return 1;
	}

	assert(src != NULL);

	return LZ4_compress_generic_validated(
		cctx,
		src,
		dst,
		srcSize,
		inputConsumed,
		dstCapacity,
		outputDirective,
		tableType,
		dictDirective,
		dictIssue,
		acceleration
	);
}


UTRACY_INTERNAL UTRACY_INLINE
int LZ4_compress_fast_continue(LZ4_stream_t *LZ4_stream, char const *source, char *dest, int inputSize, int maxOutputSize, int acceleration) {
	LZ4_tableType_t const tableType = LZ4_byU32;
	LZ4_stream_t_internal *const streamPtr = &LZ4_stream->internal_donotuse;
	char const *dictEnd = streamPtr->dictSize ? (char const *) streamPtr->dictionary + streamPtr->dictSize : NULL;

	LZ4_renormDictT(streamPtr, inputSize);
	if(acceleration < 1) acceleration = LZ4_ACCELERATION_DEFAULT;
	if(acceleration > LZ4_ACCELERATION_MAX) acceleration = LZ4_ACCELERATION_MAX;

	if((streamPtr->dictSize < 4) && (dictEnd != source) && (inputSize > 0) && (streamPtr->dictCtx == NULL)) {
		streamPtr->dictSize = 0;
		streamPtr->dictionary = (LZ4_byte const *) source;
		dictEnd = source;
	}

	{
		char const *const sourceEnd = source + inputSize;
		if((sourceEnd > (char const *) streamPtr->dictionary) && (sourceEnd < dictEnd)) {
			streamPtr->dictSize = (LZ4_u32) (dictEnd - sourceEnd);
			if(streamPtr->dictSize > 65536u) streamPtr->dictSize = 65536u;
			if(streamPtr->dictSize < 4u) streamPtr->dictSize = 0;
			streamPtr->dictionary = (LZ4_byte const *) dictEnd - streamPtr->dictSize;
		}
	}

	if(dictEnd == source) {
		if((streamPtr->dictSize < 65536u) && (streamPtr->dictSize < streamPtr->currentOffset)) {
			return LZ4_compress_generic(
				streamPtr,
				source,
				dest,
				inputSize,
				NULL,
				maxOutputSize,
				LZ4_limitedOutput,
				tableType,
				LZ4_withPrefix64k,
				LZ4_dictSmall,
				acceleration
			);
		} else {
			return LZ4_compress_generic(
				streamPtr,
				source,
				dest,
				inputSize,
				NULL,
				maxOutputSize,
				LZ4_limitedOutput,
				tableType,
				LZ4_withPrefix64k,
				LZ4_noDictIssue,
				acceleration
			);
		}
	}

	{
		int result;
		if(streamPtr->dictCtx) {
			if(inputSize > 4096) {
				(void) UTRACY_MEMCPY(streamPtr, streamPtr->dictCtx, sizeof(*streamPtr));
				result = LZ4_compress_generic(
					streamPtr,
					source,
					dest,
					inputSize,
					NULL,
					maxOutputSize,
					LZ4_limitedOutput,
					tableType,
					LZ4_usingExtDict,
					LZ4_noDictIssue,
					acceleration
				);
			} else {
				result = LZ4_compress_generic(
					streamPtr,
					source,
					dest,
					inputSize,
					NULL,
					maxOutputSize,
					LZ4_limitedOutput,
					tableType,
					LZ4_usingDictCtx,
					LZ4_noDictIssue,
					acceleration
				);
			}
		} else {
			if ((streamPtr->dictSize < 65536u) && (streamPtr->dictSize < streamPtr->currentOffset)) {
				result = LZ4_compress_generic(
					streamPtr,
					source,
					dest,
					inputSize,
					NULL,
					maxOutputSize,
					LZ4_limitedOutput,
					tableType,
					LZ4_usingExtDict,
					LZ4_dictSmall,
					acceleration
				);
			} else {
				result = LZ4_compress_generic(
					streamPtr,
					source,
					dest,
					inputSize,
					NULL,
					maxOutputSize,
					LZ4_limitedOutput,
					tableType,
					LZ4_usingExtDict,
					LZ4_noDictIssue,
					acceleration
				);
			}
		}

		streamPtr->dictionary = (LZ4_byte const *) source;
		streamPtr->dictSize = (LZ4_u32) inputSize;
		return result;
	}
}

UTRACY_INTERNAL UTRACY_INLINE
int LZ4_isAligned(void const *ptr, size_t alignment) {
	return ((size_t) ptr & (alignment - 1)) == 0;
}

UTRACY_INTERNAL UTRACY_INLINE
size_t LZ4_stream_t_alignment(void) {
	typedef struct {char c; LZ4_stream_t t;} t_a;
	return sizeof(t_a) - sizeof(LZ4_stream_t);
}

UTRACY_INTERNAL UTRACY_INLINE
LZ4_stream_t *LZ4_initStream(void *buffer, size_t size) {
	if(buffer == NULL) {return NULL;}
	if(size < sizeof(LZ4_stream_t)) {return NULL;}
	if(!LZ4_isAligned(buffer, LZ4_stream_t_alignment())) return NULL;
	(void) UTRACY_MEMSET(buffer, 0, sizeof(LZ4_stream_t_internal));
	return (LZ4_stream_t *) buffer;
}

UTRACY_INTERNAL UTRACY_INLINE
void LZ4_resetStream_fast(LZ4_stream_t *ctx) {
	LZ4_prepareTable(&(ctx->internal_donotuse), 0, LZ4_byU32);
}

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
	void  *srcloc;
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

	struct {
		int connected;
#if defined(UTRACY_WINDOWS)
		WSADATA wsa;
		SOCKET server;
		SOCKET client;
#elif defined(UTRACY_LINUX)
		int server;
		int client;
#endif
	} sock;

	struct {
		struct {
			int unsigned tid;
			long long timestamp;
		} cur_thread;

		long long prev_commit;
		LZ4_stream_t stream;

		int unsigned raw_buf_head;
		int unsigned raw_buf_tail;
		char raw_buf[UTRACY_MAX_FRAME_SIZE * 3];

		int unsigned frame_buf_len;
		char frame_buf[sizeof(int) + LZ4_COMPRESSBOUND(UTRACY_MAX_FRAME_SIZE)];
	} data;

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

#define UTRACY_RESPONSE_SOURCELOCATION (67)
#define UTRACY_RESPONSE_ACKSERVERQUERYNOOP (87)
#define UTRACY_RESPONSE_ACKSOURCECODENOTAVAILABLE (88)
#define UTRACY_RESPONSE_ACKSYMBOLCODENOTAVAILABLE (89)
#define UTRACY_RESPONSE_STRINGDATA (94)
#define UTRACY_RESPONSE_THREADNAME (95)
#define UTRACY_RESPONSE_PLOTNAME (96)

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

struct utracy_source_location {
	char const *name;
	char const *function;
	char const *file;
	int unsigned line;
	int unsigned color;
};

static struct utracy_source_location srclocs[0x14000];

UTRACY_INTERNAL UTRACY_INLINE
void utracy_emit_zone_begin(struct utracy_source_location const *const srcloc) {
	event_queue_push(&(struct event) {
		.type = UTRACY_EVT_ZONEBEGIN,
		.zone_begin.tid = utracy_tid(),
		.zone_begin.timestamp = utracy_tsc(),
		.zone_begin.srcloc = (void *) srcloc
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

	struct utracy_source_location srcloc = {
		.name = NULL,
		.function = __func__,
		.file = __FILE__,
		.line = 0,
		.color = 0
	};

	long long clk0 = utracy_tsc();

	for(int unsigned i=0; i<iterations; i++) {
		utracy_emit_zone_begin(&srcloc);
		utracy_emit_zone_end();
	}

	long long clk1 = utracy_tsc();

	long long dclk = clk1 - clk0;

	struct event evt;
	while(0 == event_queue_pop(&evt));

	return dclk / (long long) (iterations * 2);
}

/* server */
UTRACY_INTERNAL
int utracy_server_init(void) {
#if defined(UTRACY_WINDOWS)
	if(0 != WSAStartup(MAKEWORD(2, 2), &utracy.sock.wsa)) {
		LOG_DEBUG_ERROR;
		return -1;
	}
#endif

	utracy.sock.server = socket(AF_INET, SOCK_STREAM, 0);
	if(0 == utracy.sock.server) {
		LOG_DEBUG_ERROR;
		return -1;
	}

#if defined(UTRACY_WINDOWS)
	char opt = 1;
#elif defined(UTRACY_LINUX)
	int opt = 1;
#endif
	if(0 != setsockopt(utracy.sock.server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		LOG_DEBUG_ERROR;
		return -1;
	}

#if defined(UTRACY_WINDOWS)
	char node[64] = "127.0.0.1\0";
	if(sizeof(node) < GetEnvironmentVariable("UTRACY_BIND_ADDRESS", node, sizeof(node))) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	char service[6] = "8086\0";
	if(sizeof(service) < GetEnvironmentVariable("UTRACY_BIND_PORT", service, sizeof(service))) {
		LOG_DEBUG_ERROR;
		return -1;
	}

#elif defined(UTRACY_LINUX)
	char *node = getenv("UTRACY_BIND_ADDRESS");
	if(NULL == node) {
		node = "127.0.0.1";
	}

	char *service = getenv("UTRACY_BIND_PORT");
	if(NULL == service) {
		service = "8086";
	}

#endif

	struct addrinfo hints = {
		.ai_flags = AI_PASSIVE,
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = IPPROTO_TCP
	};

	struct addrinfo *result;

retry:
	switch(getaddrinfo(node, service, &hints, &result)) {
		case 0:
			break;

		case EAI_AGAIN:
			goto retry;

		default:
			LOG_DEBUG_ERROR;
			return -1;
	}

	if(NULL == result) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	socklen_t addrlen = result->ai_addrlen;
	struct sockaddr_in *addr = (struct sockaddr_in *) result->ai_addr;

	if(0 > bind(utracy.sock.server, (struct sockaddr *) addr, addrlen)) {
		LOG_DEBUG_ERROR;
		freeaddrinfo(result);
		return -1;
	}

	if(0 > listen(utracy.sock.server, 2)) {
		LOG_DEBUG_ERROR;
		freeaddrinfo(result);
		return -1;
	}

#if defined(UTRACY_DEBUG)
	char ip[INET_ADDRSTRLEN];
	if(NULL == inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip))) {
		LOG_DEBUG_ERROR;
		freeaddrinfo(result);
		return -1;
	}

	LOG_INFO("listening on %s:%hu\n", ip, ntohs(addr->sin_port));
#endif

	freeaddrinfo(result);

	return 0;
}

UTRACY_INTERNAL
int utracy_client_accept(void) {
	struct sockaddr_in client;

	socklen_t len = sizeof(client);
	if(0 > (utracy.sock.client = accept(utracy.sock.server, (struct sockaddr *) &client, &len))) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	char ip[INET_ADDRSTRLEN];
	if(NULL == inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(ip))) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	LOG_INFO("received connection: %s\n", ip);
	utracy.sock.connected = 1;

	return 0;
}

UTRACY_INTERNAL
int utracy_client_recv(void *const buf, int unsigned len) {
	size_t offset = 0;

	while(offset < len) {
		int received = recv(utracy.sock.client, (char *) buf + offset, len - offset, 0);

		if(0 >= received) {
			LOG_DEBUG_ERROR;

#if defined(UTRACY_LINUX)
			if(EINTR == errno) {
				continue;
			}
#endif

			return -1;
		}

		offset += received;
	}

	return 0;
}

UTRACY_INTERNAL
int utracy_client_send(void *const buf, int unsigned len) {
	size_t offset = 0;

	while(offset < len) {
		int sent = send(utracy.sock.client, (char *) buf + offset, len - offset, 0);

		if(0 >= sent) {
			LOG_DEBUG_ERROR;

#if defined(UTRACY_LINUX)
			if(EINTR == errno) {
				continue;
			}
#endif

			return -1;
		}

		offset += sent;
	}

	return 0;
}

UTRACY_INTERNAL
int utracy_commit(void) {
	if(0 < utracy.data.raw_buf_head - utracy.data.raw_buf_tail) {
		int unsigned pending_len;
		pending_len = utracy.data.raw_buf_head - utracy.data.raw_buf_tail;

		do {
			int unsigned raw_frame_len = min(pending_len, UTRACY_MAX_FRAME_SIZE);

			/* write compressed buf */
			int unsigned compressed_len = LZ4_compress_fast_continue(
				&utracy.data.stream,
				/* src */
				utracy.data.raw_buf + utracy.data.raw_buf_tail,
				/* dst */
				utracy.data.frame_buf + sizeof(int),
				/* src len */
				raw_frame_len,
				/* dst max len */
				LZ4_COMPRESSBOUND(UTRACY_MAX_FRAME_SIZE),
				1
			);

			/* write compressed buf len */
			(void) UTRACY_MEMCPY(utracy.data.frame_buf + 0, &compressed_len, sizeof(compressed_len));

			/* transmit frame */
			if(0 != utracy_client_send(utracy.data.frame_buf, compressed_len + sizeof(compressed_len))) {
				LOG_DEBUG_ERROR;
				return -1;
			}

			/* advance tail */
			utracy.data.raw_buf_tail += raw_frame_len;
			pending_len = utracy.data.raw_buf_head - utracy.data.raw_buf_tail;
		} while(0 < pending_len);

		/* previous 64kb of uncompressed data must remain unclobbered at the
		   same memory address! */
		if(utracy.data.raw_buf_head >= UTRACY_MAX_FRAME_SIZE * 2) {
			utracy.data.raw_buf_head = 0;
			utracy.data.raw_buf_tail = 0;
		}

		utracy.data.prev_commit = utracy_tsc();
	}

	return 0;
}

UTRACY_INTERNAL UTRACY_INLINE
int utracy_write_packet(void const *const buf, int unsigned len) {
	int unsigned current_frame_size = utracy.data.raw_buf_head - utracy.data.raw_buf_tail;

	if(current_frame_size + len > UTRACY_MAX_FRAME_SIZE) {
		if(0 != utracy_commit()) {
			LOG_DEBUG_ERROR;
			return -1;
		}
	}

	if(len > sizeof(utracy.data.raw_buf) - utracy.data.raw_buf_head) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	(void) UTRACY_MEMCPY(utracy.data.raw_buf + utracy.data.raw_buf_head, buf, len);
	utracy.data.raw_buf_head += len;

	return 0;
}

UTRACY_INTERNAL UTRACY_INLINE
int utracy_write_server_context(int unsigned tid) {
#pragma pack(push, 1)
	struct network_thread_context {
		char unsigned type;
		int unsigned tid;
	};

	_Static_assert(5 == sizeof(struct network_thread_context), "incorrect size");
#pragma pack(pop)

	struct network_thread_context msg = {
		.type = UTRACY_EVT_THREADCONTEXT,
		.tid = tid
	};

	if(0 != utracy_write_packet(&msg, sizeof(msg))) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	return 0;
}

UTRACY_INTERNAL UTRACY_INLINE
int utracy_write_zone_begin(struct event evt) {
#pragma pack(push, 1)
	struct network_zone_begin {
		char unsigned type;
		long long timestamp;
		long long unsigned srcloc;
	};
	_Static_assert(17 == sizeof(struct network_zone_begin), "incorrect size");
#pragma pack(pop)

	long long timestamp = evt.zone_begin.timestamp - utracy.data.cur_thread.timestamp;
	utracy.data.cur_thread.timestamp = evt.zone_begin.timestamp;

	struct network_zone_begin msg = {
		.type = UTRACY_EVT_ZONEBEGIN,
		.timestamp = timestamp,
		.srcloc = (uintptr_t) evt.zone_begin.srcloc
	};

	if(0 != utracy_write_packet(&msg, sizeof(msg))) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	return 0;
}

UTRACY_INTERNAL UTRACY_INLINE
int utracy_write_zone_end(struct event evt) {
#pragma pack(push, 1)
	struct network_zone_end {
		char unsigned type;
		long long timestamp;
	};
	_Static_assert(9 == sizeof(struct network_zone_end), "incorrect size");
#pragma pack(pop)

	long long timestamp = evt.zone_end.timestamp - utracy.data.cur_thread.timestamp;
	utracy.data.cur_thread.timestamp = evt.zone_end.timestamp;

	struct network_zone_end msg = {
		.type = UTRACY_EVT_ZONEEND,
		.timestamp = timestamp
	};

	if(0 != utracy_write_packet(&msg, sizeof(msg))) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	return 0;
}

UTRACY_INTERNAL UTRACY_INLINE
int utracy_write_zone_color(struct event evt) {
#pragma pack(push, 1)
	struct network_zone_color {
		char unsigned type;
		char unsigned r;
		char unsigned g;
		char unsigned b;
	};
	_Static_assert(4 == sizeof(struct network_zone_color), "incorrect size");
#pragma pack(pop)

	struct network_zone_color msg = {
		.type = UTRACY_EVT_ZONECOLOR,
		.r = (evt.zone_color.color >> 0) & 0xFF,
		.g = (evt.zone_color.color >> 8) & 0xFF,
		.b = (evt.zone_color.color >> 16) & 0xFF
	};

	if(0 != utracy_write_packet(&msg, sizeof(msg))) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	return 0;
}

UTRACY_INTERNAL UTRACY_INLINE
int utracy_write_frame_mark(struct event evt) {
#pragma pack(push, 1)
	struct network_frame_mark {
		char unsigned type;
		long long timestamp;
		long long unsigned name;
	};
	_Static_assert(17 == sizeof(struct network_frame_mark), "incorrect size");
#pragma pack(pop)

	struct network_frame_mark msg = {
		.type = UTRACY_EVT_FRAMEMARKMSG,
		.timestamp = evt.frame_mark.timestamp,
		.name = (uintptr_t) evt.frame_mark.name
	};

	if(0 != utracy_write_packet(&msg, sizeof(msg))) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	return 0;
}

UTRACY_INTERNAL UTRACY_INLINE
int utracy_write_srcloc(struct utracy_source_location const *const srcloc) {
#pragma pack(push, 1)
	struct network_srcloc {
		char unsigned type;
		long long unsigned name;
		long long unsigned function;
		long long unsigned file;
		int unsigned line;
		char unsigned r;
		char unsigned g;
		char unsigned b;
	};
	_Static_assert(32 == sizeof(struct network_srcloc), "incorrect size");
#pragma pack(pop)

	struct network_srcloc msg = {
		.type = UTRACY_RESPONSE_SOURCELOCATION,
		.name = (uintptr_t) srcloc->name,
		.function = (uintptr_t) srcloc->function,
		.file = (uintptr_t) srcloc->file,
		.line = srcloc->line,
		.r = (srcloc->color >> 0) & 0xFF,
		.g = (srcloc->color >> 8) & 0xFF,
		.b = (srcloc->color >> 16) & 0xFF
	};

	if(0 != utracy_write_packet(&msg, sizeof(msg))) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	return 0;
}

UTRACY_INTERNAL UTRACY_INLINE
int utracy_write_stringdata(char unsigned type, char const *const str, long long unsigned ptr) {
#pragma pack(push, 1)
	struct network_query_stringdata {
		char unsigned type;
		long long unsigned ptr;
		short unsigned len;
		char str[];
	};
	_Static_assert(11 == sizeof(struct network_query_stringdata), "incorrect size");
#pragma pack(pop)

	short unsigned len = (short unsigned) strlen(str);
	size_t size = sizeof(struct network_query_stringdata) + len;

	static char buf[sizeof(struct network_query_stringdata) + 65536];
	struct network_query_stringdata *msg = (struct network_query_stringdata *) buf;

	msg->type = type;
	msg->ptr = ptr;
	msg->len = len;
	(void) UTRACY_MEMCPY(msg->str, str, len);

	if(0 != utracy_write_packet(msg, size)) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	return 0;
}

UTRACY_INTERNAL
int utracy_consume_request(void) {
#pragma pack(push, 1)
	struct network_recv_request {
		char unsigned type;
		long long unsigned ptr;
		int unsigned extra;
	};
	_Static_assert(13 == sizeof(struct network_recv_request), "incorrect size");
#pragma pack(pop)

	struct network_recv_request req;
	if(0 != utracy_client_recv(&req, sizeof(req))) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	switch(req.type) {
		case UTRACY_QUERY_STRING:
			if(0 != utracy_write_stringdata(UTRACY_RESPONSE_STRINGDATA, (char *) (uintptr_t) req.ptr, req.ptr)) {
				LOG_DEBUG_ERROR;
				return -1;
			}
			break;

		case UTRACY_QUERY_PLOTNAME:
			if(0 != utracy_write_stringdata(UTRACY_RESPONSE_PLOTNAME, (char *) (uintptr_t) req.ptr, req.ptr)) {
				LOG_DEBUG_ERROR;
				return -1;
			}
			break;

		case UTRACY_QUERY_THREADSTRING:
			if(0 != utracy_write_stringdata(UTRACY_RESPONSE_THREADNAME, "main", req.ptr)) {
				LOG_DEBUG_ERROR;
				return -1;
			}
			break;

		case UTRACY_QUERY_SOURCELOCATION:;
			if(0 != utracy_write_srcloc((void *) (uintptr_t) req.ptr)) {
				LOG_DEBUG_ERROR;
				return -1;
			}
			break;

		case UTRACY_QUERY_SYMBOLCODE:;
			char unsigned symbol_type = UTRACY_RESPONSE_ACKSYMBOLCODENOTAVAILABLE;
			if(0 != utracy_write_packet(&symbol_type, sizeof(symbol_type))) {
				LOG_DEBUG_ERROR;
				return -1;
			}
			break;

		case UTRACY_QUERY_SOURCECODE:;
			char unsigned sourcecode_type = UTRACY_RESPONSE_ACKSOURCECODENOTAVAILABLE;
			if(0 != utracy_write_packet(&sourcecode_type, sizeof(sourcecode_type))) {
				LOG_DEBUG_ERROR;
				return -1;
			}
			break;

		case UTRACY_QUERY_DATATRANSFER:
		case UTRACY_QUERY_DATATRANSFERPART:;
			char unsigned datatransfer_type = UTRACY_RESPONSE_ACKSERVERQUERYNOOP;
			if(0 != utracy_write_packet(&datatransfer_type, sizeof(datatransfer_type))) {
				LOG_DEBUG_ERROR;
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

UTRACY_INTERNAL UTRACY_INLINE
int utracy_switch_thread_context(int unsigned tid) {
	if(tid != utracy.data.cur_thread.tid) {
		utracy.data.cur_thread.tid = tid;
		utracy.data.cur_thread.timestamp = 0ll;

		return utracy_write_server_context(tid);
	}

	return 0;
}

static int unsigned hacky_proc_stack;

UTRACY_INTERNAL
int utracy_consume_queue(void) {
	struct event evt;

	if(1 != utracy.sock.connected) {
		while(0 == event_queue_pop(&evt));
		return 0;
	}

	while(0 == event_queue_pop(&evt)) {
		switch(evt.type) {
			case UTRACY_EVT_ZONEBEGIN:
				if(0 != utracy_switch_thread_context(evt.zone_begin.tid)) {
					LOG_DEBUG_ERROR;
					return -1;
				}

				if(0 != utracy_write_zone_begin(evt)) {
					LOG_DEBUG_ERROR;
					return -1;
				}

				hacky_proc_stack++;
				break;

			case UTRACY_EVT_ZONEEND:
				if(0 < hacky_proc_stack) {
					hacky_proc_stack--;
				} else {
					break;
				}

				if(0 != utracy_switch_thread_context(evt.zone_end.tid)) {
					LOG_DEBUG_ERROR;
					return -1;
				}

				if(0 != utracy_write_zone_end(evt)) {
					LOG_DEBUG_ERROR;
					return -1;
				}
				break;

			case UTRACY_EVT_ZONECOLOR:
				if(!hacky_proc_stack) {
					break;
				}

				if(0 != utracy_switch_thread_context(evt.zone_color.tid)) {
					LOG_DEBUG_ERROR;
					return -1;
				}

				if(0 != utracy_write_zone_color(evt)) {
					LOG_DEBUG_ERROR;
					return -1;
				}
				break;

			case UTRACY_EVT_FRAMEMARKMSG:
				if(0 != utracy_write_frame_mark(evt)) {
					LOG_DEBUG_ERROR;
					return -1;
				}
				break;

			default:
				LOG_DEBUG_ERROR;
				return -1;
		}
	}

	return 0;
}

UTRACY_INTERNAL
int utracy_server_pump(void) {
	if(0 != utracy_consume_queue()) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	if(1 != utracy.sock.connected) {
		return -2;
	}

#if defined(UTRACY_WINDOWS)
	WSAPOLLFD descriptor = {
		.fd = utracy.sock.client,
		.events = POLLRDNORM,
		.revents = 0
	};

	int polled = WSAPoll(&descriptor, 1, 1);

#elif defined(UTRACY_LINUX)
	struct pollfd descriptor = {
		.fd = utracy.sock.client,
		.events = POLLRDNORM,
		.revents = 0
	};

	int polled = poll(&descriptor, 1, 1);

#endif

	if(0 < polled) {
		if(0 != utracy_consume_request()) {
			LOG_DEBUG_ERROR;
			return -1;
		}

	} else if(-1 == polled) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	long long now = utracy_tsc();
	if(now - utracy.data.prev_commit >= UTRACY_LATENCY) {
		if(0 != utracy_commit()) {
			LOG_DEBUG_ERROR;
			return -1;
		}
	}

	return 0;
}

UTRACY_INTERNAL
int utracy_client_negotiate(void) {
	char handshake[8];
	if(0 != utracy_client_recv(handshake, sizeof(handshake))) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	if(0 != UTRACY_MEMCMP(handshake, "TracyPrf", 8)) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	int unsigned protocol;
	if(0 != utracy_client_recv(&protocol, sizeof(protocol))) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	int unsigned supported_protocols[] = {
		56, /* 0.8.1 */
		57, /* 0.8.2 */
		0 /* sentinel - do not remove */
	};

	int compatible = 0;
	for(int unsigned *p=supported_protocols; *p; p++) {
		if(*p == protocol) {
			compatible = 1;
			break;
		}
	}

	if(0 == compatible) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	char unsigned response = 1;
	if(0 != utracy_client_send(&response, sizeof(response))) {
		LOG_DEBUG_ERROR;
		return -1;
	}

#pragma pack(push, 1)
	struct network_welcome {
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
	};
	_Static_assert(1178 == sizeof(struct network_welcome), "incorrect size");
#pragma pack(pop)

	struct network_welcome welcome = {
		.multiplier = utracy.info.multiplier,
		.init_begin = utracy.info.init_begin,
		.init_end = utracy.info.init_end,
		.delay = utracy.info.delay,
		.resolution = utracy.info.resolution,
		.epoch = utracy.info.epoch,
		.exec_time = utracy.info.exec_time,
		.pid = 0,
		.sampling_period = 0,
		.flags = 1,
		.cpu_arch = 0,
		.cpu_manufacturer = "???",
		.cpu_id = 0,
		.program_name = "DREAMDAEMON",
		.host_info = "???"
	};

	if(0 != utracy_client_send(&welcome, sizeof(welcome))) {
		LOG_DEBUG_ERROR;
		return -1;
	}

#pragma pack(push, 1)
	struct network_ondemand {
		long long unsigned frames;
		long long unsigned timestamp;
	};
	_Static_assert(16 == sizeof(struct network_ondemand), "incorrect size");
#pragma pack(pop)

	struct network_ondemand ondemand = {
		.frames = 0ull,
		.timestamp = utracy_tsc()
	};

	if(0 != utracy_client_send(&ondemand, sizeof(ondemand))) {
		LOG_DEBUG_ERROR;
		return -1;
	}

	return 0;
}

UTRACY_INTERNAL
int utracy_client_disconnect(void) {
	utracy.sock.connected = 0;

#if defined(UTRACY_WINDOWS)
	if(0 != shutdown(utracy.sock.client, SD_SEND)) {
		LOG_DEBUG_ERROR;
	}

	if(0 != closesocket(utracy.sock.client)) {
		LOG_DEBUG_ERROR;
		return -1;
	}

#elif defined(UTRACY_LINUX)
	if(0 != close(utracy.sock.client)) {
		LOG_DEBUG_ERROR;
		return -1;
	}

#endif

	return 0;
}

UTRACY_INTERNAL
#if defined(UTRACY_WINDOWS)
DWORD WINAPI utracy_server_thread_start(PVOID user) {
#elif defined(UTRACY_LINUX)
void *utracy_server_thread_start(void *user) {
#endif
	(void) user;

	do {
		if(0 == utracy.sock.connected) {
#if defined(UTRACY_WINDOWS)
			WSAPOLLFD descriptor = {
				.fd = utracy.sock.server,
				.events = POLLRDNORM,
				.revents = 0
			};

			int polled = WSAPoll(&descriptor, 1, 1);

#elif defined(UTRACY_LINUX)
			struct pollfd descriptor = {
				.fd = utracy.sock.server,
				.events = POLLRDNORM,
				.revents = 0
			};

			int polled = poll(&descriptor, 1, 1);

#endif

			if(0 < polled) {
				if(0 != utracy_client_accept()) {
					LOG_DEBUG_ERROR;
					continue;
				}

				(void) UTRACY_MEMSET(&utracy.data, 0, sizeof(utracy.data));
				LZ4_resetStream_fast(&utracy.data.stream);
				hacky_proc_stack = 0;

				if(0 != utracy_client_negotiate()) {
					LOG_DEBUG_ERROR;
					(void) utracy_client_disconnect();
					continue;
				}
			}
		}

		while(0 == utracy_server_pump());

		if(1 == utracy.sock.connected) {
			(void) utracy_client_disconnect();
		}
	} while(1);

#if defined(UTRACY_WINDOWS)
	ExitThread(0);
#elif defined(UTRACY_LINUX)
	pthread_exit(NULL);
#endif
}

/* byond hooks */
UTRACY_INTERNAL
struct object UTRACY_WINDOWS_CDECL UTRACY_LINUX_REGPARM(3) exec_proc(struct proc *proc) {
	if(likely(proc->procdef < 0x14000)) {
		utracy_emit_zone_begin(srclocs + proc->procdef);

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
	static struct utracy_source_location const srcloc = {
		.name = NULL,
		.function = "ServerTick",
		.file = __FILE__,
		.line = __LINE__,
		.color = 0x44AF44
	};

	/* server tick is the end of a frame and the beginning of the next frame */
	utracy_emit_frame_mark(NULL);

	utracy_emit_zone_begin(&srcloc);

	int interval = byond.orig_server_tick();

	utracy_emit_zone_end();

	return interval;
}

UTRACY_INTERNAL
void UTRACY_WINDOWS_CDECL UTRACY_LINUX_CDECL send_maps(void) {
	static struct utracy_source_location const srcloc = {
		.name = NULL,
		.function = "SendMaps",
		.file = __FILE__,
		.line = __LINE__,
		.color = 0x44AF44
	};

	utracy_emit_zone_begin(&srcloc);

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
#	define BYOND_MAX_BUILD 1593
#	define BYOND_MIN_BUILD 1543
#	define BYOND_VERSION_ADJUSTED(a) ((a) - BYOND_MIN_BUILD)

static int unsigned const byond_offsets[][7] = {
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
};

#elif defined(UTRACY_LINUX)
#	define BYOND_MAX_BUILD 1593
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
	[BYOND_VERSION_ADJUSTED(1593)] = {0x006B1C68, 0x006B1C80, 0x006B1CCC, 0x00313820, 0x003001A0, 0x002F61A0, 0x00050505},
};

#endif

UTRACY_INTERNAL
void build_srclocs(void) {
#define byond_get_string(id) (id < *byond.strings_len ? *(*byond.strings + id) : NULL)
#define byond_get_misc(id) (id < *byond.miscs_len ? *(*byond.miscs + id) : NULL)
#define byond_get_procdef(id) (id < *byond.procdefs_len ? *byond.procdefs + id : NULL)

	for(int unsigned i=0; i<0x14000; i++) {
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

	if(NULL == LZ4_initStream(&utracy.data.stream, sizeof(utracy.data.stream))) {
		LOG_DEBUG_ERROR;
		return "LZ4_initStream failed";
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

	if(0 != utracy_server_init()) {
		return "failed to init server";
	}

	utracy.info.resolution = calibrate_resolution();
	utracy.info.delay = calibrate_delay();
	utracy.info.multiplier = calibrate_multiplier();
	utracy.info.epoch = unix_timestamp();
	utracy.info.exec_time = unix_timestamp();
	utracy.info.init_end = utracy_tsc();

#if defined(UTRACY_WINDOWS)
	if(NULL == CreateThread(NULL, 0, utracy_server_thread_start, NULL, 0, NULL)) {
		LOG_DEBUG_ERROR;
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

	return "0";
}

#if (__STDC_HOSTED__ == 0)
BOOL WINAPI DllMainCRTStartup(HINSTANCE instance, DWORD reason, LPVOID reserved) {
	return TRUE;
}
#endif
