#include <windows.h>
#include "byond.h"
#include "MinHook/MinHook.h"
#include "microtracy/microtracy.h"
#include <stdio.h>

#define MAX_SUPPORTED_BYOND_VERSION \
	(sizeof(byond_offsets) / sizeof(byond_offsets[0]) - 1)

int unsigned byond_offsets[][4] = {
	/*	tables      exec_proc   servertick  sendmaps */
	[1543] = {0x0035FC58, 0x001003B0, 0x001C7D20, 0x00187C80},
	[1544] = {0x00360C58, 0x00100A10, 0x001C8420, 0x00188220},
	[1545] = {0x00360C60, 0x00100980, 0x001C8400, 0x00188190},
	[1546] = {0x00360C60, 0x00100830, 0x001C8280, 0x001880C0},
	[1547] = {0x00362C68, 0x00101210, 0x001C9320, 0x001891F0},
	[1548] = {0x00362C48, 0x00101640, 0x001C96D0, 0x00188E80},
	[1549] = {0x00368DD4, 0x001023B0, 0x001CB0A0, 0x0018AD80},
	[1550] = {0x0036903C, 0x00102710, 0x001CB710, 0x0018B0B0},
	[1551] = {0x00369034, 0x00102C30, 0x001CB830, 0x0018B120},
	[1552] = {0x0036A054, 0x00102DE0, 0x001CBDE0, 0x0018B6B0},
	[1553] = {0x0036E234, 0x00104FF0, 0x001CF780, 0x0018DE50},
	[1554] = {0x0036DFF8, 0x00104ED0, 0x001CF650, 0x0018E000},
	[1555] = {0x0036E0B0, 0x001064F0, 0x001CFD80, 0x0018EEB0},
	[1556] = {0x0036E0AC, 0x00106560, 0x001CFD80, 0x0018EEE0},
	[1557] = {0x0036E0C0, 0x001063B0, 0x001CFB60, 0x0018EC70},
	[1558] = {0x0036F4F4, 0x00106DE0, 0x001D1160, 0x0018FD80},
	[1559] = {0x0036F4F4, 0x00106DE0, 0x001D1160, 0x0018FD80},
	[1560] = {0x0036F4F4, 0x00106AF0, 0x001D1120, 0x0018FA80},
	[1561] = {0x0036F4F4, 0x00106AF0, 0x001D1120, 0x0018FA80},
	[1562] = {0x0036F538, 0x00106960, 0x001D0F00, 0x0018F780},
	[1563] = {0x0036F538, 0x001066A0, 0x001D1160, 0x0018F660},
	[1564] = {0x0036F538, 0x00106310, 0x001D0F20, 0x0018F1E0},
	[1565] = {0x00371538, 0x00106960, 0x001D15A0, 0x0018FCC0},
	[1566] = {0x00371538, 0x00106160, 0x001D0A70, 0x0018EF80},
	[1567] = {0x00370548, 0x00106220, 0x001D0B00, 0x0018F470},
	[1568] = {0x00370548, 0x00106220, 0x001D0B30, 0x0018F470},
	[1569] = {0x00370548, 0x00106220, 0x001D0B40, 0x0018F500},
	[1570] = {0x00371548, 0x00106560, 0x001D0BF0, 0x0018F8F0},
	[1571] = {0x00371548, 0x001061D0, 0x001D0A70, 0x0018F500},
	[1572] = {0x00371540, 0x001066A0, 0x001D0F60, 0x0018FCC0},
	[1573] = {0x00371608, 0x00106BD0, 0x001D13C0, 0x0018FC40},
	[1574] = {0x00371550, 0x001065A0, 0x001D10E0, 0x0018FDC0},
	[1575] = {0x00371550, 0x001065A0, 0x001D10E0, 0x0018FDC0},
	[1576] = {0x003745BC, 0x001087B0, 0x001D30A0, 0x00191C60},
	[1577] = {0x003745BC, 0x00107FC0, 0x001D2C90, 0x00191A60},
	[1578] = {0x003745BC, 0x001083B0, 0x001D2E90, 0x00191910},
	[1579] = {0x003745C8, 0x00108C20, 0x001D3940, 0x001925C0},
	[1580] = {0x003745C8, 0x00108BD0, 0x001D38B0, 0x00192520},
	[1581] = {0x003745C8, 0x001086A0, 0x001D3780, 0x001923A0},
	[1582] = {0x003745C8, 0x001087B0, 0x001D3A40, 0x00191FF0},
	[1583] = {0x003755C8, 0x00108240, 0x001D33F0, 0x001919E0},
	[1584] = {0x003764B4, 0x00108460, 0x001D3A40, 0x001922C0},
	[1585] = {0x003774AC, 0x001094D0, 0x001D49E0, 0x00192D80},
};

