#include <cat/cat.h>
#include <cat/emit_file.h>

#if CAT_USE_STDLIB
#include <stdio.h>
#else /* CAT_USE_STDLIB */
#include <cat/catstdio.h>
#endif /* CAT_USE_STDLIB */


int file_emit_func(struct emitter *em, const void *buf, size_t len)
{
  struct file_emitter *fe = (struct file_emitter *)em;
  size_t rv;

  if ( len == 0 )
    return 0;

  rv = fwrite(buf, len, 1, fe->fe_file);
  if ( ferror(fe->fe_file) ) {
    fe->fe_emitter.emit_state = EMIT_ERR;
    return -1;
  }

  return 0;
}


void file_emitter_init(struct file_emitter *fe, FILE *file)
{
  struct emitter *e;
  abort_unless(fe && file);

  e = &fe->fe_emitter;
  e->emit_state = EMIT_OK;
  e->emit_func = file_emit_func;
}


int file_emitter_open(struct file_emitter *fe, const char *fname, int append)
{
  FILE *fp;
  abort_unless(fe);
  if ( (fp = fopen(fname, (append ? "a" : "r"))) == NULL )
    return -1;
  file_emitter_init(fe, fp);
  return 0;
}


int file_emitter_close(struct file_emitter *fe)
{
  abort_unless(fe && fe->fe_file);
  return fclose(fe->fe_file);
}

