#include <cat/cat.h>
#include <cat/stdclio.h>

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


static int file_charport_readchar(struct charport *cp, char *ch)
{
  struct file_charport *fcp = (struct file_charport *)cp;
  int rch;
  abort_unless(fcp->file);
  if ( feof(fcp->file) )
    return READCHAR_END;
  if ( ferror(fcp->file) )
    return READCHAR_ERROR;
  rch = fgetc(fcp->file);
  if ( rch == EOF ) {
    if ( feof(fcp->file) )
      return READCHAR_END;
    if ( ferror(fcp->file) )
      return READCHAR_ERROR;
  }
  *ch = rch;
  return READCHAR_CHAR;
}


void file_charport_init(struct file_charport *fcp, FILE *fp)
{
  abort_unless(fcp && fp);
  fcp->cp.read = &file_charport_readchar;
  fcp->file = fp;
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


static int fd_charport_readchar(struct charport *cp, char *ch)
{
  int rv;
  struct fd_charport *fdcp = (struct fd_charport *)cp;
  abort_unless(fdcp->fd >= 0);
  rv = read(fdcp->fd, ch, 1);
  if ( rv == 1 )
    return READCHAR_CHAR;
  if ( rv == 0 )
    return READCHAR_END;
  return READCHAR_ERROR;
}


void fd_charport_init(struct fd_charport *fdcp, int fd)
{
  abort_unless(fdcp && (fd >= 0));
  fdcp->cp.read = &fd_charport_readchar;
  fdcp->fd = fd;
}

#endif /* CAT_HAS_POSIX */
