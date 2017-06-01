/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <cat/cat.h>
#include <cat/pspawn.h>
#include <cat/err.h>

#define DBG fprintf(stderr, "DBG:%s:%d\n", __FILE__, __LINE__);

extern char **environ;

void test1(char *file)
{
	struct pspawn *ps;
	struct ps_spec spec;
	struct ps_fd_entry *psfde;
	FILE *pin, *pout, *perr;
	char *t1[] = { "/bin/cat", NULL, "-", NULL };
	char line[256];
	int running;

	t1[1] = file;
	ps_spec_init(&spec);
	psfde = ps_spec_add_pipe(&spec,PSFD_IN|PSFD_STDIO|PSFD_LINEBUF);
	ps_fde_addfd(psfde, 0);
	psfde = ps_spec_add_pipe(&spec, PSFD_OUT|PSFD_STDIO|PSFD_FULLBUF);
	ps_fde_addfd(psfde, 1);
	ps_spec_keepfd(&spec, 2);
	/* ps_fde_addfd(psfde, 2); */

	ps = ps_launch(t1, NULL, &spec);
	if (ps == NULL)
		errsys("error launching /bin/cat:\n");

	printf("1: Process %d is %s\n", ps->cpid, 
	       ps_running(ps) ? "running" : "not running");
	fflush(stdout);

	pin = ps_get_locfile(ps, 0);
	if (pin == NULL)
		err("Couldn't find input file");
	fputs("Hello World\n", pin);
	fputs("Another line\n", pin);
	ps_closeio(ps, 0);

	printf("2: Process %d is %s\n", ps->cpid, 
	       ps_running(ps) ? "running" : "not running");
	fflush(stdout);

	pout = ps_get_locfile(ps, 1);
	if (pout == NULL)
		errsys("Can't find process output\n");

	while (fgets(line, sizeof(line), pout))
		printf("line> %s", line);

	printf("3: Process %d is %s\n", ps->cpid, 
	       (running = ps_running(ps)) ? "running" : "not running");
	fflush(stdout);

	if (running) {
		sleep(1);
		printf("4: Process %d is %s\n", ps->cpid, 
		       ps_running(ps) ? "running" : "not running");
		fflush(stdout);
	}

	printf("cleanup return code = %d\n", ps_cleanup(ps, 1));

	printf("Done 1\n");
}


void test2(char *file)
{
	struct pspawn *ps;
	FILE *pin, *pout, *perr;
	char line[256];
	int running;

	ps = ps_spawn("el01", environ, "/bin/cat", file, "-", NULL);
	if (ps == NULL)
		errsys("error2 launching /bin/cat:\n");

	printf("4: Process %d is %s\n", ps->cpid, 
	       ps_running(ps) ? "running" : "not running");
	fflush(stdout);

	pin = ps_get_locfile(ps, 0);
	if (pin == NULL)
		err("Couldn't find input file");
	fputs("Hello World 2\n", pin);
	fputs("Another line 2\n", pin);
	ps_closeio(ps, 0);

	printf("5: Process %d is %s\n", ps->cpid, 
	       ps_running(ps) ? "running" : "not running");
	fflush(stdout);

	pout = ps_get_locfile(ps, 1);
	if (pout == NULL)
		errsys("Can't find process output\n");

	while (fgets(line, sizeof(line), pout))
		printf("line> %s", line);

	printf("6: Process %d is %s\n", ps->cpid, 
	       (running = ps_running(ps)) ? "running" : "not running");
	fflush(stdout);

	if (running) {
		sleep(1);
		printf("6: Process %d is %s\n", ps->cpid, 
		       ps_running(ps) ? "running" : "not running");
		fflush(stdout);
	}

	printf("cleanup return code = %d\n", ps_cleanup(ps, 1));

	printf("Done 2\n");
}



void test3()
{
	struct pspawn *ps;
	FILE *pin, *pout, *perr;
	char line[256];
	int running;

	if ((ps = ps_spawn("el01+2", environ, "/usr/bin/env", NULL)) == NULL)
		errsys("error2 launching /bin/cat:\n");

	printf("7: Process %d is %s\n", ps->cpid, 
	       ps_running(ps) ? "running" : "not running");
	fflush(stdout);

	if ((pout = ps_get_locfile(ps, 1)) == NULL)
		errsys("Can't find process output\n");
	while (fgets(line, sizeof(line), pout))
		printf("line> %s", line);

	printf("8: Process %d is %s\n", ps->cpid, 
	       (running = ps_running(ps)) ? "running" : "not running");
	fflush(stdout);

	if (running) {
		sleep(1);
		printf("9: Process %d is %s\n", ps->cpid, 
		       ps_running(ps) ? "running" : "not running");
		fflush(stdout);
	}

	printf("cleanup return code = %d\n", ps_cleanup(ps, 1));

	printf("Done 3\n");
}



int main(int argc, char *argv[]) 
{ 
	if (argc < 2)
		err("usage: %s file\n", argv[0]);

	ps_ignore_sigcld();
	test1(argv[1]);
	test2(argv[1]);
	test3();

	return 0;
}
