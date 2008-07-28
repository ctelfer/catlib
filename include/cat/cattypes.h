#ifndef __cat_cattypes_h
#define __cat_cattypes_h

#include <cat/cat.h>

#ifdef CAT_HAS_FIXED_WIDTH 

#ifdef CAT_USE_STDINT_TYPES
#include <stdint.h>
#elif defined(CAT_USE_POSIX_TYPES)
#include <sys/types.h>
#else /* no defined headers: take guess */

/* reasonable guesses for a 8, 16, 32 or some 64 bit architectures */
/* While these will always be at least wide enough, they may be too wide. */

/* Allow overrides during compilation */
#ifndef CAT_U8_T
#define CAT_U8_T unsigned char
#endif /* CAT_U8_T */
#ifndef CAT_S8_T
#define CAT_S8_T char
#endif /* CAT_S8_T */
#ifndef CAT_U16_T
#define CAT_U16_T unsigned short
#endif /* CAT_U16_T */
#ifndef CAT_S16_T
#define CAT_S16_T short
#endif /* CAT_S16_T */
#ifndef CAT_U32_T
#define CAT_U32_T unsigned long
#endif /* CAT_U32_T */
#ifndef CAT_S32_T
#define CAT_S32_T long
#endif /* CAT_S32_T */

#ifdef CAT_64BIT
#ifndef CAT_U64_T
#define CAT_U64_T unsigned long long
#endif /* CAT_U64_T */
#ifndef CAT_S64_T
#define CAT_S64_T long long
#endif /* CAT_S64_T */
#endif /* CAT_64BIT */


typedef CAT_U8_T	uint8_t;
typedef CAT_S8_T	int8_t;
typedef CAT_U16_T	uint16_t;
typedef CAT_S16_T	int16_t;
typedef CAT_U32_T	uint32_t;
typedef CAT_S32_T	int32_t;
#ifdef CAT_64BIT
typedef CAT_U64_S	uint64_t;
typedef CAT_S64_T	int64_t;
#endif /* CAT_64BIT */

#endif /* no defined headers: take guess*/

#endif /* CAT_HAS_FIXED_WIDTH */

/* convenient clear pseudonym */
typedef unsigned char byte_t;

#endif /* __cat_cattypes_h */
