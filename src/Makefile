LCATODIR=  ../build/libcat
LCATAODIR=  ../build/libcata
LCAT_DBG_ODIR= ../build/libcat_dbg
LCAT_NO_LIBC_ODIR=  ../build/libcat_nolibc

all: makefile
	make -f makefile

makefile: makefile.tmpl
	./buildmake.sh
	@echo "makefile rebuilt: invoke 'make' again to build" >&2

clean:
	rm -f $(LCATODIR)/*.o $(LCATAODIR)/*.o $(LCAT_NO_LIBC_ODIR)/*.o \
		$(LCAT_DBG_OF) \
		../lib/libcat.a ../lib/libcata.a ../lib/libcat_nolibc.a \
		../lib/libcat_dbg.a

veryclean: clean
	rm -f makefile
	@echo "makefile destroyed: will be rebuilt next time"
