#ifndef __anonsocks_h
#define __anonsocks_h

#include <cat/cat.h>

#if CAT_HAS_POSIX

#include <sys/socket.h>

#define SOCKS5_CT_CONNECT	1
#define SOCKS5_CT_BIND		2
#define SOCKS5_CT_UDP		3

#define SOCKS5_AT_IPV4		1
#define SOCKS5_AT_DN		3
#define SOCKS5_AT_IPV6		4

#define SOCKS5_MAXLEN		264

#define SOCKS5_RC_OK		0 /* succeeded */
#define SOCKS5_RC_FAIL		1 /* general SOCKS server failure */
#define SOCKS5_RC_DISALLOWED	2 /* connection not allowed by ruleset */
#define SOCKS5_RC_NUNREACH	3 /* Network unreachable */
#define SOCKS5_RC_HUNREACH	4 /* Host unreachable */
#define SOCKS5_RC_ECONN		5 /* Connection refused */
#define SOCKS5_RC_ETTL		6 /* TTL expired */
#define SOCKS5_RC_CTUNSUPP	7 /* Command not supported */
#define SOCKS5_RC_ATUNSUPP	8 /* Address type not supported */

/* return codes returned by socks5_recvresp() */
#define SOCKS5_RRC_OK		0  /* succeeded */
#define SOCKS5_RRC_SYSERR	-1 /* system error */
#define SOCKS5_RRC_FAIL		-2 /* general SOCKS server failure */
#define SOCKS5_RRC_DISALLOWED	-3 /* connection not allowed by ruleset */
#define SOCKS5_RRC_NUNREACH	-4 /* Network unreachable */
#define SOCKS5_RRC_HUNREACH	-5 /* Host unreachable */
#define SOCKS5_RRC_ECONN	-6 /* Connection refused */
#define SOCKS5_RRC_ETTL		-7 /* TTL expired */
#define SOCKS5_RRC_CTUNSUPP	-8 /* Command not supported */
#define SOCKS5_RRC_ATUNSUPP	-9 /* Address type not supported */


struct socks5addr {
	byte_t			type;
	byte_t			len;
	union {
		byte_t		v4addr[4];
		byte_t		v6addr[16];
		byte_t		dn[256];
	} addr_u;
	ushort			port;
};


void socks5_sa_to_s5a(struct socks5addr *s5a, struct sockaddr *sa);


void socks5_dn_to_s5a(struct socks5addr *s5a, char *domainname, ushort port);


int socks5_anon_probe(int fd);


int socks5_recvresp(int fd, int cmd, int atype, struct socks5addr *outsa);


int socks5_sendreq(int fd, int cmd, struct socks5addr *s5a);


int socks5_anon_conn(int fd, struct socks5addr *insa, struct socks5addr *outsa);


#endif /* CAT_HAS_POSIX */

#endif /* __anonsocks_h */
