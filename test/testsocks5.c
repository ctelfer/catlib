/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <stdio.h>
#include <string.h>
#include <cat/net.h>
#include <cat/err.h>
#include <cat/socks5.h>

int main(int argc, char *argv[])
{
	int fd;
	int rv;
	char line[256];
	FILE *in, *out;
	struct socks5addr insa, outsa;
	struct sockaddr_storage ss;

	if ( argc < 4 )
		err("usage: %s [proxy addr] [target addr] [target port]\n", argv[0]);

	if ( (fd = tcp_cli(argv[1], "1080")) < 0 )
		errsys("tcp_cli -- ");


	if ( net_resolv(argv[2], argv[3], "tcp", &ss) < 0 )
		errsys("net_resolv -- ");
	socks5_sa_to_s5a(&insa, (struct sockaddr *)&ss);

	if ( (rv = socks5_anon_conn(fd, &insa, &outsa)) < 0 )
		err("Socks5 error: %d\n", rv);

	if ( (in = fdopen(fd, "r")) == NULL )
		errsys("fdopen read");

	if ( (out = fdopen(fd, "w")) == NULL )
		errsys("fdopen write");

	while ( fgets(line, sizeof(line), stdin) ) {
		printf("Sending:  %s", line);
		fputs(line, out);
		fflush(out);
		fgets(line, sizeof(line), in);
		printf("Received: %s", line);
	}

	return 0;
}
