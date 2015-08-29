#!/bin/sh

set -e 

if ! which echo >/dev/null ; then exit 2 ; fi
if ! which rm >/dev/null ; then echo "'rm' not found in path!" >&2 ; exit 1 ; fi
if ! which cp >/dev/null ; then echo "'cp' not found in path!" >&2 ; exit 1 ; fi
if ! which mv >/dev/null ; then echo "'mv' not found in path!" >&2 ; exit 1 ; fi
if ! which sed >/dev/null ; then echo "'sed' not found in path!" >&2 ; exit 1 ; fi

rm -f build_system.conf

if [ -f build_system_override.conf ]
then
	cp build_system_override.conf build_system.conf
else
	echo "# Generated build configuration" > build_system.conf

	if which cc >/dev/null ; then
		# maybe pcc
		echo "CC="`which cc` >> build_system.conf
		echo "ISYSTEM=\"-isystem /usr/include\"" >> build_system.conf
		echo "NOSTD=-nostdlib" >> build_system.conf
		if which ar >/dev/null ; then
			echo "AR="`which ar` >> build_system.conf
		else
			echo "Couldn't find 'ar'!  Aboring..." >&2
			exit 1
		fi
		if which ranlib >/dev/null ; then
			echo "RANLIB="`which ranlib` >> build_system.conf
		else
			echo "Couldn't find 'ranlib'!  Aboring..." >&2
			exit 1
		fi

	elif which gcc >/dev/null ; then
		# gcc
		echo "CC="`which gcc` >> build_system.conf
		echo "ISYSTEM=\"-isystem /usr/include\"" >> build_system.conf
		echo "NOSTD=-nostdlib" >> build_system.conf
		if which ar ; then
			echo "AR="`which ar` >> build_system.conf
		else
			echo "Couldn't find 'ar'!  Aboring..." >&2
			exit 1
		fi
		if which ranlib ; then
			echo "RANLIB="`which ranlib` >> build_system.conf
		else
			echo "Couldn't find 'ranlib'!  Aboring..." >&2
			exit 1
		fi
	elif which clang >/dev/null ; then
		# clang
		echo "CC="`which clang` >> build_system.conf
		echo "ISYSTEM=" >> build_system.conf
		echo "NOSTD=" >> build_system.conf
		if which llvm-ar ; then
			echo "AR="`which llvm-ar` >> build_system.conf
		else
			echo "Couldn't find 'llvm-ar'!  Aboring..." >&2
			exit 1
		fi
		if which llvm-ranlib ; then
			echo "RANLIB="`which llvm-ranlib` >> build_system.conf
		else
			echo "Couldn't find 'llvm-ranlib'!  Aboring..." >&2
			exit 1
		fi

	else
		echo "Couldn't find C compiler!  Aboring..." >&2
		exit 1
	fi

fi

. ./build_system.conf


rm -f config.h platform.conf


cleanup() {
	ECODE=$1
	if [ $ECODE -ne 0 ]
	then
		rm -f config.h
		rm -f platform.conf 
	else
		rm -f simple.c 
		rm -f is64.c
		rm -f stdlib.c 
	fi
	exit $ECODE
}


cat > simple.c <<SIMPLE
#include <stddef.h>
int main() { 
  return 0;
}
SIMPLE

cat > is64.c <<IS64
#include <stddef.h>
enum { FOO = 1 / (sizeof(size_t) >= 8 || sizeof(long) >= 8) };
int main() { 
  return 0;
}
IS64


cat > stdlib.c <<STDLIB
#include <stdio.h>
int main() { 
  printf("Hello World!\n");
  return 0;
}
STDLIB

# -- Generate header -- 

rm -f config.h
cat > config.h <<HDR
#ifndef __config_h
#define __config_h
/* config.h -- automatically generated, do not edit */

HDR

echo > platform.conf

if [ -f config_override.h ]
then
	cat config_override.h >> config.h
fi


# -- Run tests -- 


if ! $CC -o /dev/null $NOSTD simple.c > /dev/null 2>&1 
then
	echo "unable to build simple.c"
	cleanup 1
fi


echo "#ifndef CAT_64BIT" >> config.h
if $CC -o /dev/null $NOSTD is64.c > /dev/null 2>&1
then
	echo "#define CAT_64BIT 1" >> config.h
else
	echo "#define CAT_64BIT 0" >> config.h
fi
echo "#endif /* CAT_64BIT*/" >> config.h




if $CC -o /dev/null stdlib.c > /dev/null 2>&1 
then
	echo "TARGETS=../lib/libcat.a \\" >> platform.conf
	echo "	../lib/libcata.a \\" >> platform.conf
	echo "	../lib/libcat_dbg.a \\" >> platform.conf
	echo "	../lib/libcat_nolibc.a">> platform.conf
	echo >> platform.conf
else
	echo "TARGETS=../lib/libcat_nolibc.a" >> platform.conf

	echo "#ifndef CAT_HAS_DIV" >> config.h
	echo "#define CAT_HAS_DIV 0 " >> config.h
	echo "#endif /* CAT_HAS_DIV */" >> config.h
	echo "#ifndef CAT_HAS_FLOAT" >> config.h
	echo "#define CAT_HAS_FLOAT 0 " >> config.h
	echo "#endif /* CAT_HAS_FLOAT */" >> config.h
fi


echo "#endif /* __config_h */" >> config.h
echo "" >> platform.conf

echo Successfully built config.h and platform.conf: cleaning up

cleanup 0
