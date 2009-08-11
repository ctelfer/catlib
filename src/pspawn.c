#include <cat/cat.h>

#if CAT_HAS_POSIX

#define DBG fprintf(stderr, "DBG:%s:%d\n", __FILE__, __LINE__);

#ifndef CAT_PS_MAXFDS
#define CAT_PS_MAXFDS 1024
#endif

#include <cat/pspawn.h>
#include <cat/cds.h>
#include <cat/bitset.h>
#include <cat/io.h>

#include <stdarg.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

/* data structures */
CDS_NEWSTRUCT(struct list, int, fdnode_s);
#define ln_to_fd(node)	CDS_DATA(node, fdnode_s)
#define ln_to_fde(node)	container(node, struct ps_fd_entry, entry)


/* Helper functions */
static struct ps_fd_entry *ps_spec_newfde(struct ps_spec *spec, int type);
static int ps_fds_type_ok(int type);
static void ps_fd_entry_cleanup(struct ps_fd_entry *psfde);
static struct ps_fd_entry *ps_fde_find(struct pspawn *ps, int remfd);
static int spec_ok(struct ps_spec *spec);
static int create_pipes(struct pspawn *ps);
static int open_stdio_files(struct pspawn *ps);
static int open_rdr_files(struct pspawn *ps);
static int set_up_fds(struct pspawn *ps, int *retfd);
static int set_up_fd(struct pspawn *ps, bitset_t *pipefds, int tfd, int sfd, 
		     int *retfd, int *nomap);
static void ps_launch_child(struct pspawn *ps, int pipefds[2],
		            char * const *argv, char * const *envp);
static int ps_launch_parent(struct pspawn *ps, int pipefds[2]);

 
void ps_ignore_sigcld()
{
	signal(SIGCHLD, SIG_IGN); 
}


void ps_spec_init(struct ps_spec *spec)
{
	l_init(&spec->fdelist);
}


void ps_spec_cleanup(struct ps_spec *spec)
{
	struct list *node;
	struct ps_fd_entry *psfde;
	while ( (node = l_deq(&spec->fdelist)) != NULL ) {
		psfde = ln_to_fde(node);
		ps_fd_entry_cleanup(psfde);
		free(psfde);
	}
}


static void ps_fd_entry_cleanup(struct ps_fd_entry *e)
{
	struct list *node;
	if ( e->file != NULL ) {
		fclose(e->file);
		e->file = NULL;
	} else if ( e->locfd >= 0 ) {
		close(e->locfd);
	}
	if ( (e->pipefd >= 0) && (e->type != PSFD_REMAP) )
		close(e->pipefd);
	e->locfd = -1;
	e->pipefd = -1;

	while ( (node = l_deq(&e->remfds)) != NULL )
		CDS_FREE(node);
}


int ps_spec_copy(struct ps_spec *dst, const struct ps_spec *src)
{
	struct list *t, *t2;
	struct ps_fd_entry *old, *new;
	struct fdnode_s *fdn;
	abort_unless(dst != NULL);
	abort_unless(src != NULL);

	ps_spec_init(dst);
	l_for_each(t, &src->fdelist) {
		old = ln_to_fde(t);
		if ( (new = ps_spec_newfde(dst, old->type)) == NULL )
			goto err;
		new->pipefd = old->pipefd;
		new->locfd = old->locfd;
		l_for_each(t2, &old->remfds) {
			CDS_NEW(fdn, ln_to_fd(t2));
			if ( fdn == NULL )
				goto err;
			l_enq(&new->remfds, &fdn->node);
		}
	}

	return 0;

err:
	ps_spec_cleanup(dst);
	return -1;
}


static struct ps_fd_entry *ps_spec_newfde(struct ps_spec *spec, int type)
{
	struct ps_fd_entry *psfde;

	psfde = malloc(sizeof(struct ps_fd_entry));
	if ( psfde == NULL )
		return NULL;
	psfde->locfd = -1;
	psfde->pipefd = -1;
	l_init(&psfde->remfds);
	psfde->type = type;
	psfde->file = NULL;
	psfde->path = NULL;
	psfde->mode = 0644;
	l_enq(&spec->fdelist, &psfde->entry);
	return psfde;
}


