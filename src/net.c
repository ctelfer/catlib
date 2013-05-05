/*
 * net.c -- Network functions
 *
 * Christopher Adam Telfer
 *
 * Copyright 1999-2012 -- see accompanying license
 *
 */

#include <cat/cat.h>

#if CAT_HAS_POSIX

#include <cat/net.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>

#ifndef CAT_HAS_ADDRINFO
#define CAT_HAS_ADDRINFO 1
#endif


#if CAT_HAS_ADDRINFO


int net_resolv(const char *host, const char *serv, const char *proto,
	       struct sockaddr_storage *sas)
{
	struct addrinfo *res, *hintsp = NULL, hints;

	abort_unless(sas);

	if ( proto != NULL ) {
		hintsp = &hints;
		memset(hintsp, 0, sizeof(struct addrinfo));
		hintsp->ai_family = AF_UNSPEC;
		if ( strcmp(proto, "tcp") == 0 ) {
			hintsp->ai_socktype = SOCK_STREAM;
			hintsp->ai_protocol = IPPROTO_TCP;
		} else if ( strcmp(proto, "udp") == 0 ) {
			hintsp->ai_socktype = SOCK_DGRAM;
			hintsp->ai_protocol = IPPROTO_UDP;
		} else {
			return -1;
		}
	}

	if ( getaddrinfo(host, serv, hintsp, &res) < 0 )
		return -1;
	memcpy(sas, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);

	return 0;
}


/*
 * This function is used to open a passive tcp socket.  It uses hostname and
 * service to declare explicity which address and port to bind to if specified.
 * To for a connection from here do an accept()...
 */
int tcp_srv(const char *host, const char *serv)
{
	int r, sock, set = 1;
	struct addrinfo hints, *res, *trav;

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ( (r = getaddrinfo(host, serv, &hints, &res)) < 0 )
		return -1;
	trav = res;

	do {
		sock = socket(trav->ai_family, trav->ai_socktype,
			      trav->ai_protocol);
		if ( sock < 0 )
			goto nextsock;
		if ( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set,
							sizeof(set)) < 0 )
			goto badsock;
		if ( !bind(sock, trav->ai_addr, trav->ai_addrlen) )
			break;
badsock:
		close(sock);
nextsock:
		trav = trav->ai_next;

	} while (trav);

	if ( !trav )
		return -3;
	listen(sock, 16);
	freeaddrinfo(res);

	return sock;
}


/*
 * This function opens a tcp connection to the address specified.  It is
 * (theoretically) protocol independant.  You can start reading and writing
 * from here.
 */
int tcp_cli(const char *host, const char *serv)
{
	int r, sock;
	struct addrinfo hints, *res, *trav;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ( (r = getaddrinfo(host, serv, &hints, &res)) < 0 )
		return -1;

	trav = res;
	do {
		sock = socket(trav->ai_family, trav->ai_socktype,
			      trav->ai_protocol);
		if ( sock < 0 )
			continue;
		if ( connect(sock, trav->ai_addr, trav->ai_addrlen) == 0 )
			break;
		close(sock);
		trav = trav->ai_next;
	} while (trav);

	if ( !trav )
		return -3;
	freeaddrinfo(res);

	return sock;
}


/*
 * This opens a passive udp socket on the system.  The hostname and service
 * will specifiy the address and port if given.  Addrlen returns the length
 * of the address (for protocol independence).
 */
int udp_sock(char *host, char *serv)
{
	int r, sock, set = 1;
	struct addrinfo hints, *res, *trav;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags  = AI_PASSIVE;
	hints.ai_socktype = SOCK_DGRAM;
	if ( (r = getaddrinfo(host, serv, &hints, &res)) < 0 )
		return -1;

	trav = res;
	do {
		sock = socket(trav->ai_family, trav->ai_socktype,
			      trav->ai_protocol);
		if ( sock < 0 )
			goto nextsock;
		if ( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set,
				sizeof(set)) < 0 )
			goto badsock;
		if ( !bind(sock, trav->ai_addr, trav->ai_addrlen) )
			break;
badsock:
		close(sock);
nextsock:
		trav = trav->ai_next;
	} while (trav);

	if ( !trav )
		return -3;
	freeaddrinfo(res);

	return sock;
}


int tcp_cli_nb(const char *host, const char *serv)
{
	int r, sock;
	int flags;
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ( (r = getaddrinfo(host, serv, &hints, &res)) < 0 )
		return -1;

	sock = socket(res->ai_family, res->ai_socktype,
		      res->ai_protocol);
	if ( sock < 0 ) {
		r = -2;
		goto err;
	}

        flags = fcntl(sock, F_GETFL);
        if ( flags < 0 ) {
                r = -3;
		goto err;
	}

        flags |= O_NONBLOCK;

        if ( fcntl(sock, F_SETFL, flags) < 0 ) {
		goto err;
	}

	r = connect(sock, res->ai_addr, res->ai_addrlen);
	if ( (r == -1) && (errno == EINPROGRESS) )
		r = 0;

err:
	freeaddrinfo(res);
	return r;
}



#else /* CAT_HAS_ADDRINFO */


int net_resolv(const char *host, const char *serv, const char *proto,
	       struct sockaddr_storage *sas)
{
	struct hostent *hp;
	struct servent *sp;
	ulong portnum;
	char *cp;
	struct sockaddr_in *sin = (struct sockaddr_in *)sas;

	abort_unless(sas);

	memset(sas, 0, sizeof(struct sockaddr_in));
	sin->sin_family = AF_INET;

