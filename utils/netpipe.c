#include <cat/cat.h>
#include <cat/net.h>
#include <cat/uevent.h>
#include <cat/io.h>
#include <cat/err.h>
#include <cat/ring.h>
#include <cat/stduse.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>

int lfd, sfd, cfd, crdone = 0, srdone = 0, cwdone = 0, swdone = 0;
unsigned bsiz = 4096;
char *ourname;
char *laddr = "0.0.0.0", *lport, *raddr, *rport;
struct uemux mux;
struct ring c2s, s2c;
struct ue_ioevent c2sr, c2sw, s2cr, s2cw;

int writer(void *arg, struct callback *cb);
int reader(void *arg, struct callback *cb);


int reader(void *arg, struct callback *cb)
{
	struct ring *r = cb->ctx;
	struct ue_ioevent *io = (struct ue_ioevent *)cb;
	int fd = io->fd, rv;
	unsigned long last, toend, olen;

	last = ring_last(r);
	toend = r->alloc - last;
	if ( ring_avail(r) < toend )
		toend = ring_avail(r);
	if ( toend > SSIZE_MAX )
		toend = SSIZE_MAX;

	rv = io_try_read(fd, r->data + last, toend);
	if ( rv < 0 ) {
		if ( rv == -2 )
			return 0;
		errsys("input error: ");
	} else if ( rv == 0 ) {
		if ( fd == cfd )
			crdone = 1;
		else
			srdone = 1;
		ue_io_cancel(io);
	} else { 
		olen = r->len;
		ring_put(r, NULL, rv, 0);
		if ( !ring_avail(r) )
			ue_io_cancel(io);
		if ( fd == cfd ) {
			if ( !olen && !swdone )
				ue_io_reg(&mux, &c2sw);
		} else {
			if ( !olen && !cwdone )
				ue_io_reg(&mux, &s2cw);
		}
	}

	return 0;
}


int writer(void *arg, struct callback *cb)
{
	struct ring *r = cb->ctx;
	struct ue_ioevent *io = (struct ue_ioevent *)cb;
	int fd = io->fd, rv;
	unsigned long toend, oavail;

	toend = r->alloc - r->start;
	if ( r->len < toend )
		toend = r->len;
	if ( toend > SSIZE_MAX )
		toend = SSIZE_MAX;

	rv = io_try_write(fd, r->data + r->start, toend);
	if ( rv < 0 ) {
		if ( rv == -2 )
			return 0;
		errsys("input error: ");
	} else if ( rv == 0 ) {
		if ( fd == cfd )
			cwdone = 1;
		else
			swdone = 1;
		ue_io_cancel(io);
	} else { 
		oavail = ring_avail(r);
		ring_get(r, NULL, rv);
		if ( !r->len )
			ue_io_cancel(io);
		if ( fd == cfd ) {
			if ( !oavail && !srdone )
				ue_io_reg(&mux, &s2cr);
		} else {
			if ( !oavail && !crdone )
				ue_io_reg(&mux, &c2sr);
		}
	}

	return 0;
}


void usage(char *str)
{
	err("%s\n"
	    "usage: %s [-b bufsize] [-l loc addr]\n"
	    "\t<loc port> <rem addr> <rem port>\n", str, ourname);
}


void getopts(int argc, char *argv[])
{
	int c;

	if ( argc < 4 )
		usage("too few arguments");

	while ( (c = getopt(argc, argv, "b:l:")) >= 0 ) {
		switch(c) {
		case 'b':
			bsiz = atoi(optarg);
			break;
		case 'l':
			laddr = optarg;
			break;
		case '?':
		default:
			usage("Unknown option");
		}
	}
	argc -= optind;
	argv += optind;
	lport = argv[0];
	raddr = argv[1];
	rport = argv[2];
}


int main(int argc, char *argv[])
{
	struct sockaddr_storage ss;
	socklen_t alen = sizeof(ss);
	char *c2sbuf, *s2cbuf;

	ourname = argv[0];
	getopts(argc, argv);
	printf("listening on %s:%s\n", laddr, lport);
	if ( (lfd = tcp_srv(laddr, lport)) < 0 )
		err("Couldn't open server address to %s:%s", laddr, lport);
	if ( (sfd = accept(lfd, (struct sockaddr *)&ss, &alen)) < 0 )
		errsys("Accepting: ");
	printf("Got Connection!\nConnecting to %s:%s\n", raddr, rport);
	if ( (cfd = tcp_cli(raddr, rport)) < 0 )
		err("Couldn't open socket to %s:%s", raddr, rport);

	close(lfd);
	if ( io_setnblk(sfd) < 0 )
		errsys("Couldn't set server to non-blocking mode");
	if ( io_setnblk(cfd) < 0 )
		errsys("Couldn't set client to non-blocking mode");

	ue_init(&mux);
	c2sbuf = emalloc(bsiz);
	s2cbuf = emalloc(bsiz);
	ring_init(&c2s, c2sbuf, bsiz);
	ring_init(&s2c, s2cbuf, bsiz);
	ue_io_init(&c2sr, UE_RD, cfd, reader, &c2s);
	ue_io_init(&s2cr, UE_RD, sfd, reader, &s2c);
	ue_io_init(&c2sw, UE_WR, sfd, writer, &c2s);
	ue_io_init(&s2cw, UE_WR, cfd, writer, &s2c);
	ue_io_reg(&mux, &c2sr);
	ue_io_reg(&mux, &s2cr);
	ue_run(&mux);

	free(c2sbuf);
	free(s2cbuf);
	close(sfd);
	close(cfd);

	return 0;
}