struct ps_fd_entry *ps_spec_add_pipe(struct ps_spec *spec, int type)
{
	if ( !ps_fds_type_ok(type) || 
	     ((type & PSFD_FSRDR) != 0) || 
	     ((type & PSFD_REMAP) != 0) ) {
		errno = EINVAL;
		return NULL;
	}

	return ps_spec_newfde(spec, type);
}


struct ps_fd_entry *ps_spec_add_remap(struct ps_spec *spec, int fd)
{
	struct ps_fd_entry *psfde;

	if ( fd < 0 ) {
		errno = EINVAL;
		return NULL;
	}
		
	if ( (psfde = ps_spec_newfde(spec, PSFD_REMAP)) != NULL )
		psfde->pipefd = fd;
	return psfde;
}


struct ps_fd_entry *ps_spec_keepfd(struct ps_spec *spec, int fd)
{
	struct ps_fd_entry *psfde = ps_spec_add_remap(spec, fd);
	if ( psfde != NULL )
		ps_fde_addfd(psfde, fd);
	return psfde;
}


struct ps_fd_entry *ps_spec_add_fsrdr(struct ps_spec *spec, int type, 
				      const char *path)
{
	struct ps_fd_entry *psfde;

	if ( (path == NULL) || (*path == '\0') ) {
		errno = EINVAL;
		return NULL;
	}

	type |= PSFD_FSRDR;
	if ( !ps_fds_type_ok(type) ) {
		errno = EINVAL;
		return NULL;
	}

	if ( (psfde = ps_spec_newfde(spec, type)) != NULL )
		psfde->path = path;
	return psfde;
}


void ps_rdr_set_mode(struct ps_fd_entry *psfde, uint mode)
{
	if ( (psfde == NULL) || ((psfde->type & PSFD_FSRDR) != 0) )
		return;
	psfde->mode = mode;
}


/* rules:
	- if type is 'remap' no other bits must be set
	- if type contains 'fsrdr' then no buffer flags, or stdio can be set
	- 'append' can only be set on if 'filerdr' is set and dir is 'out'
	- direction bits must be non-zero (unless type == 'preserve')
	- buffer bits must be non-zero if STDIO flag is set 
 */
static int ps_fds_type_ok(int type) 
{
	if ( type == PSFD_REMAP )
		return 1;
	if ( (type & PSFD_REMAP) != 0 )
		return 0;
	if ( ((type & PSFD_FSRDR) != 0) && 
			 ((type & (PSFD_STDIO|PSFD_BMASK))  != 0) )
		return 0;
	if ( (((type & PSFD_FSRDR) == 0) || ((type &PSFD_DMASK) != PSFD_OUT)) &&
			 ((type & (PSFD_APPEND)) != 0) )
		return 0;
	if ( (type & PSFD_DMASK) == 0 )
		return 0;
	if ( ((type & PSFD_STDIO) != 0) && ((type & PSFD_BMASK) == 0) )
		return 0;
	return 1;
}


void ps_spec_del_fde(struct ps_spec *spec, struct ps_fd_entry *psfde)
{
	ps_fd_entry_cleanup(psfde);
	l_rem(&psfde->entry);
	free(psfde);
}


int ps_fde_addfd(struct ps_fd_entry *psfde, int fd)
{
	struct fdnode_s *fdn;
	CDS_NEW(fdn, fd);
	if ( fdn == NULL )
		return -1;
	l_enq(&psfde->remfds, &fdn->node);
	return 0;
}


void ps_fde_delfd(struct ps_fd_entry *psfde, int fd)
{
	struct list *t, *h, *remfdl = &psfde->remfds;
	t = l_head(remfdl);
	while ( t != l_end(remfdl) ) {
		h = t;
		t = t->next;
		if ( ln_to_fd(h) == fd ) {
			l_rem(h);
			CDS_FREE(h);
		}
	}
}


struct pspawn * ps_launch(char * const argv[], char * const envp[], 
		          struct ps_spec *spec)
{
	struct pspawn *ps;
	int retpipe[2] = { -1, -1 };
	int rerrno = 0;
	pid_t pid;

