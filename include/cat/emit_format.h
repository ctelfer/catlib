#ifndef __cat_emit_format_h
#define __cat_emit_format_h

#include <cat/cat.h>
#include <cat/emit.h>
#include <stdarg.h>

int emit_format(struct emitter *em, const char *fmt, ...);
int emit_vformat(struct emitter *em, const char *fmt, va_list ap);

#endif /* __cat_emit_format_h */
