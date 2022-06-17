#include <windows.h>
#include "byond.h"
#include "MinHook/MinHook.h"
#include "tracy-0.8.1/TracyC.h"
#include <stdio.h>

static struct ___tracy_source_location_data srclocs[0x10000];
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

/* requires compiler with good msvc compatibility to return this struct in
   registers EDX:EAX */
static struct object exec_proc(struct proc *proc) {
	if(proc->procdef < 0x10000) {
		struct ___tracy_c_zone_context tracy_ctx = ___tracy_emit_zone_begin(srclocs + proc->procdef, 1);

		/* procs with pre-existing contexts are resuming from sleep */
		if(proc->ctx != NULL) {
			___tracy_emit_zone_color(tracy_ctx, 0xAF4444);
		}

		struct object result = byond.orig_exec_proc(proc);

		___tracy_emit_zone_end(tracy_ctx);

		return result;
	}

	return byond.orig_exec_proc(proc);
}

static int servertick(void) {
	TracyCFrameMark;

	static struct ___tracy_source_location_data const srcloc = {
		.name = NULL,
		.function = "ServerTick",
		.file = __FILE__,
		.line = __LINE__,
		.color = 0x44AF44
	};

	struct ___tracy_c_zone_context tracy_ctx = ___tracy_emit_zone_begin(&srcloc, 1);

	int r = byond.orig_servertick();

	___tracy_emit_zone_end(tracy_ctx);

	return r;
}

static void sendmaps(void) {
	static struct ___tracy_source_location_data const srcloc = {
		.name = NULL,
		.function = "SendMaps",
		.file = __FILE__,
		.line = __LINE__,
		.color = 0x44AF44
	};

	struct ___tracy_c_zone_context tracy_ctx = ___tracy_emit_zone_begin(&srcloc, 1);

	byond.orig_sendmaps();

	___tracy_emit_zone_end(tracy_ctx);
}

/* prepopulate source locations for all procs */
static void build_srclocs(void) {
	#define byond_get_string(id) (id < *byond.strings_len ? *(*byond.strings + id) : NULL)
	#define byond_get_misc(id) (id < *byond.miscs_len ? *(*byond.miscs + id) : NULL)
	#define byond_get_procdef(id) (id < *byond.procdefs_len ? *byond.procdefs + id : NULL)

	for(int unsigned i=0; i<0x10000; i++) {
		struct ___tracy_source_location_data *const srcloc = srclocs + i;

		struct procdef const *const procdef = byond_get_procdef(i);
		if(procdef != NULL) {
			srcloc->name = NULL;
			srcloc->color = 0x4444AF;

			struct string const *const str = byond_get_string(procdef->path);
			if(str != NULL && str->len > 0) {
				srcloc->function = str->data;
			} else {
				srcloc->function = "<?>";
			}

			struct misc const *const misc = byond_get_misc(procdef->bytecode);
			if(misc != NULL) {
				int unsigned bytecode_len = misc->bytecode.len;
				int unsigned *bytecode = misc->bytecode.bytecode;
				if(bytecode_len >= 2) {
					if(bytecode[0x00] == 0x84) {
						int unsigned file = bytecode[0x01];
						if(file < *byond.strings_len) {
							srcloc->file = (*(*byond.strings + file))->data;
						} else {
							srcloc->file = "<?.dm>";
						}

						if(bytecode_len >= 4) {
							if(bytecode[0x02] == 0x85) {
								srcloc->line = bytecode[0x03];
							}
						}
					}
				}
			} else {
				srcloc->file = "<?.dm>";
				srcloc->line = 0xFFFFFFFF;
			}
		} else {
			srcloc->name = NULL;
			srcloc->function = "<?>";
			srcloc->file = "<?.dm>";
			srcloc->line = 0xFFFFFFFF;
			srcloc->color = 0x44AF44;
		}
	}

	#undef byond_get_string
	#undef byond_get_misc
	#undef byond_get_procdef
}

static int prof_init(void) {
	void *byondcore;
	if(GetModuleHandleExA(
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		"byondcore.dll",
		(HMODULE *) &byondcore
	) == 0) {
		return -1;
	}

	/* only valid for 514.1584 */
	byond.strings = byondcore + 0x003764B4;
	byond.strings_len = byondcore + 0x003764B8;
	byond.miscs = byondcore + 0x003764C4;
	byond.miscs_len = byondcore + 0x003764C8;
	byond.procdefs = byondcore + 0x003764D4;
	byond.procdefs_len = byondcore + 0x003764D8;
	byond.exec_proc = byondcore + 0x00108460;
	byond.servertick = byondcore + 0x001D3A40;
	byond.sendmaps = byondcore + 0x001922C0;

	if(MH_Initialize() != MH_OK) {
		return -1;
	}

	if(MH_CreateHook(byond.exec_proc, (void *) exec_proc, (void *) &byond.orig_exec_proc)) {
		return -1;
	}

	if(MH_EnableHook(byond.exec_proc)) {
		return -1;
	}

	if(MH_CreateHook(byond.sendmaps, (void *) sendmaps, (void *) &byond.orig_sendmaps)) {
		return -1;
	}

	if(MH_CreateHook(byond.servertick, (void *) servertick, (void *) &byond.orig_servertick)) {
		return -1;
	}

	if(MH_EnableHook(MH_ALL_HOOKS)) {
		return -1;
	}

	build_srclocs();
	___tracy_set_thread_name("byond thread");

	return 0;
}

__declspec(dllexport) char *init(int argc, char **argv) {
	(void) argc;
	(void) argv;

	if(prof_init()) {
		return "prof.dll failed initialization";
	}

	return "prof.dll initialized";
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
	(void) reserved;

	switch(reason) {
		case DLL_PROCESS_ATTACH:
			(void) DisableThreadLibraryCalls(instance);
			break;
	}

	return TRUE;
}
