#!/bin/sh
if [ ! -f makefile ] 
then
	rm -f ../include/cat/config.h
	(cd ../conf ; ./genconfig.sh ) && cp ../conf/config.h ../include/cat/config.h
	make -f makefile.tmpl .depend
fi
