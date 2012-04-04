#include "cat/buffer.h"
#include <stdlib.h>
#include <string.h>


#if CAT_USE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

static INLINE void dyb_sanity(struct dynbuf *b)
{
	abort_unless(b && b->mm != NULL &&
		     (b->off <= b->size) && 
		     (b->size - b->off >= b->len) && 
		     (b->data != NULL || b->size == 0));
}


void dyb_init(struct dynbuf *b, struct memmgr *mm)
{
	b->size = 0;
	b->data = NULL;
	b->off = 0;
	b->len = 0;
	if (mm == NULL)
		b->mm = &stdmm;
	else
		b->mm = mm;
}


int dyb_resv(struct dynbuf *b, ulong sz)
{
	byte_t *p;

	dyb_sanity(b);

	if (sz <= b->size)
		return 0;

	/* always at least double size if possible to */
	/* make the max # of allocations the ceiling of lg2 of SIZE_MAX */
	if ((b->size < ((ulong)-1) / 2) && (sz < b->size * 2))
		sz = b->size * 2;

	p = mem_resize(b->mm, b->data, sz);
	if (p == NULL)
		return -1;

	b->data = p;
	b->size = sz;

	return 0;
}


void dyb_clear(struct dynbuf *b)
{
	dyb_sanity(b);

	mem_free(b->mm, b->data);
	dyb_init(b, b->mm);
}


void *dyb_release(struct dynbuf *b)
{
	void *p;
	dyb_sanity(b);

	p = b->data;
	b->data = NULL;
	b->size = 0;
	b->len = 0;
	b->off = 0;
	return p;
}


void dyb_empty(struct dynbuf *b)
{
	dyb_sanity(b);
	b->len = 0;
	b->off = 0;
}


int dyb_cat(struct dynbuf *b, void *p, ulong len)
{
	dyb_sanity(b);

	if (b->size - b->off - b->len < len)
		return -1;
	memmove(b->data + b->off + b->len, p, len);
	b->len += len;

	return 0;
}


int dyb_cat_a(struct dynbuf *b, void *p, ulong len)
{
	dyb_sanity(b);

	if (b->size - b->off - b->len < len) {
		if (((ulong)-1) - b->off - b->len < len)
			return -1;
		if (dyb_resv(b, b->off + b->len + len) < 0)
			return -1;
	}
	memmove(b->data + b->off + b->len, p, len);
	b->len += len;
	return 0;
}


int dyb_set(struct dynbuf *b, ulong off, void *p, ulong len)
{
	dyb_sanity(b);

	if (b->size < off || b->size - off < len)
		return -1;
	b->off = off;
	b->len = len;
	memmove(b->data + off, p, len);

	return 0;
}


int dyb_set_a(struct dynbuf *b, ulong off, void *p, ulong len)
{
	ulong tlen;

	dyb_sanity(b);

	if (((ulong)-1) - off > len)
		return -1;

	tlen = off + len;
	if (b->size < tlen) {
		if (dyb_resv(b, tlen) < 0)
			return -1;
	}
	b->off = off;
	b->len = len;
	memmove(b->data + off, p, len);

	return 0;
}


int dyb_copy(struct dynbuf *db, struct dynbuf *sb)
{
	dyb_sanity(db);
	dyb_sanity(sb);

	if (dyb_resv(db, db->size) < 0)
		return -1;


	db->off = sb->off;
	db->len = sb->len;
	memmove(db->data + db->off, sb->data + sb->off, sb->len);

	return 0;
}