/* stores source locations for up to 65,535 procs
   funny quirk is byond supports more than 0xFFFF procs but the 0xFFFFth proc
   is a sentinel value sort of like NULL so the 0xFFFFth proc skips this value
   and is 0x10000 instead
   tgstation has over 50,000 procdefs */
static struct utracy_source_location srclocs[0x10000];
static struct {
	struct string ***strings;
	int unsigned *strings_len;
	struct misc ***miscs;
	int unsigned *miscs_len;
	struct procdef **procdefs;
	int unsigned *procdefs_len;
	void *exec_proc;
	struct object (*orig_exec_proc)(struct proc *);
	void *servertick;
	int (*orig_servertick)(void);
	void *sendmaps;
	void (*orig_sendmaps)(void);
} byond;
static struct {
	HANDLE initialized_event;
	HANDLE connected_event;
	HANDLE request_shutdown_event;
	HANDLE thread;
} utracy_thread;

/* requires compiler with good msvc compatibility to return this struct in
   registers EDX:EAX */
static struct object exec_proc(struct proc *proc) {
	if(proc->procdef < 0x10000) {
		utracy_emit_zone_begin(srclocs + proc->procdef);

		/* procs with pre-existing contexts are resuming from sleep */
		if(proc->ctx != NULL) {
			utracy_emit_zone_color(0xAF4444);
		}

		struct object result = byond.orig_exec_proc(proc);

		utracy_emit_zone_end();

		return result;
	}

	return byond.orig_exec_proc(proc);
}

static int servertick(void) {
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

	int interval = byond.orig_servertick();

	utracy_emit_zone_end();

	return interval;
}

static void sendmaps(void) {
	static struct utracy_source_location const srcloc = {
		.name = NULL,
		.function = "SendMaps",
		.file = __FILE__,
		.line = __LINE__,
		.color = 0x44AF44
	};

	utracy_emit_zone_begin(&srcloc);

	byond.orig_sendmaps();

	utracy_emit_zone_end();
}

/* prepopulate source locations for all procs */
static void build_srclocs(void) {
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

	#undef byond_get_string
	#undef byond_get_misc
	#undef byond_get_procdef
}

static int byond_init(char unsigned *byondcore) {
	typedef int (*PFN_BYOND_BUILD)(void);

	 PFN_BYOND_BUILD byond_build = (PFN_BYOND_BUILD) GetProcAddress(
		(HMODULE) byondcore,
		"?GetByondBuild@ByondLib@@QAEJXZ"
	);

	if(NULL == byond_build) {
		return -1;
	}

	int unsigned build = byond_build();

	if(MAX_SUPPORTED_BYOND_VERSION < build) {
		return -1;
	}

	int unsigned const *const offsets = byond_offsets[build];
	if(0x00000000 == offsets[0]) {
		return -1;
	}

	byond.strings = (void *) (byondcore + offsets[0] + 0x00);
	byond.strings_len = (void *) (byondcore + offsets[0] + 0x04);
	byond.miscs = (void *) (byondcore + offsets[0] + 0x10);
	byond.miscs_len = (void *) (byondcore + offsets[0] + 0x14);
	byond.procdefs = (void *) (byondcore + offsets[0] + 0x20);
	byond.procdefs_len = (void *) (byondcore + offsets[0] + 0x24);
	byond.exec_proc = (void *) (byondcore + offsets[1]);
	byond.servertick = (void *) (byondcore + offsets[2]);
	byond.sendmaps = (void *) (byondcore + offsets[3]);

	return 0;
}

