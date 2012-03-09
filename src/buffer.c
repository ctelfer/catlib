#include "cat/buffer.h"
#include <stdlib.h>
#include <string.h>


#if CAT_USE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

static INLINE void bb_sanity(struct bbuf *b)
{
	abort_unless(b && b->mm != NULL &&
		     (b->off < b->size) && 
		     (b->size - b->off <= b->len) && 
		     (b->data != NULL || b->size == 0));
}


void bb_init(struct bbuf *b, struct memmgr *mm)
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


int bb_alloc(struct bbuf *b, size_t sz)
{
	byte_t *p;

	bb_sanity(b);

	if (sz <= b->size)
		return 0;

	/* always at least double size if possible to */
	/* make the max # of allocations the ceiling of lg2 of SIZE_MAX */
	if ((b->size < ((size_t)-1) / 2) && (sz < b->size * 2))
		sz = b->size * 2;

	p = mem_resize(b->mm, b->data, sz);
	if (p == NULL)
		return -1;

	b->data = p;
	b->size = sz;

	return 0;
}


void bb_free(struct bbuf *b)
{
	bb_sanity(b);

	mem_free(b->mm, b->data);
	bb_init(b, b->mm);
}


int bb_cat(struct bbuf *b, void *p, size_t len)
{
	bb_sanity(b);

	if (b->size - b->off - b->len < len)
		return -1;
	memmove(b->data + b->off + b->len, p, len);
	b->len += len;

	return 0;
}


int bb_cat_a(struct bbuf *b, void *p, size_t len)
{
	bb_sanity(b);

	if (b->size - b->off - b->len < len) {
		if (((size_t)-1) - b->off - b->len < len)
			return -1;
		if (bb_alloc(b, b->off + b->len + len) < 0)
			return -1;
	}
	memmove(b->data + b->off + b->len, p, len);
	b->len += len;
	return 0;
}


int bb_set(struct bbuf *b, size_t off, void *p, size_t len)
{
	bb_sanity(b);

	if (b->size < off || b->size - off < len)
		return -1;
	b->off = off;
	b->len = len;
	memmove(b->data + off, p, len);

	return 0;
}


int bb_set_a(struct bbuf *b, size_t off, void *p, size_t len)
{
	size_t tlen;

	bb_sanity(b);

	if (((size_t)-1) - off > len)
		return -1;

	tlen = off + len;
	if (b->size < tlen) {
		if (bb_alloc(b, tlen) < 0)
			return -1;
	}
	b->off = off;
	b->len = len;
	memmove(b->data + off, p, len);

	return 0;
}


int bb_copy(struct bbuf *db, struct bbuf *sb)
{
	bb_sanity(db);
	bb_sanity(sb);

	if (bb_alloc(db, db->size) < 0)
		return -1;


	db->off = sb->off;
	db->len = sb->len;
	memmove(db->data + db->off, sb->data + sb->off, sb->len);

	return 0;
}

