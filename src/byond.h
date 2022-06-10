#ifndef DMN_BYOND_H
#define DMN_BYOND_H

struct object {
	int unsigned type;
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
	int unsigned unk1;
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

typedef void (*PFN_PRINT) (
	char *
);

typedef struct string *(*PFN_GET_STRING) (
	int unsigned
);

typedef struct object (*PFN_CALL_PROC) (
	struct object,
	int unsigned,
	int unsigned,
	int unsigned,
	struct object,
	struct object *,
	int unsigned,
	int unsigned,
	int unsigned
);

typedef struct procdef *(*PFN_GET_PROC) (
	int unsigned
);

struct dmn_byond {
	char unsigned *start;
	char unsigned *end;

	PFN_PRINT print;
	PFN_GET_STRING get_string;
	PFN_CALL_PROC call_proc;
	PFN_GET_PROC get_proc;
};

int dmn_byond_init(
	struct dmn_byond *byond
);

void *dmn_byond_scanmem(
	struct dmn_byond *byond,
	char unsigned const *pattern,
	char unsigned const *mask,
	size_t len
);

#endif