	if ( (argv == NULL) || (*argv == NULL) || !spec_ok(spec) ) {
		errno = EINVAL;
		return NULL;
	}

	if ( (ps = calloc(1, sizeof(*ps))) == NULL )
		return NULL;
	ps->cpid = -1;
	if ( ps_spec_copy(&ps->spec, spec) < 0 ) {
		rerrno = errno;
		goto err;
	}
	ps->state = PSS_INIT;

	if ( pipe(retpipe) < 0 ) {
		rerrno = errno;
		goto err;
	}

	if ( create_pipes(ps) < 0 ) {
		rerrno = ps->error;
		goto err;
	}

	if ( open_stdio_files(ps) < 0 ) {
		rerrno = ps->error;
		goto err;
	}

	pid = fork();
	if ( pid < 0 ) {
		rerrno = errno;
		goto err;
	} else if ( pid == 0 ) {
		ps_launch_child(ps, retpipe, argv, envp);  /* does not return */
	} else {
		ps->cpid = pid;
		close(retpipe[1]);
		retpipe[1] = -1;
		if ( ps_launch_parent(ps, retpipe) < 0 ) {
			rerrno = ps->error;
			goto err;
		}
		close(retpipe[0]);
	}
	ps->state = PSS_LAUNCHED;

	return ps;

err:
	if ( retpipe[0] >= 0 )
		close(retpipe[0]);
	if ( retpipe[1] >= 0 )
		close(retpipe[1]);
	ps_spec_cleanup(&ps->spec);
	free(ps);
	errno = rerrno;
	return NULL;
}


static int spec_ok(struct ps_spec *spec)
{
	struct list *t, *t2;
	struct ps_fd_entry *psfde;
	DECLARE_BITSET(mapped, CAT_PS_MAXFDS);

	bset_zero(mapped, CAT_PS_MAXFDS);

	l_for_each(t, &spec->fdelist) {
		psfde = ln_to_fde(t);
		if ( !ps_fds_type_ok(psfde->type) ) 
			goto fail;
		if ( l_isempty(&psfde->remfds) )
			goto fail;
		l_for_each(t2, &psfde->remfds) {
			int fd = ln_to_fd(t2);
			if ( (fd < 0) || (fd >= CAT_PS_MAXFDS) )
				goto fail;
			if ( bset_test(mapped, fd) )
				goto fail;
			bset_set(mapped, fd);
		}
	}

	return 1;

fail:
	return 0;
}


static int create_pipes(struct pspawn *ps)
{
	struct list *t;
	struct ps_fd_entry *psfde;
	int pipefds[2];

	l_for_each(t, &ps->spec.fdelist) {
		psfde = ln_to_fde(t);
		if ( psfde->type == PSFD_REMAP )
			continue;
		if ( (psfde->type & PSFD_FSRDR) != 0 )
			continue;
		if ( (psfde->type & PSFD_DMASK) == 0 ) {
			ps->error = EINVAL;
			return -1;
		}

		if ( (psfde->type & PSFD_DMASK) == PSFD_INOUT ) {
			if ( socketpair(AF_UNIX,SOCK_STREAM,0,pipefds) < 0 ) {
				ps->error = errno;
				return -1;
			}
		} else {
			if ( pipe(pipefds) < 0 ) {
				ps->error = errno;
				return -1;
			}
		}
		if ( (psfde->type & PSFD_DMASK) == PSFD_IN ) {
			psfde->locfd = pipefds[1];
			psfde->pipefd = pipefds[0];
		} else { 
			psfde->locfd = pipefds[0];
			psfde->pipefd = pipefds[1];
		}
	}

	return 0;
}


