PROGS=	netpipe vernam pegcc rwatch
	
CC=gcc

INC=-I../include

CF= -O3
LIBDEP=../lib/libcat.a
LIB= -L../lib -lcat

#CF=-g
#LIBDEP=../lib/libcat_dbg.a
#LIB= -L../lib -lcat_dbg

all: $(PROGS)

clean:
	rm -f $(PROGS) *.core rwatch.c

$(LIBDEP):
	( cd ../src ; make )

netpipe: netpipe.c $(LIBDEP)
	$(CC) $(CF) -o netpipe netpipe.c $(INC) $(LIB)

vernam: vernam.c $(LIBDEP)
	$(CC) $(CF) -o vernam vernam.c $(INC) $(LIB)

pegcc: pegcc.c $(LIBDEP)
	$(CC) $(CF) -o pegcc pegcc.c $(INC) $(LIB)


rwatch.c: pegcc rwatch.pp
	./pegcc -o rwatch rwatch.pp

rwatch: rwatch.c
	$(CC) $(CF) -o rwatch rwatch.c $(INC) $(LIB)
