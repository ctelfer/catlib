PROGS=	netpipe vernam pegcc
	
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

pegcc: pegcc.c $(LIBDEP)
	$(CC) $(CF) -o pegcc pegcc.c $(INC) $(LIB)
