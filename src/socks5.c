/*
 * socks5.c -- SOCKS5 protocol routines.
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <cat/socks5.h>

#if CAT_HAS_POSIX 

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <cat/pack.h>
#include <cat/netinc.h>
#include <cat/io.h>


void socks5_sa_to_s5a(struct socks5addr *s5a, struct sockaddr *sa)
{
	abort_unless(s5a != NULL && sa != NULL);
	abort_unless(sa->sa_family == AF_INET || sa->sa_family == AF_INET6);
	if ( sa->sa_family == AF_INET ) {
		struct sockaddr_in *sin = (struct sockaddr_in *)sa;
		s5a->type = SOCKS5_AT_IPV4;
		s5a->len = 4;
		memcpy(s5a->addr_u.v4addr, &sin->sin_addr, 4);
		s5a->port = ntoh16(sin->sin_port);
	} else {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
		s5a->type = SOCKS5_AT_IPV6;
		s5a->len = 16;
		memcpy(s5a->addr_u.v6addr, &sin6->sin6_addr, 16);
		s5a->port = ntoh16(sin6->sin6_port);
	}
}


void socks5_dn_to_s5a(struct socks5addr *s5a, char *domainname, ushort port)
{
	size_t slen;
	abort_unless(s5a != NULL && domainname != NULL);
	slen = strlen(domainname);
	abort_unless(slen <= 255);

	s5a->type = SOCKS5_AT_DN;
	s5a->len = slen;
	memcpy(s5a->addr_u.dn, domainname, slen + 1);
	s5a->port = port;
}


int socks5_anon_probe(int fd)
{
	char buf[64];
	int len;
	byte_t v, m;

	len = pack(buf, sizeof(buf), "bbb", 5, 1, 0);
	if ( io_write(fd, buf, len) < len )
		return -1;
	if ( io_read(fd, buf, 2) < 2 )
		return -1;
	unpack(buf, 2, "bb", &v, &m);
	if ( v != 5 ) {
		errno = EPROTO;
		return -1;
	}
	if ( m != 0 ) {
		errno = EACCES;
		return -1 - m;
	}

	return 0;
}


static int resp_ok(int r_v, int r_rc, int r_at, int atype)
{
	if ( r_v != 5 ) {
		errno = EPROTO;
		return -1;
	}
	if ( r_at != atype ) {
		if ( (r_at != SOCKS5_AT_IPV4) && 
		     (r_at != SOCKS5_AT_DN) && 
		     (r_at != SOCKS5_AT_IPV6) ) {
			errno = EPROTO;
			return -1;
		}
		if ( atype != SOCKS5_AT_DN ) {
			errno = EPROTO;
			return -1;
		}
	}
	if ( r_rc != 0 )
		return -1 - r_rc;
	return 0;
}


int socks5_recvresp(int fd, int cmd, int atype, struct socks5addr *outsa)
{
	byte_t buf[SOCKS5_MAXLEN], *bp;
	int len, rok;
	ssize_t rv;
	byte_t r_v, r_rc, r_r, r_at;

	if ( cmd == SOCKS5_CT_UDP ) {

		/* get the return packet in one big whomp */
		if ( (rv = recv(fd, buf, SOCKS5_MAXLEN, 0)) < 0 )
			return -1;

		/* min response size is 4 bytes + 1-byte len + 2 bytes */
		if ( rv < 7 ) {
			errno = EIO;
			return -1;
		}

		/* validate return codes.  Ignore reserved bits.  Ensure */
		/* version is correct.  Make sure address type matches or is */
		/* valid address if the address we sent was a domain name */
		unpack(buf, 4, "bbbb", &r_v, &r_rc, &r_r, &r_at);
		if ( (rok = resp_ok(r_v, r_rc, r_at, atype)) != 0 )
			return rok;

		/* Set the correct length for the data */
		if ( r_at == SOCKS5_AT_IPV4 ) {
			len = 4;
		} else if ( r_at == SOCKS5_AT_DN ) {
			len = *((byte_t *)buf + 4);
		} else {
			len = 16;
		}

		if ( len > rv - 6 ) {
			errno = EPROTO;
			return -1;
		}
			
		bp = buf + 4;

	} else {

		/* read the first part of the response */
		if ( io_read(fd, buf, 4) < 4 )
			return -1;

		/* validate return codes.  Ignore reserved bits.  Ensure */
		/* version is correct.  Make sure address type matches or is */
		/* valid address if the address we sent was a domain name */
		unpack(buf, 4, "bbbb", &r_v, &r_rc, &r_r, &r_at);
		if ( (rok = resp_ok(r_v, r_rc, r_at, atype)) != 0 )
			return rok;

		/* determine the length of the return address */
		if ( r_at == SOCKS5_AT_IPV4 ) {
			len = 4;
		} else if ( r_at == SOCKS5_AT_DN ) {
			if ( io_read(fd, buf, 1) < 1 )
				return -1;
			len = *(byte_t *)buf;
		} else {
			len = 16;
		}

		/* read the actual response */
		if ( io_read(fd, buf, len + 2) < len + 2 )
			return -1;
		bp = buf;
	}

	/* report the response if the outsa was requested */
	if ( outsa != NULL ) {
		outsa->type = r_at;
		outsa->len = len;
		memcpy(&outsa->addr_u, bp, len);
		if ( r_at == SOCKS5_AT_DN )
			outsa->addr_u.dn[len] = '\0';
		unpack(bp + len, 2, "h", &outsa->port);
	}

	return 0;
}


int socks5_sendreq(int fd, int cmd, struct socks5addr *s5a)
{
	byte_t buf[SOCKS5_MAXLEN];
	int len;
	struct raw ra;

	abort_unless(cmd >= SOCKS5_CT_CONNECT && cmd <= SOCKS5_CT_UDP);
	abort_unless(s5a != NULL);
	abort_unless(s5a->type == SOCKS5_AT_IPV4 || 
		     s5a->type == SOCKS5_AT_DN || 
		     s5a->type == SOCKS5_AT_IPV6);
	if ( s5a->type == SOCKS5_AT_IPV4 ) {
		abort_unless(s5a->len == 4);
	} else if ( s5a->type == SOCKS5_AT_DN ) {
		abort_unless(s5a->len <= 255);
	} else {
		abort_unless(s5a->len == 16);
	}

	/* form the request */
	len = pack(buf, sizeof(buf), "bbbb", 5, cmd, 0, s5a->type);
	ra.data = (byte_t*)&s5a->addr_u;
	ra.len = s5a->len;
	len += pack(buf + len, sizeof(buf) - len, "rh", &ra, s5a->port);

	/* write the request */
	if ( io_write(fd, buf, len) < len )
		return -1;

	return 0;
}


int socks5_anon_conn(int fd, struct socks5addr *insa, struct socks5addr *outsa)
{
	int rv;

	if ( (rv = socks5_anon_probe(fd)) != 0 )
		return rv;

	if ( (rv = socks5_sendreq(fd, SOCKS5_CT_CONNECT, insa)) != 0 )
		return rv;

	return socks5_recvresp(fd, SOCKS5_CT_CONNECT, insa->type, outsa);
}


#endif /* CAT_HAS_POSIX */
