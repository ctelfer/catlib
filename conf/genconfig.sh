#!/bin/sh

. ./build_system.conf


TESTS=is64


cleanup() {
	ECODE=$1
	for t in $TESTS
	do
		rm -f $t $t.c
	done
	if [ $ECODE -ne 0 ]
	then
		rm config.h
	fi
	exit $ECODE
}


cat > is64.c <<IS64
#include <stdio.h>
int main() { 
  printf("#ifndef CAT_64BIT\n");
  printf("#define CAT_64BIT %d\n", sizeof(long) >= 8); 
  printf("#endif\n");
  return 0; 
}
IS64


# -- Build Tests -- 

for t in $TESTS
do
	$CC -o $t $t.c 
	if [ $? -ne 0 ]
	then 
		echo Unable to compile "'$t.c'"
		cleanup 1
	fi
done


# -- Generate header -- 

rm -f config.h
cat > config.h <<HDR
#ifndef __config_h
#define __config_h
/* config.h -- automatically generated, do not edit */

HDR

if [ -f config_override.h ]
then
	cat config_override.h >> config.h
fi


# -- Run tests -- 

echo "Config tests: $TESTS"
for t in $TESTS
do
	echo Running ./$t
	./$t >> config.h
	if [ $? -ne 0 ]
	then
		echo Error running $t >&2
		cleanup 1
	fi
	echo >> config.h
done

echo "#endif /* __config_h */" >> config.h

echo Successfully built config.h: cleaning up

cleanup 0