static int open_stdio_files(struct pspawn *ps)
{
	struct list *t;
	struct ps_fd_entry *psfde;
	const char *omode = NULL;
	int bmode;
	FILE *fp;

	l_for_each(t, &ps->spec.fdelist) {
		psfde = ln_to_fde(t);
		if ( (psfde->type & PSFD_STDIO) == 0 )
			continue;
		switch (psfde->type & PSFD_DMASK) {
		case PSFD_IN:
			omode = "w";
			break;
		case PSFD_OUT:
			omode = "r";
			break;
		case PSFD_INOUT:
			omode = "r+";
			break;
		default:
			abort_unless(0);
		}

		switch (psfde->type & PSFD_BMASK) {
		case PSFD_FULLBUF:
			bmode = _IOFBF;
			break;
		case PSFD_NOBUF:
			bmode = _IOLBF;
			break;
		case PSFD_LINEBUF:
			bmode = _IONBF;
			break;
		default:
			ps->error = EINVAL;
			return -1;
		}
		
		if ( (fp = fdopen(psfde->locfd, omode)) == NULL ) {
			ps->error = errno;
			return -1;
		}

		if ( bmode != _IOFBF ) { 
			if ( setvbuf(fp, NULL, bmode, 0) < 0 ) { 
				fclose(fp);
				close(psfde->pipefd);
				psfde->locfd = psfde->pipefd = -1;
			}
		}

		psfde->file = fp;
	}

	return 0;
}


static void ps_launch_child(struct pspawn *ps, int retpipe[2], 
		            char * const argv[], char * const envp[])
{
	int rerrno = 0;
	int flags;
	char * const tenv[] = { NULL };

	if ( envp == NULL )
		envp = tenv;

				close(retpipe[0]);
	if ( open_rdr_files(ps) < 0 ) { 
		rerrno = ps->error;
		goto err;
	}

	if ( (flags = fcntl(retpipe[1], F_GETFD)) < 0 ) {
		rerrno = errno;
		goto err;
	}
	flags |= FD_CLOEXEC;
	if ( fcntl(retpipe[1], F_SETFD, flags) < 0 ) {
		rerrno = errno;
		goto err;
	}

	if ( set_up_fds(ps, &retpipe[1]) < 0 ) {
		rerrno = ps->error;
		goto err;
	}

	execve(argv[0], argv, envp);
	rerrno = errno;
err:
	io_write(retpipe[1], &rerrno, sizeof(int));
	exit(129);

}


static int open_rdr_files(struct pspawn *ps)
{
	struct list *t;
	struct ps_fd_entry *psfde;
	int fd;
	int flags;

	l_for_each(t, &ps->spec.fdelist) {
		psfde = ln_to_fde(t);
		if ( (psfde->type & PSFD_FSRDR) == 0 )
			continue;

		if ( (psfde->type & PSFD_DMASK) == PSFD_IN ) {
			flags = O_RDONLY;
		} else if ( (psfde->type & PSFD_DMASK) == PSFD_OUT ) {
			flags = O_WRONLY|O_CREAT;
			if ( (psfde->type & PSFD_APPEND) != 0 )
				flags |= O_APPEND;
			else
				flags |= O_TRUNC;
		} else { 
			flags = O_RDWR|O_CREAT;
		}

		if ( (psfde->type & PSFD_DMASK) == PSFD_IN )
			fd = open(psfde->path, flags);
		else
			fd = open(psfde->path, flags, psfde->mode);
		if ( fd < 0 ) {
			ps->error = errno;
			return -1;
		}
		psfde->pipefd = fd;
	}

	return 0;
}


static int set_up_fds(struct pspawn *ps, int *retfd)
{
	int i, rv, nomap;
	struct list *t, *t2;
	struct ps_fd_entry *psfde;
	DECLARE_BITSET(pipefds, CAT_PS_MAXFDS);

	bset_zero(pipefds, CAT_PS_MAXFDS);
	bset_set(pipefds, *retfd);

	/* populate the set of file descriptors we need to map */
	l_for_each(t, &ps->spec.fdelist) {
		psfde = ln_to_fde(t);
		if ( psfde->locfd >= 0 )
			close(psfde->locfd);
		bset_set(pipefds, psfde->pipefd);
	}

	/* close all descriptors except the remaining pipes */
	for ( i = 0 ; i < CAT_PS_MAXFDS ; i++ )
		if ( !bset_test(pipefds, i) )
			close(i);

	l_for_each(t, &ps->spec.fdelist) {
		nomap = 0;
		psfde = ln_to_fde(t);
		l_for_each(t2, &psfde->remfds) {
			rv = set_up_fd(ps, pipefds, ln_to_fd(t2), 
				       psfde->pipefd, retfd, &nomap);
			if ( rv < 0 ) {
				ps->error = errno;
				goto err;
			}
		}
		if (!nomap) { 
			close(psfde->pipefd);
			psfde->pipefd = -1;
		}
	}

	return 0;
err: 
	return -1;
}


