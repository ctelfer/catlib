#!/bin/sh
if [ ! -f Makefile ] 
then
	make -f Makefile.tmpl .depend
fi
