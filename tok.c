/* the preprocessor and tokenizer */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eqn.h"

#define T_BIN(c1, c2)		(((c1) << 8) | (c2))
#define T_SEP			"^~{}\"\n\t "
#define T_SOFTSEP		(T_SEP "=:|.+-*/\\,()[]<>!")

static char *kwds[] = {
	"fwd", "down", "back", "up",
	"bold", "italic", "roman", "font", "fat", "size",
	"bar", "dot", "dotdot", "dyad", "hat", "under", "vec", "tilde",
	"left", "right", "over", "sqrt", "sub", "sup", "from", "to", "vcenter",
	"delim", "define",
	"gfont", "grfont", "gbfont",
	"mark", "lineup",
};

static int tok_eqen;		/* non-zero if inside .EQ/.EN */
static int tok_line;		/* inside inline eqn block */
static int tok_part;		/* partial line with inline eqn blocks */
static char tok[LNLEN];		/* current token */
static char tok_prev[LNLEN];	/* previous token */
static int tok_curtype;		/* type of current token */
static int tok_cursep;		/* current character is a separator */
static int tok_prevsep;		/* previous character was a separator */
static int eqn_beg, eqn_end;	/* inline eqn delimiters */

/* return zero if troff request .ab is read */
static int tok_req(int a, int b)
{
	int eqln[LNLEN];
	int i = 0;
	int ret = 0;
	eqln[i++] = in_next();
	if (eqln[i - 1] != '.')
		goto failed;
	eqln[i++] = in_next();
	while (eqln[i - 1] == ' ')
		eqln[i++] = in_next();
	if (eqln[i - 1] != a)
		goto failed;
	eqln[i++] = in_next();
	if (eqln[i - 1] != b)
		goto failed;
	ret = 1;
failed:
	while (i > 0)
		in_back(eqln[--i]);
	return ret;
}

/* read .EN */
static int tok_en(void)
{
	return tok_req('E', 'N');
}

/* does the line start with eq */
static int tok_eq(char *s)
{
	if (*s++ != '.')
		return 0;
	while (isspace(*s))
		s++;
	return s[0] == 'E' && s[1] == 'Q';
}

/* read the next input character */
static int tok_next(void)
{
	int c;
	if (!tok_eqen && !tok_line)
		return 0;
	c = in_next();
	if (tok_eqen && c == '\n' && tok_en())
		tok_eqen = 0;
	if (tok_line && c == eqn_end) {
		tok_line = 0;
		return 0;
	}
	return c;
}

/* push back the last character read */
static void tok_back(int c)
{
	if (tok_eqen || tok_line)
		in_back(c);
}

/* read the next word */
static void tok_preview(char *s)
{
	int c = in_next();
	int n = 0;
	while (c > 0 && !strchr(T_SEP, c) && (!tok_line || c != eqn_end)) {
		s[n++] = c;
		c = in_next();
	}
	s[n] = '\0';
	in_back(c);
}

/* push back the given word */
static void tok_unpreview(char *s)
{
	int n = strlen(s);
	while (n > 0)
		in_back((unsigned char) s[--n]);
}

/* read a keyword; return zero on success */
static int tok_keyword(void)
{
	int i;
	tok_preview(tok);
	for (i = 0; i < LEN(kwds); i++)
		if (!strcmp(kwds[i], tok))
			return 0;
	tok_unpreview(tok);
	return 1;
}

/* read the next argument of a macro call; return zero if read a ',' */
static int tok_readarg(struct sbuf *sbuf)
{
	int c = in_next();
	while (c > 0 && c != ',' && c != ')') {
		sbuf_add(sbuf, c);
		c = in_next();
	}
	return c == ',' ? 0 : 1;
}

