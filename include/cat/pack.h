/*
 * cat/pack.h -- functions to pack and unpack values into byte arrays
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, 2004, 2005 See accompanying license
 *
 */

#ifndef __cat_pack_h
#define __cat_pack_h

#include <cat/cat.h>

/* 
   "eEbhwjBHWJr"

   e - little endian
   E - big endian
   b - byte (8 bits)
   h - half-word (16 bits)
   w - word (32 bits)
   j - jumbo word (64 bits)
   B - signed 8 bit value 
   H - signed 16 bit value
   W - signed 32 bit value
   J - signed 64 bit value
   r - (struct raw *) follows (pack only)

   Prefix with a number to indicate a count.  In this case source or 
   destination operand is an array of the type indicated.
*/

size_t pack(void * buf, size_t len, const char *fmt, ... ); 
size_t unpack(void * buf, size_t len, const char *fmt, ... );
size_t packlen(const char *fmt, ... );

#define PSIZ_BYTE	1
#define PSIZ_HALF	2
#define PSIZ_WORD	4
#define PSIZ_JUMBO	8

#ifdef CAT_PSIZ_MAX
#define PSIZ_MAX	CAT_PSIZ_MAX
#else  /* CAT_PSIZ_MAX */
#define PSIZ_MAX	0x7fffffff
#endif /* CAT_PSIZ_MAX */

#endif /* __cat_pack_h */
