#ifndef __cat_emit_file_h
#define __cat_emit_file_h
#include <cat/cat.h>
#include <cat/emit.h>

#if CAT_USE_STDLIB
#include <stdio.h>
#else /* CAT_USE_STDLIB */
#include <cat/catstdio.h>
#endif /* CAT_USE_STDLIB */

struct file_emitter {
  struct emitter	fe_emitter;
  FILE *		fe_file;
};

void file_emitter_init(struct file_emitter *fe, FILE *file);
int file_emitter_open(struct file_emitter *fe, const char *fname, int append);
int file_emitter_close(struct file_emitter *fe);
int file_emit_func(struct emitter *em, const void *buf, size_t len);


#endif /* __cat_emit_file_h */
