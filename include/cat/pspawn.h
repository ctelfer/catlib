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
	/* input / output flags */
	PSFD_IN      = 0x0001, /* input to child */
	PSFD_OUT     = 0x0002, /* output from child */
	PSFD_INOUT   = 0x0003, /* both input and output to child */
	PSFD_DMASK   = 0x0003, /* mask i/o bits */

	/* modifier flags */
	PSFD_STDIO   = 0x0004, /* generate stdio FILE for file descriptor */
	PSFD_APPEND  = 0x0008, /* open in append-mode (output only) */

	/* stdio buffering flags: set only if PSFD_STDIO is also set */
	PSFD_FULLBUF = 0x0010, /* fully bufferd stdio */
	PSFD_NOBUF   = 0x0020, /* non-buffered stdio */
	PSFD_LINEBUF = 0x0030, /* line buffered stdio */
	PSFD_BMASK   = 0x0030, /* stdio buffering mask */

	/* internal flags, do not set */
	/* if neither REMAP nor FSRDR is set, preserve the fd in the child */
	PSFD_REMAP   = 0x0200, /* re-map fd from parent in child process */
	PSFD_FSRDR   = 0x0400, /* redirect fd from file system object */
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
};

/* initilize a process spawn specification */
void ps_spec_init(struct ps_spec *spec);

/* clean up a process spawn specification */
void ps_spec_cleanup(struct ps_spec *spec);

/* copy a process spawn specification */
void ps_spec_copy(struct ps_spec *dst, const struct ps_spec *src);



/* ------------- 
   The steps for setting up I/O for the process are as follows 

   1) specify which file descriptors from the parent to keep but not remap
      (ps_spec_keepfd() for each)

   2) specify which file descriptors from the parent to remap
      (ps_spec_add_remap() for each)
      + for the fde returned from each ps_spec_add_remap() call:
        - for each fd that the child should have this parent fd mapped to:
	  ps_fde_addfd() for that target fd

   3) specify which file descriptors to have as pipes/sockets to/from/both
      the parent
      + ps_spec_add_pipe() for each specifying
        - whether input, output or both
	- whether the server side should use stdio
	- what buffering mode to use if using stdio
      + for each fde returned from ps_spec_add_pipe()
        - for each fd that the child should have this pipe mapped to:
	  ps_fde_addfd() for the target fd

   4) specify which file descriptors to redirect from/to a file system
      object (such as a file)
      + ps_spec_add_fsrdr() for each specifying
        - whether to open for input / output or both
	- whether to open for appending (output-only)
      + for each fde returned from ps_spec_add_fsrdr()
        - for each fd that the child should have this file mapped to:
	  ps_fde_addfd() for the target fd
	- if the mode should be changed call ps_rdr_set_mode()
   ------------- */

/* specify to add a pipe to the child process (uni- or bi-directional) */
struct ps_fd_entry *ps_spec_add_pipe(struct ps_spec *spec, int type);

/* specify to map fd in the current process to something else in the child */
struct ps_fd_entry *ps_spec_add_remap(struct ps_spec *spec, int fd);

/* specify to preserve (not close) fd in the child process */
struct ps_fd_entry *ps_spec_keepfd(struct ps_spec *spec, int fd);

/* specify to open file system object "path" in the child process */
struct ps_fd_entry *ps_spec_add_fsrdr(struct ps_spec *spec, int type, 
				      const char *path);

/* set access permissions for a filesystem redirection object */
void ps_rdr_set_mode(struct ps_fd_entry *psfde, unsigned int mode);



/* delete a file descriptor entry from a specification */
void ps_spec_del_fde(struct ps_spec *spec, struct ps_fd_entry *psfde);

/* add a file descriptor to a file descriptor entry.  This specifies */
/* which file descriptor the child process will have for this entry */
void ps_fde_addfd(struct ps_fd_entry *psfde, int fd);

/* delete a child file descriptor from a file descriptor entry */
void ps_fde_delfd(struct ps_fd_entry *psfde, int fd);


/*
 * struct ps_spec psf;
 * struct ps_fd_entry *psfde;
 *
 * ps_spec_init(&psf);
 * ps_fde_addfd(ps_spec_add(&psf, PSFD_STDIO|PSFD_IN|PSFD_LINEBUF), 0);
 * ps_fde_addfd(ps_spec_add(&psf, PSFD_STDIO|PSFD_OUT|PSFD_FULLBUF), 1);
 * ps_fde_addfd(ps_spec_add(&psf, PSFD_STDIO|PSFD_OUT|PSFD_NOBUF), 2);
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


/* ignore sigcld signals:  this should be called before ps_launch or */
/* ps_run_std() unless your code handles signals itself (which is fine) */
/* this function is just for convenience: signal(SIGCLD, SIG_IGN); */
void ps_ignore_sigcld(); 

/* spawn co-process trapping i/o as per flags */
/* note that unless handling signals in your program yourself you should */
/* call ps_ignore_sigcld() (only once is necessary) before calling ps_launch */
struct pspawn * ps_launch(char * const argv[], char * const envp[], 
			  struct ps_spec *spec);

/* get the local fd that corresponds to the remote fd specified */
/* return -1 if remfd is not specified */
int  ps_get_locfd(struct pspawn *ps, int remfd);

/* get the local FILE entry that corresponds to the remote fd specified */
/* this may well be NULL if that fd isn't specified or if the PSFD_USE_STDIO */
/* was not set */
FILE *ps_get_locfile(struct pspawn *ps, int remfd);

/* close the local file/fd for the process:  using this API for closing is */
/* CRUCIAL because there is otherwise no way to close the file safely */
/* without a ps_cleanup() */
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
/* ex modes: "012", "e0", "n1", "el0n1+2", "1+2", "12"... */ 
/* if the mode starts with 'e', then the first arg must be the */
/* environment array to use.  Otherwise it wipes the current environment */
/* 'n' stands for non-blocking stdio and 'l' stands for line-buffered */
/* 1+2 means redirect stdout and stderr to the same stream */
/* note that unless handling signals in your program yourself you should */
/* call ps_ignore_sigcld() (only once is necessary) before calling ps_launch */
struct pspawn *ps_run_std(const char *mode, ...);


#endif /* CAT_HAS_POSIX */
#endif /* __spawn_h */