static int set_up_fd(struct pspawn *ps, bitset_t *pipefds, int tfd, 
		     int sfd, int *retfd, int *nomap)
{
	struct list *t;
	struct ps_fd_entry *psfde = NULL;
	int dfd = -1;

	if ( tfd == sfd ) {
		*nomap = 1;
		return 0;
	}

	if ( bset_test(pipefds, tfd) ) {
		/* duplicate the blocking fd */
		if ( (dfd = dup(tfd)) < 0 )
			return -1;
	} 

	if ( dup2(sfd, tfd) < 0 ) {
		if ( dfd >= 0 )
			close(dfd);
		return -1;
	}

	if ( dfd >= 0 ) {
		/* find the entry that was blocking us to update later */
		l_for_each(t, &ps->spec.fdelist) {
			psfde = ln_to_fde(t);
			if ( psfde->pipefd == tfd )
				break;
		}
		if (psfde == NULL) {
			abort_unless(tfd == *retfd);
			*retfd = dfd;
		} else { 
			psfde->pipefd = dfd;
		}
	}

	bset_clr(pipefds, sfd);
	return 0;
}


static int ps_launch_parent(struct pspawn *ps, int retpipe[2])
{
	int rlen, rval;
	struct list *t;
	struct ps_fd_entry *psfde;

	l_for_each(t, &ps->spec.fdelist) {
		psfde = ln_to_fde(t);
		if (psfde->type == PSFD_REMAP)
			continue;
		if (psfde->pipefd >= 0) { 
			close(psfde->pipefd);
			psfde->pipefd = -1;
		}
	}

	if ( (rlen = io_read(retpipe[0], &rval, sizeof(int))) < 0 ) {
		ps->error = errno;
		return -1;
	}
	abort_unless(rlen <= sizeof(int));
	if ( rlen > 0 && rlen < sizeof(int) ) { 
		ps->error = EIO;
		return -1;
	}

	if ( rlen == sizeof(int) ) {
		ps->error = rval;
		return -1;
	}

	return 0;
}


static struct ps_fd_entry *ps_fde_find(struct pspawn *ps, int remfd) 
{
	struct list *t, *t2;
	struct ps_fd_entry *psfde;

	if ( ps == NULL )
		return NULL;

	l_for_each(t, &ps->spec.fdelist) {
		psfde = ln_to_fde(t);
		l_for_each(t2, &psfde->remfds) {
			if ( ln_to_fd(t2) == remfd )
				return psfde;
		}
	}

	return NULL;
}


int ps_get_locfd(struct pspawn *ps, int remfd)
{
	struct ps_fd_entry *psfde = ps_fde_find(ps, remfd);
	return (psfde == NULL) ? -1 : psfde->locfd;
}


FILE *ps_get_locfile(struct pspawn *ps, int remfd)
{
	struct ps_fd_entry *psfde = ps_fde_find(ps, remfd);
	return (psfde == NULL) ? NULL : psfde->file;
}


void ps_closeio(struct pspawn *ps, int remfd)
{
	struct ps_fd_entry *psfde = ps_fde_find(ps, remfd);
	if ( psfde == NULL )
		return;
	if ( psfde->file ) {
		fclose(psfde->file);
		psfde->file = NULL;
	} else if ( psfde->locfd >= 0 ) {
		close(psfde->locfd);
	}
	psfde->locfd = -1;
}


int ps_running(struct pspawn *ps)
{
	pid_t pid;
	int status;

	abort_unless(ps != NULL);
	if ( ps->state != PSS_LAUNCHED )
		return 0;

	pid = waitpid(ps->cpid, &status, WNOHANG);
	if ( pid < 0 ) {
		abort_unless(errno == ECHILD);
		pid = 0;
	}

	if ( pid != 0 ) {
		abort_unless(pid == ps->cpid);
		abort_unless(WIFEXITED(status));
		ps->state = PSS_FINISHED;
		ps->exit_status = WEXITSTATUS(status);
	}

	return ps->state == PSS_LAUNCHED;
}


