#!/bin/sh

set -e 

if ! which echo >/dev/null ; then exit 2 ; fi
if ! which rm >/dev/null ; then echo "'rm' not found in path!" >&2 ; exit 1 ; fi
if ! which cp >/dev/null ; then echo "'cp' not found in path!" >&2 ; exit 1 ; fi
if ! which mv >/dev/null ; then echo "'mv' not found in path!" >&2 ; exit 1 ; fi
if ! which sed >/dev/null ; then echo "'sed' not found in path!" >&2 ; exit 1 ; fi

rm -f build_system.conf

if [ -f genconfig_override.conf ]
then
	. genconfig_override.conf
fi

if [ -f build_system_override.conf ]
then
	cp build_system_override.conf build_system.conf
else
	echo "# Generated build configuration" > build_system.conf

	if [ ! -z "$CC" ]
	then
		echo "CC=\"$CC\"" >> build_system.conf
	elif which cc >/dev/null ; then
		# maybe pcc
		echo "CC="`which cc` >> build_system.conf
		if [ -z "$ISYSTEM" ]; then
			echo "ISYSTEM=\"-isystem /usr/include\"" >> build_system.conf
		fi
		if [ -z "$NOSTD" ]; then
			echo "NOSTD=-nostdlib" >> build_system.conf
		fi
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
		if [ -z "$ISYSTEM" ]; then
			echo "ISYSTEM=\"-isystem /usr/include\"" >> build_system.conf
		fi
		if [ -z "$NOSTD" ]; then
			echo "NOSTD=-nostdlib" >> build_system.conf
		fi
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
		if [ -z "$ISYSTEM" ]; then
			echo "ISYSTEM=" >> build_system.conf
		fi
		if [ -z "$NOSTD" ]; then
			echo "NOSTD=" >> build_system.conf
		fi
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

	if [ ! -z "$ISYSTEM" ]; then
		echo "ISYSTEM=\"$ISYSTEM\"" >> build_system.conf
	fi

	if [ ! -z "$NOSTD" ]; then
		echo "NOSTD=\"$NOSTD\"" >> build_system.conf
	fi

	if [ ! -z "$CCXFLAGS" ]; then
		echo "CCXFLAGS=\"$CCXFLAGS\"" >> build_system.conf
	fi

fi

echo Successfully generated build_system.conf

. ./build_system.conf


rm -f config.h platform.conf

TESTFILES="simple.c stdlib.c has_intptr_t.c has_uintptr_t.c int_size_4.c
	   long_size_4.c is64.c has_llong.c long_size_8.c llong_size_8.c"

cleanup() {
	ECODE=$1
	if [ $ECODE -ne 0 ]
	then
		rm -f config.h
		rm -f platform.conf 
	else
		rm -f $TESTFILES
	fi
	exit $ECODE
}


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


cat > simple.c <<SIMPLE
#include <stddef.h>
int main() { return 0; }
SIMPLE

if ! $CC -o /dev/null simple.c $NOSTD $CCXFLAGS > /dev/null 2>&1 
then
	echo "unable to build simple.c"
	cleanup 1
fi


cat > stdlib.c <<STDLIB
#include <stdio.h>
int main() { printf("Hello World!\n"); return 0; }
STDLIB
if $CC -o /dev/null stdlib.c $CCXFLAGS > /dev/null 2>&1 
then
	echo "TARGETS=../lib/libcat.a \\" >> platform.conf
	echo "	../lib/libcata.a \\" >> platform.conf
	echo "	../lib/libcat_dbg.a \\" >> platform.conf
	echo "	../lib/libcat_nolibc.a">> platform.conf
	echo >> platform.conf

cat > has_intptr_t.c <<IPT
#include <stdint.h>
int main() { intptr_t ipt; return 0; }
IPT
	if $CC -o /dev/null has_intptr_t.c $CCXFLAGS > /dev/null 2>&1 
	then
		echo "#ifndef CAT_HAS_INTPTR_T" >> config.h
		echo "#define CAT_HAS_INTPTR_T 1" >> config.h
		echo "#endif /* CAT_HAS_INTPTR_T */" >> config.h
	else
		echo "#ifndef CAT_HAS_INTPTR_T" >> config.h
		echo "#define CAT_HAS_INTPTR_T 0" >> config.h
		echo "#endif /* CAT_HAS_INTPTR_T */" >> config.h
	fi


cat > has_uintptr_t.c <<UIPT
#include <stdint.h>
int main() { uintptr_t uipt; return 0; }
UIPT
	if $CC -o /dev/null has_uintptr_t.c $CCXFLAGS > /dev/null 2>&1 
	then
		echo "#ifndef CAT_HAS_UINTPTR_T" >> config.h
		echo "#define CAT_HAS_UINTPTR_T 1" >> config.h
		echo "#endif /* CAT_HAS_UINTPTR_T */" >> config.h
	else
		echo "#ifndef CAT_HAS_UINTPTR_T" >> config.h
		echo "#define CAT_HAS_UINTPTR_T 0" >> config.h
		echo "#endif /* CAT_HAS_UINTPTR_T */" >> config.h
	fi


else
	echo "TARGETS=../lib/libcat_nolibc.a" >> platform.conf

	echo "#ifndef CAT_HAS_DIV" >> config.h
	echo "#define CAT_HAS_DIV 0 " >> config.h
	echo "#endif /* CAT_HAS_DIV */" >> config.h
	echo "#ifndef CAT_HAS_FLOAT" >> config.h
	echo "#define CAT_HAS_FLOAT 0 " >> config.h
	echo "#endif /* CAT_HAS_FLOAT */" >> config.h
