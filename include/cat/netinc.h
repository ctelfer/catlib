/*
 * cat/netinc.h -- standard headers to include in a network application
 * 
 * Christopher Adam Telfer
 * 
 * Copyright 1999,2003 -- See accompanying license
 * 
 */ 

#ifndef __CAT_NETINC_H
#define __CAT_NETINC_H

/* Here go all of the include files for the networking code */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/select.h>
#include <poll.h>
#include <sys/ioctl.h>

/* XXX do we want these always? */
#include <pthread.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <syslog.h>

#ifndef SUN_LEN
#define SUN_LEN(su) \
	(sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif /* SUN_LEN */

#endif /* __CAT_NETINC_H */
