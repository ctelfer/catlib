/*
 * match.h -- fast string matching algorithms.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#ifndef __match_h
#define __match_h
#include <cat/cat.h>
#include <cat/mem.h>
#include <cat/list.h>
#include <cat/hash.h>

/* Knuth-Morris-Pratt String matching */

struct kmppat {
	ulong *		skips;
	struct raw	pat;
};

void kmp_pinit(struct kmppat *p, struct raw *pat, ulong *skips);
int kmp_match(const struct raw *str, struct kmppat *pat, ulong *loc);


/* Boyer-Moore String matching */


struct bmpat {
	ulong		last[256];
	struct raw	pat;
	ulong *		skips;
};

void bm_pinit(struct bmpat *bmp, struct raw *pat, ulong *skips, 
	      ulong *scratch);

/* This function initializes a Boyer-Moore pattern, but does not perform */
/* skip-ahead calculation.  (i.e. the algorithm will only skip forward */
/* based on the last time the mismatched character was seen.  This is */
/* quicker to initialize and lower memory, but can be less efficient */
/* especially for longer patterns. */
void bm_pinit_lite(struct bmpat *bmp, struct raw *pat);
int bm_match(struct raw *str, struct bmpat *pat, ulong *loc);


/* Suffix Tree string matching */

#ifndef CAT_SFX_MAXLEN
#define CAT_SFX_MAXLEN 0x7ffffffe
#undef CAT_SFX_EXPLICIT
#define CAT_SFX_EXPLICIT (CAT_SFX_MAXLEN + 1)
#endif /* CAT_SFX_MAXLEN */


struct sfxnode {
	struct sfxnode *		sptr;
};

struct sfxedge {
	struct hnode			hentry;
	long				start;
	long				end;
};

struct suffix {
	struct sfxnode *		node;
	long				start;
	long				end;
};

struct sfxedgekey {
	struct sfxnode *		node;
	ushort				character;
};

struct sfxtree {
	struct memmgr *			mm;
	struct raw			str;
	struct htab			edges;
	struct sfxnode			root;
};


int  sfx_init(struct sfxtree *sfx, struct raw *str, struct memmgr *mm);
int  sfx_match(struct sfxtree *sfx, struct raw *pat, ulong *off);
void sfx_clear(struct sfxtree *sfx);
struct sfxnode *sfx_next(struct sfxtree *t, struct sfxnode *cur, int ch);


/* Regular expression string matching */


#define REX_T_STRING		0
#define REX_T_CLASS		1
#define REX_T_BANCHOR		2	/* no repitition */
#define REX_T_EANCHOR		3	/* no repitition */
#define REX_T_CHOICE		4	/* no repitition */
#define REX_T_GROUP_S		5	/* no repitition */
#define REX_T_GROUP_E		6
#define REX_WILDCARD		0

struct rex_node {
	uchar			type;
	uchar			repmin;
	uchar			repmax;
	struct rex_node *	next;
};

struct rex_node_str {
	struct rex_node		base;
	uchar			str[32];
	ulong 			len;
};

struct rex_ascii_class {
	struct rex_node 	base;
	uchar 			set[32];
};

struct rex_group {
	struct rex_node		base;
	unsigned		num;
	struct rex_group *	other;
};

struct rex_choice {
	struct rex_node		base;
	struct rex_node *	opt1;
	struct rex_node *	opt2;
};

struct rex_pat {
	struct memmgr *		mm;
	int			start_anchor;
	struct rex_group 	start;
	struct rex_group 	end;
};

struct rex_match_loc {
	int			valid;
	size_t			start;
	size_t			len;
};


#define REX_MATCH	0
#define REX_NOMATCH	1
#define REX_ERROR	-1

int rex_init(struct rex_pat *rxp, struct raw *pat, struct memmgr *mm,
	     int *error);
int rex_match(struct rex_pat *rxp, struct raw *str, struct rex_match_loc *m,
	      uint nm);
void rex_free(struct rex_pat *rxp);

#endif /* __match_h */
