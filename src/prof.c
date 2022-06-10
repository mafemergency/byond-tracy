#include "minwin.h"
#include "byond.h"
#include "MinHook/MinHook.h"
#include "tracy-0.8.1/TracyC.h"

struct dmn_byond byond = {0};
PFN_CALL_PROC orig_call_proc;

struct object call_proc(
	struct object usr,
	int unsigned proc_type,
	int unsigned proc_id,
	int unsigned unk0,
	struct object src,
	struct object *argv,
	int unsigned argc,
	int unsigned unk1,
	int unsigned unk2
) {
	int unsigned line = 0;
	char *file = "???";
	size_t file_len = strlen(file);
	struct procdef *proc = byond.get_proc(proc_id);
	struct string *str = byond.get_string(proc->name);
	char *name = str->data;
	size_t name_len = strlen(name);

	long long unsigned tracy_src = ___tracy_alloc_srcloc(line, file, file_len, name, name_len);
	struct ___tracy_c_zone_context tracy_ctx = ___tracy_emit_zone_begin_alloc(tracy_src, 1);
	struct object result = orig_call_proc(usr, proc_type, proc_id, unk0, src, argv, argc, unk1, unk2);
	___tracy_emit_zone_end(tracy_ctx);
	return result;
}

__declspec(dllexport) char *init(int argc, char **argv) {
	(void) argc;
	(void) argv;

	if(dmn_byond_init(&byond)) {
		DebugBreak();
	}

	if(MH_Initialize()) {
		DebugBreak();
	}

	if(MH_CreateHook((void *) byond.call_proc, (void *) call_proc, (void **) &orig_call_proc)) {
		DebugBreak();
	}

	if(MH_EnableHook((void *) byond.call_proc)) {
		DebugBreak();
	}

	byond.print("initialized\n");
	return NULL;
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
