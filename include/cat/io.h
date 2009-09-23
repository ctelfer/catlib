/*
 * cat/io.h -- Input/Output functions
 * 
 * Christopher Adam Telfer
 * 
 * Copyright 1999, 2003 see accompanying license
 * 
 */ 

#ifndef __cat_io_h
#define __cat_io_h

#include <cat/cat.h>

#if CAT_HAS_POSIX
#include <unistd.h>

/* I/O functions */

/*
 * read 'len' bytes from 'fd'.  Keeps trying to read bytes until it gets all
 * 'len' bytes or encounters an end of file or error.  Returns a short count
 * on an end-of-file and a -1 on an error.  Aborts on a parameter error: 
 * (negative fd, NULL buf or negative len).  Note that this function ignores
 * signal interruptions and keeps trying until a full read count completes.
 * This is fine for some applications, but not for others paying attention to
 * signal interruptions.
 */
ssize_t io_read(int fd, void *buf, ssize_t len); 

/* 
 * write 'len' bytes from 'fd'.  Keeps trying to write bytes until it gets all
 * 'len' bytes or encounters an end of file or error.  Returns a short count
 * on an end-of-file and a -1 on an error.  Aborts on a parameter error: 
 * (negative fd, NULL buf or negative len).  Note that this function ignores
 * signal interruptions and keeps trying until a full read count completes.
 * This is fine for some applications, but not for others paying attention to
 * signal interruptions.
 */
ssize_t io_write(int fd, void *buf, ssize_t len); 


/*
 * read up to 'len' bytes from 'fd'.  Returns the number of bytes read or a -1
 * on an error.  Aborts on a parameter error: (negative fd, NULL buf or 
 * negative len).  Note that this function ignores signal interruptions and keeps 
 * trying until read returns 0 or more or an error occurs.  This is fine for some 
 * applications, but not for others paying attention to signal interruptions.
 */
ssize_t io_read_upto(int fd, void *buf, ssize_t len); 

/*
 * write up to 'len' bytes from 'fd'.  Returns the number of bytes read or a -1
 * on an error.  Aborts on a parameter error: (negative fd, NULL buf or 
 * negative len).  Note that this function ignores signal interruptions and keeps 
 * trying until write returns 0 or more or an error occurs.  This is fine for some 
 * applications, but not for others paying attention to signal interruptions.
 */
ssize_t io_write_upto(int fd, void *buf, ssize_t len); 


/* I/O flags for file file descriptors */

/* 
 * Check whether a file desriptor is ready for reading or writing or has a
 * pending exception.  The caller can specify a timeout in seconds to wait
 * for the condition to arrive.  If the timeout is < 0 then the call will
 * wait indefinitely.  If the timeout is 0.0, the call will return immediately.
 * Returns -1 on an error, 1 if the condition is fulfilled and 0 if the
 * condition isn't satisfied.  Aborts on negative file descriptor.  This
 * function ignores an interrupted wait and tries again until successful.  This
 * is fine for some applications, but not for those that may want the signal
 * to abort the wait.
 */
enum {
	CAT_IOT_READ = 1,
	CAT_IOT_WRITE = 2,
	CAT_IOT_EXCEPT = 3
};
int io_check_ready(int fd, int type, double timeout);


/* 
 * Enable non-blocking on a file descriptor.  Returns 0 on success or -1 
 * on error.  Aborts on a negative file descriptor.
 */
int io_setnblk(int fd);

/* 
 * Disable non-blocking on a file descriptor.  Returns 0 on success or -1 
 * on error.  Aborts on a negative file descriptor.
 */
int io_clrnblk(int fd);


#endif /* CAT_HAS_POSIX */

#endif /* __cat_io_h */
