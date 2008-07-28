/*
 * regex.c -- simplified UNIX regular expressions
 *
 * by Christopher Adam Telfer
 *
 * Copyright 2003, See accompanying license
 *
 */

#include <cat/cat.h>

#if defined(CAT_USE_STDLIB) && CAT_USE_STDLIB

#include <cat/raw.h>
#include <cat/regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int re_find(const char *s, const char *p, regmatch_t *m, int nm)
{
regex_t regex;
int rv; 

  Assert(s);
  Assert(p);
  Assert(nm >= 0);
  if ( nm ) {
    int i;
    Assert(m);
    for ( i = 0 ; i < nm ; ++i )
      Assert((m[i].rm_so < 0) || (m[i].rm_eo >= m[i].rm_so));
  }

  if ( regcomp(&regex, p, REG_EXTENDED) )
    return -1; 

  if ( m ) 
    rv = regexec(&regex, s, nm, m, 0);
  else
    rv = regexec(&regex, s, 0, NULL, REG_NOSUB);

  regfree(&regex);

  if ( !rv ) 
    return 1;
  else if ( rv == REG_NOMATCH ) 
    return 0;
  else
    return -1;
}


int re_lastrep(const char *r)
{
int val, n = 0;
char *p;

  Assert(r);

  /* find out the maximum replacement value */
  for ( ; *r ; ++r ) 
    if ( ( *r == '\\' ) && (*(r+1) >= '0') && (*(r+1) <= '9')) 
    { 
      ++r; 
      val = (int) strtoul(r, &p, 10);
      if ( val < 0 )
        return -1;
      r = p;
      n = (n < val) ? val : n; 
    }

  return n;
} 


int re_replen(const char *s, const char *r, regmatch_t *m)
{
unsigned int newlen, val;
char *p; 

  Assert(s);
  Assert(r);
  Assert(m);

  /* now find out how long the replacement string will be */
  /* matches[0] should contain offsets for the entire pattern */
  newlen = strlen(s) + 1 - (m[0].rm_eo - m[0].rm_so) + strlen(r);

  /* now find each replacement string */
  for ( ; *r ; ++r )
    if ( ( *r == '\\' ) && (*(r+1) >= '0') && (*(r+1) <= '9') ) 
    {
      ++r; 

      val = (int)strtoul(r, &p, 10);

      /* subtract out the \ and all the digits */
      newlen -= 1 + (p - r);

      /* advance r */
      r = p;

      /* add in the length of the replacement (cast do int because of  */
      /* stupidity!  Bad OpenBSD implementation.  No cookie!) */
      if ( (int)(m[val].rm_so) >= 0 )
        newlen += m[val].rm_eo - m[val].rm_so;
    }

  return newlen;
}



/* you'd better use re_replen to make sure t is long enough! */
int re_replace(const char *s, const char *r, const regmatch_t *m, char *d)
{
int val, off;
char *p;

  Assert(s);
  Assert(r);
  Assert(m);
  Assert(d);

  /* copy in to the beginning of the match */
  memmove(d, s, m[0].rm_so);
  d += m[0].rm_so;

  while ( *r )
    if ( ( *r == '\\' ) && (*(r+1) >= '0') && (*(r+1) <= '9') ) 
    {
      ++r;
      val = (int)strtoul(r, &p, 10);
      r = p;

      /* If there was no sub match just go on */
      if ( m[val].rm_so < 0 )
	continue;

      off = m[val].rm_eo - m[val].rm_so;
      memmove(d, s + m[val].rm_so, off);
      d += off;
    } else {
      *d = *r;
      ++d;
      ++r;
    }

  /* copy last bit of info and null terminate */
  off = strlen(s) - m[0].rm_eo;
  memmove(d, s + m[0].rm_eo, off);
  d += off; 
  *d = '\0'; 

  return 0;
}


#include <cat/err.h>


static void *Malloc(size_t s)
{
	void *m;

	if ( !(m = malloc(s)) )
		errsys("re_sr: ");
	return m;
}



char * re_sr(const char *s, const char *p, const char *r)
{
  regmatch_t match[CAT_RE_MAXSUB+1];
  int nm, rv;
  char *newstr;

  Assert(s);
  Assert(p);
  Assert(r);

  nm = re_lastrep(r);
  if ( ( nm < 0 ) || ( nm > CAT_RE_MAXSUB ) ) 
    return NULL;
  rv = re_find(s, p, match, nm+1);
  if ( rv < 0 ) 
    return NULL;
  if ( rv == 0 ) 
    return strcpy(Malloc(strlen(s)+1), s);
  else
  {
    newstr = Malloc(re_replen(s, r, match));
    re_replace(s, r, match, newstr);
    return newstr;
  }
}

#endif /* CAT_USE_STDLIB */
