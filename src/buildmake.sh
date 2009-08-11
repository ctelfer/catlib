#!/bin/sh
if [ ! -f Makefile ] 
then
	rm -f ../include/cat/config.h
	(cd ../conf ; ./genconfig.sh ) && cp ../conf/config.h ../include/cat/config.h
	make -f Makefile.tmpl .depend
fi