static int prof_destroy(void) {
	if(NULL != byond.orig_servertick) {
		(void) MH_RemoveHook(byond.servertick);
		byond.orig_servertick = NULL;
	}

	if(NULL != byond.orig_sendmaps) {
		(void) MH_RemoveHook(byond.sendmaps);
		byond.orig_sendmaps = NULL;
	}

	if(NULL != byond.orig_exec_proc) {
		(void) MH_RemoveHook(byond.exec_proc);
		byond.orig_exec_proc = NULL;
	}

	(void) MH_Uninitialize();

	if(NULL != utracy_thread.thread) {
		if(0 != SetEvent(utracy_thread.request_shutdown_event)) {
			if(0xFFFFFFFFlu != ResumeThread(utracy_thread.thread)) {
				(void) WaitForSingleObject(utracy_thread.thread, INFINITE);
			}
		}
		utracy_thread.thread = NULL;
	}

	if(NULL != utracy_thread.request_shutdown_event) {
		(void) CloseHandle(utracy_thread.request_shutdown_event);
		utracy_thread.request_shutdown_event = NULL;
	}

	if(NULL != utracy_thread.initialized_event) {
		(void) CloseHandle(utracy_thread.initialized_event);
		utracy_thread.initialized_event = NULL;
	}

	return 0;
}

static int prof_init(void) {
	utracy_thread.initialized_event = CreateEventA(NULL, TRUE, FALSE, NULL);
	if(NULL == utracy_thread.initialized_event) {
		goto err;
	}

	utracy_thread.connected_event = CreateEventA(NULL, TRUE, FALSE, NULL);
	if(NULL == utracy_thread.connected_event) {
		goto err;
	}

	utracy_thread.request_shutdown_event = CreateEventA(NULL, TRUE, FALSE, NULL);
	if(NULL == utracy_thread.request_shutdown_event) {
		goto err;
	}

	utracy_thread.thread = CreateThread(
		NULL,
		0,
		utracy_thread_start,
		&utracy_thread,
		CREATE_SUSPENDED,
		NULL
	);

	if(NULL == utracy_thread.thread) {
		goto err;
	}

	void *byondcore;
	if(GetModuleHandleExA(
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		"byondcore.dll",
		(HMODULE *) &byondcore
	) == 0) {
		goto err;
	}

	if(0 != byond_init(byondcore)) {
		goto err;
	}

	if(MH_OK != MH_Initialize()) {
		goto err;
	}

	if(MH_OK != MH_CreateHook(byond.exec_proc, (void *) exec_proc, (void *) &byond.orig_exec_proc)) {
		goto err;
	}

	if(MH_OK != MH_CreateHook(byond.sendmaps, (void *) sendmaps, (void *) &byond.orig_sendmaps)) {
		goto err;
	}

	if(MH_OK != MH_CreateHook(byond.servertick, (void *) servertick, (void *) &byond.orig_servertick)) {
		goto err;
	}
	(void) servertick;

	build_srclocs();
	utracy_emit_thread_name("byond");

	if(MH_OK != MH_EnableHook(MH_ALL_HOOKS)) {
		goto err;
	}

	if(0xFFFFFFFFlu == ResumeThread(utracy_thread.thread)) {
		goto err;
	}

	(void) WaitForSingleObject(utracy_thread.initialized_event, INFINITE);

	/* waits for connection! */
	(void) WaitForSingleObject(utracy_thread.connected_event, INFINITE);

	return 0;

err:
	(void) prof_destroy();
	return -1;
}

__declspec(dllexport) char *init(int argc, char **argv) {
	(void) argc;
	(void) argv;

	if(0 != prof_init()) {
		return "prof.dll failed initialization";
	}

	return "prof.dll initialized";
}

__declspec(dllexport) char *destroy(int argc, char **argv) {
	(void) argc;
	(void) argv;

	(void) prof_destroy();

	return NULL;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
	(void) reserved;

	switch(reason) {
		case DLL_PROCESS_ATTACH:
			(void) DisableThreadLibraryCalls(instance);
			(void) memset(&byond, 0, sizeof(byond));
			(void) memset(&utracy_thread, 0, sizeof(utracy_thread));
			break;

		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}