int ps_cleanup(struct pspawn *ps, int wait)
{
	pid_t pid;
	int status;

	abort_unless(ps != NULL);

	if ( wait && (ps->state == PSS_LAUNCHED) ) {
		do { 
			pid = waitpid(ps->cpid, &status, 0);
			if ( pid > 0 )
				ps->exit_status = WEXITSTATUS(status);
		} while ( (pid < 0) && (errno == EINTR) );
	}

	status = ps->exit_status;
	ps->state = PSS_FINISHED;
	ps_spec_cleanup(&ps->spec);
	free(ps);
	return status;
}


static int modech_ok(int ch)
{
	return (ch >= '0' && ch <= '2') || (ch == '+');
}


#define PS_ARGARR_SIZE	16
struct pspawn *ps_spawn(const char *mode, ...)
{
	va_list ap;
	uint i, narg = 1, type, bmode;
	int minfd = -1, fd;
	char **args = NULL, *argarr[PS_ARGARR_SIZE];
	char * const * envp = NULL;
	int rerrno = 0;
	int keepfds[3] = { 1, 1, 1 };
	struct ps_spec spec;
	struct ps_fd_entry *psfde = NULL;
	struct pspawn *ps = NULL;

	if ( mode == NULL )
		mode = "";

	va_start(ap, mode);
	while ( va_arg(ap, const char *) != NULL )
		++narg;
	va_end(ap);
	if ( *mode == 'e' )
		--narg;
	if ( narg < 2 ) {
		errno = EINVAL;
		return NULL;
	}
	if ( narg > PS_ARGARR_SIZE ) {
		if ( UINT_MAX / sizeof(char *) > narg ) {
			errno = EINVAL;
			return NULL;
		}
		if ( (args = malloc(sizeof(char *) * narg)) == NULL )
			return NULL;
	} else { 
		args = argarr;
	}
	va_start(ap, mode);
	if ( *mode == 'e' ) {
		envp = va_arg(ap, char **);
		++mode;
	}
	for ( i = 0; i < narg; i++ )
		args[i] = va_arg(ap, char *);
	va_end(ap);

	ps_spec_init(&spec);

	while ( *mode != '\0' ) {
		bmode = PSFD_FULLBUF;
		if ( *mode == 'n' ) {
			bmode = PSFD_NOBUF;
			mode++;
		} else if ( *mode == 'l' ) {
			bmode = PSFD_LINEBUF;
			mode++;
		}
		if ( !modech_ok(*mode) ) {
			rerrno = EINVAL;
			goto out;
		}
		if ( *mode == '+' )  {
			if ( bmode != PSFD_FULLBUF || minfd != 1 || 
			     *++mode != '2' ) {
				rerrno = EINVAL;
				goto out;
			}
			fd = 2;
			if ( ps_fde_addfd(psfde, 2) < 0 ) {
				rerrno = errno;
				goto out;
			}
		} else {
			fd = *mode - '0';
			if ( fd <= minfd ) {
				rerrno = EINVAL;
				goto out;
			}
			type = PSFD_STDIO | bmode;
			type |= (fd == 0 ?  PSFD_IN : PSFD_OUT);
			if ( (psfde = ps_spec_add_pipe(&spec, type)) == NULL ) {
				rerrno = errno;
				goto out;
			}
			if ( ps_fde_addfd(psfde, fd) < 0 ) {
				rerrno = errno;
				goto out;
			}
			minfd = fd;
		}
		keepfds[fd] = 0;
		++mode;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( keepfds[i] ) {
			if ( ps_spec_keepfd(&spec, i) == NULL ) {
				rerrno = errno;
				goto out;
			}
		}
	}

	ps = ps_launch(args, envp, &spec);
	if ( ps == NULL ) 
		rerrno = errno;

out:
	if ( args != argarr )
		free(args);
	ps_spec_cleanup(&spec);
	if ( rerrno != 0 )
		errno = rerrno;
	return ps;
}


#endif /* CAT_HAS_POSIX */
