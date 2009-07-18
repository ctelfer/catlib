#include <cat/lex.h>
#include <cat/raw.h>
#if CAT_USE_STDLIB
#include <string.h>
#include <stdlib.h>
#else /* CAT_USE_STDLIB */
#include <cat/catstdlib.h>
#endif /* CAT_USE_STDLIB */


#define node_to_lexent(node) container((node), struct lexer_entry, entry)

struct lexer *lex_new(struct memmgr *mm)
{
	struct lexer *lex = mem_get(mm, sizeof(*lex));
	if ( lex == NULL )
		return NULL;
	l_init(&lex->entries);
	lex->input.data = NULL;
	lex->input.len = 0;
	lex->next_char = NULL;
	lex->mm = mm;
	return lex;
}


int lex_add_entry(struct lexer *lex, const char *pattern, int token)
{
	struct lexer_entry *ent;
	struct raw r;
	int rv; 
	if ( token < 0 )
		return -1;
	if ( (ent = mem_get(lex->mm, sizeof(*ent))) == NULL )
		return -1;
	l_init(&ent->entry);
	ent->token = token;
	r.len = strlen(pattern) + 1;
	if ( (r.data = mem_get(lex->mm, r.len + 1)) == NULL ) {
		mem_free(lex->mm, ent);
		return -1;
	}
	*(char *)r.data = '^';
	memcpy((char *)r.data + 1, pattern, r.len);
	rv = rex_init(&ent->pattern, &r, lex->mm, NULL);
	mem_free(lex->mm, r.data);
	if ( rv < 0 ) {
		mem_free(lex->mm, ent);
		return -1;
	}
	l_enq(&lex->entries, &ent->entry);
	return 0;
}


void lex_reset(struct lexer *lex, const char *string)
{
	abort_unless(string);
	str_to_raw(&lex->input, (char *)string, 0);
	lex->next_char = lex->input.data;
}


int lex_next_token(struct lexer *lex, const char **string, int *len)
{
	int rv;
	struct list *node;
	struct lexer_entry *ent;
	struct raw r;
	struct rex_match_loc match;

	if ( lex->next_char == (char *)lex->input.data + lex->input.len )
		return LEX_END;

	r.data = (byte_t*)lex->next_char;
	r.len = lex->input.len - (lex->next_char - (char *)r.data);
	l_for_each(node, &lex->entries) { 
		ent = node_to_lexent(node);
		rv = rex_match(&ent->pattern, &r, &match, 1);
		if ( rv == REX_ERROR )
			return LEX_ERROR;
		if ( rv != REX_MATCH )
			continue;
		abort_unless(match.valid);
		abort_unless(match.start == 0);
		if ( string != NULL )
			*string = lex->next_char;
		if ( len != NULL )
			*len = match.len;
		lex->next_char += match.len;
		return ent->token;
	}

	return LEX_NOMATCH;
}


void lex_destroy(struct lexer *lex)
{
	struct list *node;
	struct memmgr *mm = lex->mm;
	while ( (node = l_deq(&lex->entries)) != NULL ) {
		struct lexer_entry *ent = node_to_lexent(node);
		rex_free(&ent->pattern);
		mem_free(mm, ent);
	}
	mem_free(mm, lex);
}

