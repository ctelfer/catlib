TESTS=	testlist testhash testtcpc testtcps testudpc testudps testpool \
	testheap testmem testhw testtime testavl testpack testdl \
	testpcache testuemux testrb markov markov2 testmatch \
	testsplay testcsv testbitset testshell testgraph testprintf teststr \
	testbitops testpspawn testdynmem testtlsf testmalloc testregex \
	testlex testsort testoptparse testcatstr testcrypto testsocks5 testcrc
	
CFILES= testlist.c testhash.c testtcpc.c testtcps.c testudpc.c testudps.c \
	testpool.c testmem.c testheap.c testhw.c testtime.c testavl.c \
	testpack.c testdl.c testpcache.c testuemux.c testrb.c markov.c	\
	markov2.c testmatch.c testsplay.c testcsv.c testbitset.c \
	testshell.c testgraph.c testprintf.c teststr.c testbitops.c \
	testdynmem.c testtlsf.c testmalloc.c testregex.c testlex.c testsort.c \
	testoptparse.c testcatstr.c testcrypto.c testsocks5.c testcrc.c

CC=gcc

INC= -I ../include

# Use optimized library
CAT_LIBDEP=../lib/libcat.a
CAT_CF= -O3
CAT_LIB= -L../lib -lcat

# Use base library
CATA_LIBDEP=../lib/libcata.a
CATA_CF= -g -DCAT_USE_INLINE=0 -DCAT_HAS_LONGLONG=0 -ansi -pedantic \
    -D_POSIX_C_SOURCE=200112L -D_USE_BSD
CATA_LIB= -L../lib -lcata

# Debug library
CAT_DBG_LIBDEP=../lib/libcat_dbg.a
CAT_DBG_CF= -g -pg
CAT_DBG_LIB= -L../lib -lcat_dbg

# TODO: Use no-libc version


all: $(TESTS)

clean:
	rm -f $(TESTS) core.*

$(CATA_LIBDEP):
	( cd ../src ; make $(CATA_LIBDEP))

$(CAT_LIBDEP):
	( cd ../src ; make $(CAT_LIBDEP))

testlist: testlist.c $(CAT_LIBDEP)
	$(CC) $(CAT_CF) -o testlist testlist.c $(INC) $(CAT_LIB)

testhash: testhash.c $(CAT_LIBDEP)
	$(CC) $(CAT_CF) -o testhash testhash.c $(INC) $(CAT_LIB)

testavl: testavl.c $(CAT_LIBDEP)
	$(CC) $(CAT_CF) -o testavl testavl.c $(INC) $(CAT_LIB)

testtcpc: testtcpc.c $(CATA_LIBDEP)
	$(CC) $(CATA_CF) -o testtcpc testtcpc.c $(INC) $(CATA_LIB)

testtcps: testtcps.c $(CATA_LIBDEP)
	$(CC) $(CATA_CF) -o testtcps testtcps.c $(INC) $(CATA_LIB)

testudpc: testudpc.c $(CATA_LIBDEP)
	$(CC) $(CATA_CF) -o testudpc testudpc.c $(INC) $(CATA_LIB)

testudps: testudps.c $(CATA_LIBDEP)
	$(CC) $(CATA_CF) -o testudps testudps.c $(INC) $(CATA_LIB)

testpool: testpool.c $(CAT_LIBDEP)
	$(CC) $(CAT_CF) -o testpool testpool.c $(INC) $(CAT_LIB)

testpcache: testpcache.c $(CAT_LIBDEP)
	$(CC) $(CAT_CF) -o testpcache testpcache.c $(INC) $(CAT_LIB)

testheap: testheap.c $(CATA_LIBDEP)
	$(CC) $(CATA_CF) -o testheap testheap.c $(INC) $(CATA_LIB)

testtime: testtime.c $(CATA_LIBDEP)
	$(CC) $(CATA_CF) -o testtime testtime.c $(INC) $(CATA_LIB)

testpack: testpack.c $(CAT_LIBDEP)
	$(CC) $(CAT_CF) -o testpack testpack.c $(INC) $(CAT_LIB)

testdl: testdl.c $(CATA_LIBDEP)
	$(CC) $(CATA_CF) -o testdl testdl.c $(INC) $(CATA_LIB)

testuemux: testuemux.c $(CAT_DBG_LIBDEP)
	$(CC) $(CAT_DBG_CF) -o testuemux testuemux.c $(INC) $(CAT_DBG_LIB)

testrb: testrb.c $(CAT_LIBDEP)
	$(CC) $(CAT_CF) -o testrb testrb.c $(INC) $(CAT_LIB)

