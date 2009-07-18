/*
 * cat/net.h -- Network functions
 * 
 * Christopher Adam Telfer
 * 
 * Copyright 1999, 2003 see accompanying license
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

int tcp_srv(const char *host, const char *serv);
int tcp_cli(const char *host, const char *serv);
int udp_sock(char *lhost, char *lserv);

int net_resolv(const char *addr, const char *port, const char *proto,
	       struct sockaddr_storage *sas);
char * net_tostr(struct sockaddr *sa, char *buffer, size_t len);

#ifndef CAT_LISTENQ
#define CAT_LISTENQ 50
#endif 

#endif /* CAT_HAS_POSIX */

#endif /* __cat_net_h */
