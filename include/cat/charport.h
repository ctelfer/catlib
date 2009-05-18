#ifndef __cat_charport_h
#define __cat_charport_h
#include <cat/cat.h>

#define READCHAR_CHAR      0
#define READCHAR_NONE      1
#define READCHAR_END       2
#define READCHAR_ERROR     3

struct charport;
typedef int (*readchar_f)(struct charport *cp, char *out);

struct charport {
  readchar_f            read;
};

int readchar(struct charport *cp, char *ch);


struct string_charport {
  struct charport       cp;
  const char *          start;
  const char *          end;
  const char *          cur;
};

void string_charport_init(struct string_charport *scp, const char *s);
void string_charport_reset(struct string_charport *scp);

void null_charport_init(struct charport *cp);
extern struct charport null_charport;

#endif /* __cat_charport_h */