testhw: testhw.c 
	$(CC) -O3 -o testhw testhw.c 

markov: markov.c $(CATA_LIBDEP)
	$(CC) $(CATA_CF) -o markov markov.c $(INC) $(CATA_LIB)

markov2: markov2.c $(CATA_LIBDEP)
	$(CC) $(CATA_CF) -o markov2 markov2.c $(INC) $(CATA_LIB)

testmatch: testmatch.c $(CATA_LIBDEP)
	$(CC) $(CATA_CF) -o testmatch testmatch.c $(INC) $(CATA_LIB)

testmem: testmem.c $(CATA_LIBDEP)
	$(CC) $(CATA_CF) -o testmem testmem.c $(INC) $(CATA_LIB)

testsplay: testsplay.c $(CAT_LIBDEP)
	$(CC) $(CAT_CF) -o testsplay testsplay.c $(INC) $(CAT_LIB)

testcsv: testcsv.c $(CATA_LIBDEP)
	$(CC) $(CATA_CF) -o testcsv testcsv.c $(INC) $(CATA_LIB)

testbitset: testbitset.c $(CATA_LIBDEP)
	$(CC) $(CATA_CF) -o testbitset testbitset.c $(INC) $(CATA_LIB)

testshell: testshell.c $(CATA_LIBDEP)
	$(CC) $(CATA_CF) -o testshell testshell.c $(INC) $(CATA_LIB)

testgraph: testgraph.c $(CAT_DBG_LIBDEP)
	$(CC) $(CAT_DBG_CF) -o testgraph testgraph.c $(INC) $(CAT_DBG_LIB)

testprintf: testprintf.c $(CAT_LIBDEP)
	$(CC) $(CAT_CF) -o testprintf testprintf.c $(INC) $(CAT_LIB)

teststr: teststr.c $(CATA_LIBDEP)
	$(CC) $(CATA_CF) -o teststr teststr.c $(INC) $(CATA_LIB)

testbitops: testbitops.c $(CAT_LIBDEP)
	$(CC) $(CAT_CF) -o testbitops testbitops.c $(INC) $(CAT_LIB)

testpspawn: testpspawn.c $(CAT_LIBDEP)
	$(CC) $(CAT_CF) -o testpspawn testpspawn.c $(INC) $(CAT_LIB)

testdynmem: testdynmem.c $(CAT_DBG_LIBDEP)
	$(CC) $(CAT_DBG_CF) -o testdynmem testdynmem.c $(INC) $(CAT_DBG_LIB)

testtlsf: testtlsf.c $(CAT_DBG_LIBDEP)
	$(CC) $(CAT_DBG_CF) -o testtlsf testtlsf.c $(INC) $(CAT_DBG_LIB)

testmalloc: testmalloc.c $(CAT_LIBDEP)
	$(CC) $(CAT_CF) -o testmalloc testmalloc.c $(INC) $(CAT_LIB)

testregex: testregex.c $(CAT_DBG_LIBDEP)
	$(CC) $(CAT_DBG_CF) -o testregex testregex.c $(INC) $(CAT_DBG_LIB)

testlex: testlex.c $(CAT_DBG_LIBDEP)
	$(CC) $(CAT_DBG_CF) -o testlex testlex.c $(INC) $(CAT_DBG_LIB)

testoptparse: testoptparse.c $(CAT_DBG_LIBDEP)
	$(CC) $(CAT_DBG_CF) -o testoptparse testoptparse.c $(INC) $(CAT_DBG_LIB)

testsort: testsort.c $(CAT_LIBDEP)
	$(CC) $(CAT_CF) -o testsort testsort.c $(INC) $(CAT_LIB)
#	$(CC) $(CAT_DBG_CF) -o testsort testsort.c $(INC) $(CAT_DBG_LIB)

testcatstr: testcatstr.c $(CAT_DBG_LIBDEP)
	$(CC) $(CAT_DBG_CF) -o testcatstr testcatstr.c $(INC) $(CAT_DBG_LIB)

testcrypto: testcrypto.c $(CAT_DBG_LIBDEP)
	$(CC) $(CAT_DBG_CF) -o testcrypto testcrypto.c $(INC) $(CAT_DBG_LIB)

testsocks5: testsocks5.c $(CAT_DBG_LIBDEP)
	$(CC) $(CAT_DBG_CF) -o testsocks5 testsocks5.c $(INC) $(CAT_DBG_LIB)

testcrc: testcrc.c $(CAT_DBG_LIBDEP)
	$(CC) $(CAT_DBG_CF) -o testcrc testcrc.c $(INC) $(CAT_DBG_LIB)
