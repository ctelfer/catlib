/*
 * cat/net.h -- Network functions
 * 
 * Christopher Adam Telfer
 * 
 * Copyright 1999, 2003, 2009 see accompanying license
 * 
 */ 

#ifndef __cat_net_h
#define __cat_net_h

#include <cat/cat.h>

#if CAT_HAS_POSIX

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SA struct sockaddr
#define SAS struct sockaddr_storage

/* 
 * Open a TCP socket and bind it on addr 'host' port 'serv'.
 * This function tries binding sockets that fit the 'host'/'serv'
 * pairing until the first one succeeds.  It also sets the socket in a
 * listening state with CAT_LISTENQ queue length.  Returns the file
 * descriptor or -1 on an error.
 */
int tcp_srv(const char *host, const char *serv);


/*
 * Opens a client connection to address 'host' and port 'serv' on a new socket.
 * This API will attempt to connection on every possible address for the given
 * pair until one succeeds.  This will include IPv4 and IPv6 addresses with
 * preference given according to the resolver.  This function returns the
 * socket file descriptor on success and -1 on failure.
 */
int tcp_cli(const char *host, const char *serv);

/*
 * Opens a UDP socket bound on address 'lhost' and port 'lserv'.  If neither 
 * 'lhost' or 'lserv' is specified, then the socket is not bound at all.  This
 * function returns the file descriptor on success and -1 on failure.
 */
int udp_sock(char *lhost, char *lserv);


/*
 * Attempts to populate a socket address using an address, port and protocol.
 * The address and port must be specified at a minimum, but the port can be
 * NULL.  In this case, the protocol will be set to whatever protocol is
 * first associated with "port" in the /etc/services file.  Protocols "tcp"
 * and "udp" are supported values for this field.  This function returns 0
 * on success and -1 on a resolution failure.  If the addr/port/proto
 * yields multiple socket addresses, this operation returns the first one
 * that the resolver returns.
 */
int net_resolv(const char *addr, const char *port, const char *proto,
               struct sockaddr_storage *sas);


/*
 * Formats a socket address in a human readable null-terminated ASCII string 
 * in 'buffer'.  The 'len' parameter is the maximum length of buffer including
 * the terminator.  This function returns the string on success or NULL if there
 * is an error with formatting.
 */
char * net_tostr(struct sockaddr *sa, char *buffer, size_t len);

#ifndef CAT_LISTENQ
#define CAT_LISTENQ 50
#endif 

#endif /* CAT_HAS_POSIX */

#endif /* __cat_net_h */
