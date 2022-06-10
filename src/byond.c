#include "minwin.h"
#include <psapi.h>
#include "byond.h"
#include "signatures.h"
#include <stdio.h>

static char unsigned *followcall(char unsigned *addr) {
	if(!addr) return NULL;
	int unsigned *call = (int unsigned *) (addr + 1);
	char unsigned *x = addr + *call;
	return x + 5;
}

void *dmn_byond_scanmem(
	struct dmn_byond *byond,
	char unsigned const *const pattern,
	char unsigned const *const mask,
	size_t len
) {
	size_t pos = 0;

	for(char unsigned const *cur=byond->start; cur<byond->end; cur++) {
		if((cur[0] & mask[pos]) == (pattern[pos] & mask[pos])) {
			if(++pos == len) {
				return (void *) (cur - (pos - 1));
			}
		} else {
			pos = 0;
		}
	}

	return NULL;
}

int dmn_byond_init(struct dmn_byond *byond) {
	BOOL r;

	HMODULE byondcore;
	r = GetModuleHandleExA(
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		"byondcore.dll",
		&byondcore
	);

	if(r == 0) {
		return -1;
	}

	MODULEINFO info;
	r = GetModuleInformation(
		GetCurrentProcess(),
		byondcore,
		&info,
		sizeof(info)
	);

	if(r == 0) {
		return -1;
	}

	byond->start = info.lpBaseOfDll;
	byond->end = byond->start + info.SizeOfImage;

	byond->print = (PFN_PRINT) dmn_byond_scanmem(
		byond,
		sig_print,
		mask_print,
		sizeof(sig_print)
	);

	byond->get_string = (PFN_GET_STRING) dmn_byond_scanmem(
		byond,
		sig_get_string,
		mask_get_string,
		sizeof(sig_get_string)
	);

	byond->call_proc = (PFN_CALL_PROC) followcall(dmn_byond_scanmem(
		byond,
		sig_call_proc,
		mask_call_proc,
		sizeof(sig_call_proc)
	));

	byond->get_proc = (PFN_GET_PROC) followcall(dmn_byond_scanmem(
		byond,
		sig_get_proc,
		mask_get_proc,
		sizeof(sig_get_proc)
	));

	if(byond->print == NULL) return -1;
	if(byond->get_string == NULL) return -1;
	if(byond->call_proc == NULL) return -1;
	if(byond->get_proc == NULL) return -1;

	return 0;
}