fi


# Find 32-bit integral type
cat > int_size_4.c <<INTSIZE4
enum { FOO = 1 / (sizeof(int) == 4) };
int main() { return 0; }
INTSIZE4
if $CC -o /dev/null int_size_4.c $NOSTD $CCXFLAGS > /dev/null 2>&1 
then
	echo "#ifndef CAT_S32_T" >> config.h
	echo "#define CAT_S32_T int" >> config.h
	echo "#endif /* CAT_S32_T */" >> config.h
	echo "#ifndef CAT_U32_T" >> config.h
	echo "#define CAT_U32_T unsigned int" >> config.h
	echo "#endif /* CAT_U32_T */" >> config.h
else

cat > long_size_4.c <<LONGSIZE4
enum { FOO = 1 / (sizeof(long) == 4) };
int main() { return 0; }
LONGSIZE4
	if $CC -o /dev/null long_size_4.c $NOSTD $CCXFLAGS > /dev/null 2>&1 
	then
		echo "#ifndef CAT_S32_T" >> config.h
		echo "#define CAT_S32_T long" >> config.h
		echo "#endif /* CAT_S32_T */" >> config.h
		echo "#ifndef CAT_U32_T" >> config.h
		echo "#define CAT_U32_T unsigned long" >> config.h
		echo "#endif /* CAT_U32_T */" >> config.h
	else
		echo "Unable to find 32-bit type!"
		cleanup 1
	fi
fi


# Test for long long
cat > has_llong.c <<HASLLONG
long long foo;
int main() { return 0; }
HASLLONG
if $CC -o /dev/null has_llong.c $NOSTD $CCXFLAGS > /dev/null 2>&1 
then
	echo "#if !CAT_ANSI89" >> config.h
	echo "#ifndef CAT_HAS_LONGLONG" >> config.h
	echo "#define CAT_HAS_LONGLONG 1" >> config.h
	echo "#endif /* CAT_HAS_LONGLONG */" >> config.h
	echo "#endif /* !CAT_ANSI89" */ >> config.h
	HAS_LONG_LONG=1
else
	echo "#if !CAT_ANSI89" >> config.h
	echo "#ifndef CAT_HAS_LONGLONG" >> config.h
	echo "#define CAT_HAS_LONGLONG 0" >> config.h
	echo "#endif /* CAT_HAS_LONGLONG */" >> config.h
	echo "#endif /* !CAT_ANSI89" */ >> config.h
	HAS_LONG_LONG=0
fi

# Test for 64-bit architecture
cat > is64.c <<IS64
enum { FOO = 1 / (sizeof(long) >= 8 || sizeof(void*) >= 8) };
int main() { return 0; }
IS64
if [ $HAS_LONG_LONG -eq 1 ] || 
   $CC -o /dev/null $NOSTD is64.c $CCXFLAGS > /dev/null 2>&1
then
	# Find 64-bit integer types
cat > long_size_8.c <<LONGSIZE8
enum { FOO = 1 / (sizeof(long) == 8) };
int main() { return 0; }
LONGSIZE8
	if $CC -o /dev/null long_size_8.c $NOSTD $CCXFLAGS > /dev/null 2>&1 
	then
		echo "#ifndef CAT_64BIT" >> config.h
		echo "#define CAT_64BIT 1" >> config.h
		echo "#endif /* CAT_64BIT */" >> config.h
		echo "#ifndef CAT_S64_T" >> config.h
		echo "#define CAT_S64_T long" >> config.h
		echo "#endif /* CAT_S64_T */" >> config.h
		echo "#ifndef CAT_U64_T" >> config.h
		echo "#define CAT_U64_T unsigned long" >> config.h
		echo "#endif /* CAT_U64_T */" >> config.h
	elif [ $HAS_LONG_LONG -eq 1 ]; then
			
cat > llong_size_8.c <<LLONGSIZE8
enum { FOO = 1 / (sizeof(long long) == 8) };
int main() { return 0; }
LLONGSIZE8
		if [ $HAS_LONG_LONG -eq 1 ] &&
		   $CC -o /dev/null llong_size_8.c $NOSTD $CCXFLAGS > /dev/null 2>&1 
		then
			# CAT_64BIT is conditional on not having long long
			# This test is needed for ANSI-89 builds where long
			# is not 8-bytes wide.
			echo "#if CAT_HAS_LONGLONG" >> config.h
			echo "#ifndef CAT_64BIT" >> config.h
			echo "#define CAT_64BIT 1" >> config.h
			echo "#endif /* CAT_64BIT */" >> config.h
			echo "#ifndef CAT_S64_T" >> config.h
			echo "#define CAT_S64_T long long" >> config.h
			echo "#endif /* CAT_S64_T */" >> config.h
			echo "#ifndef CAT_U64_T" >> config.h
			echo "#define CAT_U64_T unsigned long long" >> config.h
			echo "#endif /* CAT_U64_T */" >> config.h
			echo "#endif /* CAT_HAS_LONGLONG */" >> config.h
		else
			echo "Unable to find 64-bit type on 64-bit arch"
			cleanup 1
		fi

	else
		echo "Unable to find 64-bit type on 64-bit arch"
		cleanup 1
	fi

else
	echo "#ifndef CAT_64BIT" >> config.h
	echo "#define CAT_64BIT 0" >> config.h
	echo "#endif /* CAT_64BIT*/" >> config.h
fi


echo "#endif /* __config_h */" >> config.h
echo "" >> platform.conf

echo Successfully built config.h and platform.conf: cleaning up

cleanup 0
