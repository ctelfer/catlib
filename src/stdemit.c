#include <cat/cat.h>
#include <cat/stdemit.h>

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
  fe->fe_file = file;
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


#if CAT_HAS_POSIX
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

static int fd_emit_func(struct emitter *ectx, const void *buf, size_t len)
{
  size_t nwritten = 0;
  ssize_t rv;
  int fd = ((struct fd_emitter *)ectx)->fde_fd;
  const unsigned char *data = buf;

  while ( nwritten < len ) {
    rv = write(fd, data + nwritten, len - nwritten);
    if ( rv == -1 ) {
      if ( errno == EINTR )
        continue;
      ectx->emit_state = EMIT_ERR;
      return -1;
    } else {
      nwritten += (size_t)rv;
    }
  } 
  return 0;
}


void fd_emitter_init(struct fd_emitter *fde, int fd)
{
  struct emitter *e;
  abort_unless(fde);
  abort_unless(fd >= 0);

  e = &fde->fde_emitter;
  e->emit_state = EMIT_OK;
  e->emit_func  = fd_emit_func;
  fde->fde_fd   = fd;
}

#endif /* CAT_HAS_POSIX */
