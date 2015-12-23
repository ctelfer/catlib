PROGS=	peg-print peg-test-parse peg-calc
	
CC=gcc

INC= -I../../include
CF= -g
LIBDEP=../../lib/libcata.a
LIB= -L../../lib -lcata

all: $(PROGS)

clean:
	rm -f $(PROGS) *.core

peg-print: peg-print.c $(LIBDEP)
	$(CC) $(CF) -o peg-print peg-print.c $(INC) $(LIB)

peg-test-parse: peg-test-parse.c $(LIBDEP)
	$(CC) $(CF) -o peg-test-parse peg-test-parse.c $(INC) $(LIB)

peg-calc: peg-calc.c $(LIBDEP)
	$(CC) $(CF) -o peg-calc peg-calc.c $(INC) $(LIB)
