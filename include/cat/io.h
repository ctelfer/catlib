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

/* should be equivalent to ssize_t, but that isn't part of the standard */

/* I/O functions */
ssize_t io_read(int fd, void *buf, ssize_t len); 
ssize_t io_write(int fd, void *buf, ssize_t len); 
ssize_t io_try_read(int fd, void *buf, ssize_t len); 
ssize_t io_try_write(int fd, void *buf, ssize_t len); 

/* I/O flags for file fdriptors */
int io_check_ready(int fd, int type, double timeout);
int io_setnblk(int fd);
int io_clrnblk(int fd);

enum {
  CAT_IOT_READ = 1,
  CAT_IOT_WRITE = 2,
  CAT_IOT_EXCEPT = 3
};

#endif /* CAT_HAS_POSIX */

#endif /* __cat_io_h */
