#ifndef __cat_emit_h
#define __cat_emit_h

#include <cat/cat.h>

enum {
	EMIT_OK = 0,
	EMIT_EOS = 1,
	EMIT_ERR = -1
};

struct emitter;

typedef int  (*emit_f)(struct emitter *ectx, const void *buf, size_t len);

struct emitter {
	int 		emit_state;
	emit_f		emit_func;
};

int emit_raw(struct emitter *em, const void *buf, size_t len);
int emit_char(struct emitter *em, char ch);
int emit_string(struct emitter *em, const char *s);

extern struct emitter null_emitter;




/* String Emitter - emit to a fixed length string */
struct string_emitter {
	struct emitter	se_emitter;
	struct raw	se_raw;
	size_t		se_fill;
};

void string_emitter_init(struct string_emitter *se, char *buf, size_t len);
void string_emitter_terminate(struct string_emitter *se);




#if CAT_HAS_POSIX
/* File Descriptor Emitter - emit to a Posix file descriptor */
struct fd_emitter {
	struct emitter	fde_emitter;
	int		fde_fd;
};

void fd_emitter_init(struct fd_emitter *fde, int fd);
#endif /* CAT_HAS_POSIX */


#endif /* __cat_emit_h */