/* expand a macro; return zero on success */
static int tok_expand(void)
{
	char *args[10] = {NULL};
	struct sbuf sbufs[10];
	int i, n = 0;
	int pbeg;
	tok_preview(tok);
	if (!in_expand(tok, NULL))
		return 0;
	pbeg = in_macrocall(tok);
	if (pbeg) {
		tok_unpreview(tok + pbeg + 1);
		tok[pbeg] = '\0';
		while (n <= 9) {
			sbuf_init(&sbufs[n]);
			if (tok_readarg(&sbufs[n++]))
				break;
		}
		for (i = 0; i < n; i++)
			args[i] = sbuf_buf(&sbufs[i]);
		in_expand(tok, args);
		for (i = 0; i < n; i++)
			sbuf_done(&sbufs[i]);
		return 0;
	}
	tok_unpreview(tok);
	return 1;
}

/* read until .EQ or eqn_beg */
int tok_eqn(void)
{
	char ln[LNLEN];
	char *s = ln;
	int c;
	tok_cursep = 1;
	while ((c = in_next()) > 0) {
		if (c == eqn_beg) {
			*s = '\0';
			printf(".%s %s \"%s\n",
				tok_part ? "as" : "ds", EQNS, ln);
			tok_part = 1;
			tok_line = 1;
			return 0;
		}
		*s++ = c;
		if (c == '\n') {
			*s = '\0';
			s = ln;
		}
		if (c == '\n' && !tok_part) {
			printf("%s", ln);
			if (tok_eq(ln) && !tok_en()) {
				tok_eqen = 1;
				return 0;
			}
		}
		if (c == '\n' && tok_part) {
			printf("\\*%s%s", escarg(EQNS), ln);
			tok_part = 0;
		}
	}
	return 1;
}

/* collect the output of this eqn block */
void tok_eqnout(char *s)
{
	char post[128];
	sprintf(post, "\\s[\\n[%s]]\\f[\\n[%s]]", EQNSZ, EQNFN);
	if (!tok_part)
		printf("%s%s\n", s, post);
	else
		printf(".as %s \"%s%s\n", EQNS, s, post);
}

/* return the length of a utf-8 character based on its first byte */
static int utf8len(int c)
{
	if (c > 0 && c <= 0x7f)
		return 1;
	if (c >= 0xfc)
		return 6;
	if (c >= 0xf8)
		return 5;
	if (c >= 0xf0)
		return 4;
	if (c >= 0xe0)
		return 3;
	if (c >= 0xc0)
		return 2;
	return c != 0;
}

/* return the type of a token */
static int char_type(char *s)
{
	int c = (unsigned char) s[0];
	if (isdigit(c))
		return T_NUMBER;
	if (c == '"')
		return T_STRING;
	if (def_punc(s))
		return T_PUNC;
	if (def_binop(s))
		return T_BINOP;
	if (def_relop(s))
		return T_RELOP;
	if (def_left(s))
		return T_LEFT;
	if (def_right(s))
		return T_RIGHT;
	if (c == '~' || c == '^')
		return T_GAP;
	if (ispunct(c))
		return T_ORD;
	return T_LETTER;
}

