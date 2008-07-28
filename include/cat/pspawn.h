#ifndef __spawn_h
#define __spawn_h
#include <cat/cat.h>

#ifdef CAT_HAS_POSIX

#include <stdio.h>
#include <signal.h>
#include <cat/list.h>

/* rules:
    - if type is 'preserve' no other bits must be set
    - if type contains 'fsrdr' then no buffer flags, or stdio can be set
    - 'append' can only be set on if 'filerdr' is set and 'out' is set
    - direction bits must be non-zero (unless type == 'preserve') 
    - buffer bits must be non-zero if STDIO flag is set 
 */
enum {
	PSFD_IN      = 0x0001,
	PSFD_OUT     = 0x0002,
	PSFD_INOUT   = 0x0003,
	PSFD_DMASK   = 0x0003,
	PSFD_FULLBUF = 0x0010,
	PSFD_NOBUF   = 0x0020, 
	PSFD_LINEBUF = 0x0030, 
	PSFD_BMASK   = 0x0030, 
	PSFD_STDIO   = 0x0100,
	PSFD_REMAP   = 0x0200,
	PSFD_FSRDR   = 0x0400,
	PSFD_APPEND  = 0x0800
};

struct ps_fd_entry {
	int 		locfd;
	int		pipefd;
	struct list	*remfds;
	int		type;
	FILE *		file;
	const char *	path;
	unsigned int	mode;
};

struct ps_spec {
	struct list *fdelist;
	int replace_sigcld;
};

void ps_spec_init(struct ps_spec *spec);
void ps_spec_cleanup(struct ps_spec *spec);
void ps_spec_copy(struct ps_spec *dst, const struct ps_spec *src);
struct ps_fd_entry *ps_spec_add_pipe(struct ps_spec *spec, int type);
struct ps_fd_entry *ps_spec_add_remap(struct ps_spec *spec, int fd);
struct ps_fd_entry *ps_spec_keepfd(struct ps_spec *spec, int fd);
struct ps_fd_entry *ps_spec_add_fsrdr(struct ps_spec *spec, int type, 
				      const char *path);
void ps_rdr_set_mode(struct ps_fd_entry *psfde, unsigned int mode);
void ps_spec_del_fde(struct ps_spec *spec, struct ps_fd_entry *psfde);
void ps_fde_addfd(struct ps_fd_entry *psfde, int fd);
void ps_fde_delfd(struct ps_fd_entry *psfde, int fd);

/*
 * struct ps_spec psf;
 * struct ps_fd_entry *psfde;
 *
 * ps_spec_init(&psf);
 * ps_fde_addfd(ps_spec_add(&psf, PSFD_STDIO|PSFD_IN|PSFD_LINEBUF), 0);
 * ps_fde_addfd(ps_spec_add(&psf, PSFD_STDIO|PSFD_OUT|PSFD_FULLBUF),1);
 * ps_fde_addfd(ps_spec_add(&psf, PSFD_STDIO|PSFD_OUT|PSFD_NOBUF),2);
 */


enum {
	PSS_INIT,
	PSS_LAUNCHED,
	PSS_FINISHED
};

struct pspawn {
	int		state;
	int		exit_status;
	void		(*o_sigcld)(int sig);
	int		error;
	pid_t		cpid;
	struct ps_spec	spec;
};



/* spawn co-process trapping i/o as per flags */
struct pspawn * ps_launch(char * const argv[], char * const envp[], 
			  struct ps_spec *spec);

/* get the local FD that corresponds to the remote fd specified */
/* return -1 if not specified */
int  ps_get_locfd(struct pspawn *ps, int remfd);

/* get the local FILE entry that corresponds to the remote fd specified */
/* this may well be NULL if that fd isn't specified or if the PSFD_USE_STDIO */
/* was not set */
FILE *ps_get_locfile(struct pspawn *ps, int remfd);

/* close the local file/fd for the process:  using this API for closing is */
/* CRUCIAL because there is otherwise no way to close the file safely w/out */
/* a ps_cleanup() */
void ps_closeio(struct pspawn *ps, int remfd);

/* return true of the process is still running */
int ps_running(struct pspawn *ps);

/* clean up and free data structure.  optionally wait for the process to end. */
/* return the exit status.  N.B. if wait == 0, then you must not have replaced*/
/* the signal handler and you must have a sigcld handler installed.  Otherwise*/
/* the child exiting will kill the process */
int ps_cleanup(struct pspawn *ps, int wait);


/* -- utility functions based on previous library calls -- */

/* mode format: /e?([nl]?0)?([nl]?1)?(([nl]?2)|+2)?/ */
/*ex modes: "123", "e1", "n2", "el1n2+3", "2+3", "23"... */ 
/* if the mode starts with 'e', then the first arg must be the */
/* environment array to use.  Otherwise it wipes the current environment */
struct pspawn *ps_run_std(const char *mode, ...);


#endif /* CAT_HAS_POSIX */
#endif /* __spawn_h */
