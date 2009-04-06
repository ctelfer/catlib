#include <cat/emit.h>
#if CAT_USE_STDLIB
#include <string.h>
#else /* CAT_USE_STDLIB */
#include <cat/catstdlib.h>
#endif /* CAT_USE_STDLIB */


int emit_char(struct emitter *em, char ch)
{
  abort_unless(em && em->emit_func);
  if ( em->emit_state != EMIT_OK )
    return -1;
  return (*em->emit_func)(em, &ch, sizeof(char));
}


int emit_raw(struct emitter *em, const void *buf, size_t len)
{
  abort_unless(em && em->emit_func);
  if ( em->emit_state != EMIT_OK )
    return -1;
  if ( len > INT_MAX )
    return -1;
  return (*em->emit_func)(em, buf, len);
}


int emit_string(struct emitter *em, const char *s)
{
  size_t len;
  abort_unless(s);
  len = strlen(s);
  return emit_raw(em, s, len);
}


static int null_emit_func(struct emitter *em, const void *buf, size_t len)
{
  return 0;
}


int init_null_emitter(struct emitter *em)
{
  abort_unless(em);
  em->emit_state = EMIT_OK;
  em->emit_func = null_emit_func;
}


struct emitter null_emitter = { EMIT_OK, &null_emit_func };


static int str_emit_func(struct emitter *em, const void *buf, size_t len)
{
  struct string_emitter *se;
  size_t tocopy;
  se = (struct string_emitter *)em;

  abort_unless(se->se_raw.len >= se->se_fill);
  tocopy = len;
  if ( tocopy > se->se_raw.len - se->se_fill )
    tocopy = se->se_raw.len - se->se_fill;

  memmove(se->se_raw.data + se->se_fill, buf, tocopy);
  se->se_fill += tocopy;

  if ( se->se_raw.len == se->se_fill )
    se->se_emitter.emit_state = EMIT_EOS;

  return 0;
}


void string_emitter_init(struct string_emitter *se, char *str, size_t len)
{
  struct emitter *e;
  abort_unless(se);
  abort_unless(str && len >= 1);

  e = &se->se_emitter;
  e->emit_state   = EMIT_OK;
  e->emit_func    = str_emit_func;
  se->se_fill     = 0;
  se->se_raw.data = str;
  se->se_raw.len  = len - 1;
}


void string_emitter_terminate(struct string_emitter *se)
{
  abort_unless(se && se->se_fill <= se->se_raw.len);
  se->se_raw.data[se->se_fill] = '\0';
  se->se_emitter.emit_state = EMIT_EOS;
}