/* read the next token */
static int tok_read(void)
{
	char *s = tok;
	int c, c2;
	int i;
	*s = '\0';
	c = tok_next();
	if (c <= 0)
		return 1;
	tok_prevsep = tok_cursep;
	tok_cursep = !!strchr(T_SEP, c);
	if (c == ' ' || c == '\n') {
		while (c > 0 && (c == ' ' || c == '\n'))
			c = tok_next();
		tok_back(c);
		*s++ = ' ';
		*s = '\0';
		tok_curtype = T_SPACE;
		return 0;
	}
	if (c == '\t') {
		*s++ = '\t';
		*s = '\0';
		tok_curtype = T_TAB;
		return 0;
	}
	if (tok_prevsep) {
		if (c == '$') {
			c2 = tok_next();
			if (c2 >= '1' && c2 <= '9' && !in_arg(c2 - '0'))
				return tok_read();
			tok_back(c2);
		}
		tok_back(c);
		if (!tok_keyword()) {
			tok_curtype = T_KEYWORD;
			tok_cursep = 1;
			return 0;
		}
		if (!tok_expand()) {
			tok_cursep = 1;
			return tok_read();
		}
		c = tok_next();
	}
	if (strchr(T_SOFTSEP, c)) {
		*s++ = c;
		if (c == '\\') {
			c = tok_next();
			if (c == '(') {
				*s++ = c;
				*s++ = tok_next();
				*s++ = tok_next();
			} else if (c == '[') {
				while (c && c != ']') {
					*s++ = c;
					c = tok_next();
				}
				*s++ = ']';
			}
		} else if (c == '"') {
			c = tok_next();
			while (c > 0 && c != '"') {
				if (c == '\\') {
					c2 = tok_next();
					if (c2 == '"')
						c = '"';
					else
						tok_back(c2);
				}
				*s++ = c;
				c = tok_next();
			}
			*s++ = '"';
		} else {
			/* two-character operators */
			c2 = tok_next();
			switch (T_BIN(c, c2)) {
			case T_BIN('<', '='):
			case T_BIN('>', '='):
			case T_BIN('=', '='):
			case T_BIN('!', '='):
			case T_BIN('~', '='):
			case T_BIN('>', '>'):
			case T_BIN('<', '<'):
			case T_BIN(':', '='):
			case T_BIN('-', '>'):
			case T_BIN('<', '-'):
			case T_BIN('-', '+'):
				*s++ = c2;
				break;
			default:
				tok_back(c2);
			}
		}
		*s = '\0';
		tok_curtype = char_type(tok);
		return 0;
	}
	*s++ = c;
	i = utf8len(c);
	while (--i > 0)
		*s++ = tok_next();
	*s = '\0';
	tok_curtype = char_type(tok);
	return 0;
}

/* current token */
char *tok_get(void)
{
	return tok[0] ? tok : NULL;
}

/* current token type */
int tok_type(void)
{
	return tok[0] ? tok_curtype : 0;
}

/* return nonzero if current token is a separator */
int tok_sep(void)
{
	return !tok_get() || strchr(T_SEP, (unsigned char) tok_get()[0]) ||
		tok_curtype == T_KEYWORD;
}

/* read the next token, return the previous */
char *tok_pop(void)
{
	strcpy(tok_prev, tok);
	tok_read();
	return tok_prev[0] ? tok_prev : NULL;
}

/* like tok_pop() but read the next T_SEP-separated token */
char *tok_poptext(void)
{
	while (tok_type() == T_SPACE)
		tok_read();
	tok_prev[0] = '\0';
	do {
		strcat(tok_prev, tok);
		tok_read();
	} while (tok[0] && !tok_sep());
	return tok_prev[0] ? tok_prev : NULL;
}

/* skip spaces */
static void tok_blanks(void)
{
	while (tok_type() == T_SPACE)
		tok_pop();
}

/* if the next token is s, return zero and skip it */
int tok_jmp(char *s)
{
	tok_blanks();
	if (tok_get() && !s[1] && strchr("{}~^\t", s[0]) && !strcmp(s, tok_get())) {
		tok_pop();
		return 0;
	}
	if (tok_type() != T_KEYWORD || !tok_get() || strcmp(s, tok_get()))
		return 1;
	tok_pop();
	return 0;
}

/* read delim command */
void tok_delim(void)
{
	char delim[NMLEN];
	tok_preview(delim);
	if (!strcmp("off", delim)) {
		eqn_beg = 0;
		eqn_end = 0;
	} else {
		eqn_beg = delim[0];
		eqn_end = delim[1];
	}
}

/* read macro definition */
static void tok_macrodef(struct sbuf *def)
{
	int c;
	int delim;
	c = in_next();
	while (c > 0 && isspace(c))
		c = in_next();
	delim = c;
	c = in_next();
	while (c > 0 && c != delim) {
		sbuf_add(def, c);
		c = in_next();
	}
}

/* read the next macro command */
void tok_macro(void)
{
	char name[NMLEN];
	struct sbuf def;
	tok_preview(name);
	sbuf_init(&def);
	tok_macrodef(&def);
	in_define(name, sbuf_buf(&def));
	sbuf_done(&def);
}
