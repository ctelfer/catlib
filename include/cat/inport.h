/*
 * inport.c -- generic input API.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#ifndef __cat_inport_h
#define __cat_inport_h
#include <cat/cat.h>

#define READCHAR_CHAR      0
#define READCHAR_NONE      1
#define READCHAR_END       2
#define READCHAR_ERROR     3

struct inport;
typedef int (*readchar_f)(struct inport *in, char *out);

struct inport {
	readchar_f            read;
};

int readchar(struct inport *in, char *ch);


struct string_inport {
	struct inport         in;
	const char *          start;
	const char *          end;
	const char *          cur;
};

void string_inport_init(struct string_inport *sin, const char *s);
void string_inport_reset(struct string_inport *sin);

void null_inport_init(struct inport *in);
extern struct inport null_inport;

#endif /* __cat_inport_h */
