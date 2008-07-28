#ifndef __match_h
#define __match_h
#include <cat/cat.h>
#include <cat/mem.h>
#include <cat/list.h>
#include <cat/hash.h>

struct kmppat {
	size_t *		skips;
	struct raw		pat;
};

void kmp_pinit(struct kmppat *p, struct raw *pat, size_t *skips);
int kmp_match(const struct raw *str, struct kmppat *pat, size_t *loc);


struct bmpat {
	size_t			last[256];
	struct raw		pat;
	size_t *		skips;
};

void bm_pinit(struct bmpat *bmp, struct raw *pat, size_t *skips, size_t *scrat);
int bm_match(struct raw *str, struct bmpat *pat, size_t *loc);


#ifndef CAT_SFX_MAXLEN
#define CAT_SFX_MAXLEN 0x7ffffffe
#undef CAT_SFX_EXPLICIT
#define CAT_SFX_EXPLICIT (CAT_SFX_MAXLEN + 1)
#endif /* CAT_SFX_MAXLEN */


struct sfxnode {
	struct sfxnode *	sptr;
};

struct sfxedge {
	struct hnode		hentry;
	long			start;
	long			end;
};

struct suffix {
        struct sfxnode *        node;
        long                    start;
        long                    end;
};

struct sfxedgekey {
        struct sfxnode *        node;
        unsigned short          character;
};

struct sfxtree {
	struct memsys		sys;
	struct raw		str;
	struct htab		edges;
	struct sfxnode		root;
};


int  sfx_init(struct sfxtree *sfx, struct raw *str, struct memsys *sys);
int  sfx_match(struct sfxtree *sfx, struct raw *pat, unsigned long *off);
void sfx_clear(struct sfxtree *sfx);
struct sfxnode *sfx_next(struct sfxtree *t, struct sfxnode *cur, int ch);


#endif /* __match_h */
