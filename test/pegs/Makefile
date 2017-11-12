PROGS=	peg-print peg-test-parse peg-calc testtok
	
CC=gcc

INC= -I../../include
CF= -g
LIBDEP=../../lib/libcata.a
LIB= -L../../lib -lcata

all: $(PROGS)

clean:
	rm -f $(PROGS) testtok.c testtok.h *.core

../../utils/pegcc:
	make -C ../../utils pegcc

peg-print: peg-print.c $(LIBDEP)
	$(CC) $(CF) -o peg-print peg-print.c $(INC) $(LIB)

peg-test-parse: peg-test-parse.c $(LIBDEP)
	$(CC) $(CF) -o peg-test-parse peg-test-parse.c $(INC) $(LIB)

peg-calc: peg-calc.c $(LIBDEP)
	$(CC) $(CF) -o peg-calc peg-calc.c $(INC) $(LIB)


testtok: ../../utils/pegcc testtok.peg $(LIBDEP)
	../../utils/pegcc -H testtok.peg -o testtok -t -T TOK_
	$(CC) $(CF) -o testtok testtok.c $(INC) $(LIB)
