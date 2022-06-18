#ifndef BYOND_H
#define BYOND_H

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

struct proc {
	int unsigned procdef;
	char unsigned flags;
	/* dreamdaemon dies silently if you have more than 255 overrides */
	char unsigned supers;
	/* likely just padding */
	short unsigned unused;
	struct object usr;
	struct object src;
	void *ctx;
	/* incremented each time a new (not resumed) proc is called */
	int unsigned sequence;
	void (*callback)(struct object, int unsigned);
	int unsigned callback_arg;
	int unsigned argc;
	struct object *argv;
	/* related to argv */
	int unsigned unk0;
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

#endif
