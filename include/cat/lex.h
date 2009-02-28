/*
   lex.h -- Simple lexical analyzer using simple regexes
   Copyright 2009
   Christopher Telfer
*/

#include <cat/cat.h>
#include <cat/list.h>
#include <cat/match.h>


struct lexer_entry {
	struct list		entry;
	int			token;
	struct rex_pat		pattern;
};


struct lexer { 
	struct list		entries;
	struct raw		input;
	const char *		next_char;
};


enum {
	LEX_END = -1,
	LEX_NOMATCH = -2,
	LEX_ERROR = -3
};


/* instantiate a new lexer */
struct lexer *lex_new();
/* add a pattern and assign it to a token in the lexer */
int  lex_add_entry(struct lexer *lex, const char *pattern, int token);
/* reset the lexer with input to tokenize: must be null terminated */
void lex_reset(struct lexer *lex, const char *string);
/* get the next token along with the string that matched and its length */
/* returns LEX_END if at the end of the input, LEX_NOMATCH if hitting a */
/* input that the lexer is unable to match and LEX_ERROR if an error occurs */
int  lex_next_token(struct lexer *lex, const char **string, int *len);
/* free the lexer and release all associated resources */
void lex_destroy(struct lexer *lex);
