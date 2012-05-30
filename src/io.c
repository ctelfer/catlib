/*
 * io.c -- Input/Output functions
 *
 * Christopher Adam Telfer
 *
 * Copyright 1999 - 2012 -- see accompanying license
 *
 */

#include <cat/cat.h>

#if CAT_HAS_POSIX

#include <cat/io.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

ssize_t io_read(int fd, void *buf, ssize_t nb) 
{ 
	byte_t *p = buf; 
	ssize_t nr = 0;
	ssize_t n;

	abort_unless(fd >= 0);
	abort_unless(buf != NULL);
	abort_unless(nb >= 0);

	while (nr < nb) { 
		if ( (n = read(fd, p, nb - nr)) < 0) {
			if ( (errno == EINTR) || (errno == EAGAIN) )
				continue; 
			else 
				return -1;
		} else if (n == 0)
			break; 

		nr += n;
		p += n; 
	} 

	return nr;
}


ssize_t io_write(int fd, void *buf, ssize_t nb) 
{ 
	byte_t *p = buf; 
	ssize_t nw = 0;
	ssize_t n;

	abort_unless(fd >= 0);
	abort_unless(buf != NULL);
	abort_unless(nb >= 0);

	while ( nw < nb ) { 
		if ( (n = write(fd, p, nb - nw)) < 0) {
			if ( (errno == EINTR) || (errno == EAGAIN) )
				continue; 
			else 
				return -1;
		} else if ( n == 0 )
			break;

		nw += n;
		p += n; 
	} 

	return nw;
}


ssize_t io_read_upto(int fd, void *buf, ssize_t nb) 
{ 
	byte_t *p = buf; 
	ssize_t n;

	abort_unless(fd >= 0);
	abort_unless(buf != NULL);
	abort_unless(nb >= 0);

	while ( ((n = read(fd, p, nb)) == -1) && (errno == EINTR) )
		;

	return n;
}


ssize_t io_write_upto(int fd, void *buf, ssize_t nb) 
{ 
	byte_t *p = buf; 
	ssize_t n;

	abort_unless(fd >= 0);
	abort_unless(buf != NULL);
	abort_unless(nb >= 0);


	while ( ((n = write(fd, p, nb)) == -1) && (errno == EINTR) )
		;

	return n;
}


int io_check_ready(int fd, int type, double timeout)
{
	fd_set set, *rp = NULL, *wp = NULL, *ep = NULL;
	struct timeval tv, tvcopy = { 0 }, *tvp = NULL;

	abort_unless(fd >= 0);

	FD_ZERO(&set);
	FD_SET(fd, &set);
	if ( timeout > 0 ) {
		tvcopy.tv_sec = (long)timeout;
		timeout -= (long)timeout;
		tvcopy.tv_usec = (long)(timeout * 1000000.0);
		tvp = &tv;
	}

	if ( type == CAT_IOT_READ )
		rp = &set;
	else if ( type == CAT_IOT_WRITE )
		wp = &set;
	else if ( type == CAT_IOT_EXCEPT )
		ep = &set;
	else {
		errno = EINVAL;
		return -1;
	}

again:
	tv = tvcopy;
	if ( select(fd + 1, rp, wp, ep, tvp) < 0 ) {
		if ( errno == EINTR )
			goto again;
		else
			return -1;
	}

	return FD_ISSET(fd, &set) ? 1 : 0;
}


int io_setnblk(int fd)
{
	int flags;

	abort_unless(fd >= 0);

	flags = fcntl(fd, F_GETFL);
	if ( flags < 0 )
		return -1;

	flags |= O_NONBLOCK;

	if ( fcntl(fd, F_SETFL, flags) < 0 )
		return -1;
	else
		return 0;
}


int io_clrnblk(int fd)
{
	int flags;

	abort_unless(fd >= 0);

	flags = fcntl(fd, F_GETFL);
	if ( flags < 0 )
		return -1;

	flags &= ~O_NONBLOCK;

	if ( fcntl(fd, F_SETFL, flags) < 0 )
		return -1;
	else
		return 0;
}

#endif /* CAT_HAS_POSIX */
