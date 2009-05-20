#include <cat/lex.h>
#include <cat/err.h>
#include <cat/stduse.h>
#include <stdio.h>
#include <string.h>

enum token_e { 
	WHITESPACE, NEWLINE, NUMBER, PLUS, MINUS, TIMES, DIVIDE, LPAREN,
	RPAREN
};

const char *tok2name[] = { 
	"whitespace", "newline", "number", "plus", "minus", "times", "divide",
	"left parentheses", "right parentheses"
};


void Lex_add(struct lexer *lex, const char *pat, int tok)
{
	if (lex_add_entry(lex, pat, tok) < 0)
		err("Error adding token '%d' with pattern '%s'", tok, pat);
}

int main(int argc, char *argv[]) 
{
	struct lexer *lex; 
	char buffer[65563], *bp = buffer;
	int tok;
	const char *tokp;
	int toklen;

	lex = lex_new(&estdmem);
	Lex_add(lex, "[ \t]+", WHITESPACE);
	Lex_add(lex, "[\n\r]+", NEWLINE);
	Lex_add(lex, "-?[0-9]+", NUMBER);
	Lex_add(lex, "\\+", PLUS);
	Lex_add(lex, "-", MINUS);
	Lex_add(lex, "\\*", TIMES);
	Lex_add(lex, "/", DIVIDE);
	Lex_add(lex, "\\(", LPAREN);
	Lex_add(lex, "\\)", RPAREN);

	while ( bp < buffer + sizeof(buffer) - 1 && 
		fgets(bp, sizeof(buffer) - (bp - buffer), stdin) ) { 
		bp += strlen(bp);
	}

	lex_reset(lex, buffer);

	while ( (tok = lex_next_token(lex, &tokp, &toklen)) >= 0 ) {
		printf("Token %s", tok2name[tok]);
		if ( tok == NUMBER ) { 
			char argbuf[256] = { 0 };
			if ( toklen > sizeof(argbuf) - 1 )
				toklen = sizeof(argbuf) - 1;
			memcpy(argbuf, tokp, toklen);
			printf(": '%s'", argbuf);
		}
		putchar('\n');
	}

	if ( tok == LEX_END ) { 
		printf("Normal token stream end\n");
	} else if ( tok == LEX_NOMATCH ) { 
		printf("No match for more tokens\n");
	} else if ( tok == LEX_ERROR ) { 
		printf("Error in token match\n");
	} else {
		printf("Unknown return code: %d\n", tok);
	}

	return 0;
}