	/* Look up the address */
	if ( !host || !*host )
		sin->sin_addr.s_addr = INADDR_ANY;
	else if ( inet_pton(AF_INET, host, &sin->sin_addr.s_addr) )
		;
	else if ( (hp = gethostbyname(host)) )
		memcpy(&sin->sin_addr, hp->h_addr, hp->h_length);
	else
		return -1;

	/* now look up the service */
	if (!serv || !*serv) {
		sin->sin_port = 0;
		goto skiplookup;
	}

	portnum = strtoul(serv, &cp, 0);
	if ( cp != serv ) {
		if ( portnum > 65535 )
			return -1;
		sin->sin_port = htons(portnum);
	} else if ( (sp = getservbyname(serv, proto)) )
		sin->sin_port = sp->s_port;
	else
			return -1;

skiplookup:
	return 0;
}


/*
 * This function is used to open a passive tcp socket.  It uses hostname and
 * service to declare explicity which address and port to bind to if specified.
 * The length of the address is returned in addrlen if addrlen != 0;  To wait
 * for a connection from here do an Accept()...
 */
int tcp_srv(const char *host, const char *serv)
{
	int sock, set = 1;
	struct sockaddr_storage sas;
	struct sockaddr_in *sin = (struct sockaddr_in *)&sas;

	if ( net_resolv(host, serv, "tcp", &sas) < 0 )
		return -1;
	/* now we have the address to use */
	if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
		return -2;
	if ( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set))<0 ) {
		close(sock);
		return -3;
	}
	if ( bind(sock, (SA *)sin, sizeof(*sin)) < 0 ) {
		close(sock);
		return -4;
	}
	if ( listen(sock, CAT_LISTENQ) < 0 ) {
		close(sock);
		return -5;
	}

	return sock;
}


/*
 * This function opens a tcp connection to the address specified.  It is
 * not protocol independant.  You can start reading and writing
 * from here.
 */
int tcp_cli(const char *host, const char *serv)
{
	int sock;
	struct sockaddr_storage sas;
	struct sockaddr_in *sin = (struct sockaddr_in *)&sas;

	if ( net_resolv(host, serv, "tcp", &sas) < 0 )
		return -1;
	if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
		return -2;
	if ( connect(sock, (SA *)sin, sizeof(*sin)) < 0 ) {
		close(sock);
		return -3;
	}

	return sock;
}


/*
 * This opens a passive udp socket on the system.  The hostname and service
 * will specifiy the address and port if given.
 */
int udp_sock(char *host, char *serv)
{
	int sock, set = 1;
	struct sockaddr_storage sas;
	struct sockaddr_in *sin = (struct sockaddr_in *)&sas;

	if ( net_resolv(host, serv, "udp", &sas) < 0 )
		return -1;
	if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
		return -2;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0) {
		close(sock);
		return -3;
	}
	if ( bind(sock, (SA *)sin, sizeof(*sin)) < 0 ) {
		close(sock);
		return -4;
	}

	return sock;
}


int tcp_cli_nb(const char *host, const char *serv)
{
	int sock;
	struct sockaddr_storage sas;
        int flags;
	struct sockaddr_in *sin = (struct sockaddr_in *)&sas;

	if ( net_resolv(host, serv, "tcp", &sas) < 0 )
		return -1;

	if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
		return -2;

        flags = fcntl(sock, F_GETFL);
        if ( flags < 0 )
                return -3;

        flags |= O_NONBLOCK;

        if ( fcntl(sock, F_SETFL, flags) < 0 )
                return -4;

	if ( connect(sock, (SA *)sin, sizeof(*sin)) < 0 ) {
		close(sock);
		return -5;
	}

	return sock;
}



#endif /* CAT_HAS_ADDRINFO */


char * net_tostr(struct sockaddr *sa, char *buf, size_t len)
{
	abort_unless(len > 0);
	abort_unless(buf);

	switch ( sa->sa_family ) {

	case AF_INET : {
		struct sockaddr_in sin;
		uchar *p;
		/* XXX.XXX.XXX.XXX.XXXXX0 */
		if ( len < 22 )
			return NULL;

		sin = *(struct sockaddr_in *)sa;
		p = (uchar *)&sin.sin_addr;
		len = snprintf(buf, len, "%u.%u.%u.%u:%u",
			       p[0], p[1], p[2], p[3], ntohs(sin.sin_port));
		return buf;
	} break;

#ifdef AF_INET6
	case AF_INET6 : {
		char *p, *ap;
		char *off;
		ap = (char *)&((struct sockaddr_in6 *)sa)->sin6_addr;
		/* deliberately discard constant qualifier */
		p = (char *)inet_ntop(sa->sa_family, ap, buf, len);
		if ( p == NULL )
			return NULL;
		off = p + strlen(p);
		if ( (len - (off - p)) < 6 )
			return NULL;
		snprintf(off, len - (off - p), ".%u",
			 ntohs(((struct sockaddr_in6 *)sa)->sin6_port));
		return p;
	} break;
#endif /* IPV6 */

#ifdef AF_UNIX
	case AF_UNIX: {
		char *path = ((struct sockaddr_un *)sa)->sun_path;
		size_t plen = strlen(path) + 1;
		if ( len < plen )
			return NULL;
		memcpy(buf, path, plen);
		return buf;
	} break;
#endif /* AF_UNIX */

	default:
		return NULL;
	}
}



int tcp_cli_nb_completion(int fd)
{
	int err;
	socklen_t size = sizeof(err);
	abort_unless(fd < 0);
	if ( getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &size) < 0 )
		return -1;
	return err;
}


#endif /* CAT_HAS_POSIX */
