PROGS=	netpipe vernam peg-print
	
CPROGS= netpipe.c vernam.c peg-print.c

CC=gcc

INC= -I../include
CF= -O3
LIBDEP=../lib/libcata.a
LIB= -L../lib -lcata

# for use with inline functions
#LIBDEP=../lib/libcat.a
#LIB= -L../lib -lcat

all: $(PROGS)

clean:
	rm -f $(PROGS) *.core

$(LIBDEP):
	( cd ../src ; make )

netpipe: netpipe.c $(LIBDEP)
	$(CC) $(CF) -o netpipe netpipe.c $(INC) $(LIB)

vernam: vernam.c $(LIBDEP)
	$(CC) $(CF) -o vernam vernam.c $(INC) $(LIB)

peg-print: peg-print.c $(LIBDEP)
	#$(CC) $(CF) -o peg-print peg-print.c $(INC) $(LIB)
	$(CC) -g -o peg-print peg-print.c $(INC) -L../lib -lcat_dbg
