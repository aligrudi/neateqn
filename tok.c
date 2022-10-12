/* the preprocessor and tokenizer */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eqn.h"

#define T_BIN(c1, c2)		(((c1) << 8) | (c2))
#define T_SOFTSEP		("^~{}(),\"\n\t =:|.+-*/\\,()[]<>!")
#define ESAVE		"\\E*[.eqnbeg]\\R'" EQNFN "0 \\En(.f'\\R'" EQNSZ "0 \\En(.s'"
#define ELOAD		"\\f[\\En[" EQNFN "0]]\\s[\\En[" EQNSZ "0]]\\E*[.eqnend]"

static char *kwds[] = {
	"fwd", "down", "back", "up",
	"bold", "italic", "roman", "font", "fat", "size",
	"bar", "dot", "dotdot", "dyad", "hat", "under", "vec", "tilde",
	"sub", "sup", "from", "to", "vcenter",
	"left", "right", "over", "sqrt",
	"pile", "lpile", "cpile", "rpile", "above",
	"matrix", "col", "ccol", "lcol", "rcol",
	"delim", "define",
	"gfont", "grfont", "gbfont", "gsize", "set", "chartype",
	"mark", "lineup", "bracketsizes", "bracketpieces", "breakcost",
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
	eqln[i++] = src_next();
	if (eqln[i - 1] != '.')
		goto failed;
	eqln[i++] = src_next();
	while (eqln[i - 1] == ' ' && i < sizeof(eqln) - 4)
		eqln[i++] = src_next();
	if (eqln[i - 1] != a)
		goto failed;
	eqln[i++] = src_next();
	if (eqln[i - 1] != b)
		goto failed;
	ret = 1;
failed:
	while (i > 0)
		src_back(eqln[--i]);
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
	while (isspace((unsigned char) *s))
		s++;
	return s[0] == 'E' && s[1] == 'Q';
}

/* read an lf request */
static int tok_lf(char *s)
{
	if (*s++ != '.')
		return 0;
	while (isspace((unsigned char) *s))
		s++;
	if (*s++ != 'l' || *s++ != 'f')
		return 0;
	while (isspace((unsigned char) *s))
		s++;
	if (isdigit((unsigned char) *s))
		src_lineset(atoi(s));
	return 1;
}

/* read the next input character */
static int tok_next(void)
{
	int c;
	if (!tok_eqen && !tok_line)
		return 0;
	c = src_next();
	if (tok_eqen && c == '\n' && tok_en())
		tok_eqen = 0;
	if (tok_line && (src_top() && c == eqn_end)) {
		tok_line = 0;
		return 0;
	}
	return c;
}

/* push back the last character read */
static void tok_back(int c)
{
	if (tok_eqen || tok_line)
		src_back(c);
}

static int readchar(char *dst)
{
	int c = src_next();
	dst[0] = c;
	if (c == '\\') {
		c = src_next();
		dst[1] = c;
		if (c == '(') {
			dst[2] = src_next();
			dst[3] = src_next();
			return 4;
		}
		if (c == '[') {
			int n = 2;
			while (c > 0 && c != ']') {
				c = src_next();
				dst[n++] = c;
			}
			return n;
		}
		return 2;
	}
	return c > 0;
}

/* read the next word; if opstop, stop at open parenthesis */
static void tok_preview(char *s, int opstop)
{
	int c = src_next();
	int n = 0;
	if (c > 0 && def_chopped(c)) {
		src_back(c);
		n = readchar(s);
	} else {
		while (c > 0 && (!def_chopped(c) && (!opstop || c != '(')) &&
				(!tok_line || (!src_top() || c != eqn_end))) {
			src_back(c);
			n += readchar(s + n);
			c = src_next();
		}
		src_back(c);
	}
	s[n] = '\0';
}

/* push back the given word */
static void tok_unpreview(char *s)
{
	int n = strlen(s);
	while (n > 0)
		src_back((unsigned char) s[--n]);
}

/* read a keyword; return zero on success */
static int tok_keyword(void)
{
	int i;
	tok_preview(tok, 0);
	for (i = 0; i < LEN(kwds); i++)
		if (!strcmp(kwds[i], tok))
			return 0;
	tok_unpreview(tok);
	return 1;
}

/* read the next argument of a macro call; return zero if read a ',' */
static int tok_readarg(struct sbuf *sbuf)
{
	int c = src_next();
	int pdepth = 0;		/* number of nested parenthesis */
	int quotes = 0;		/* inside double quotes */
	while (c > 0 && (pdepth || quotes || (c != ',' && c != ')'))) {
		sbuf_add(sbuf, c);
		if (!quotes && c == ')')
			pdepth++;
		if (!quotes && c == '(')
			pdepth--;
		if (c == '"')
			quotes = 1 - quotes;
		if (c == '\\') {
			sbuf_add(sbuf, c = src_next());
			if (c == '*' || c == 'n')
				sbuf_add(sbuf, c = src_next());
			if (c == '(') {
				sbuf_add(sbuf, c = src_next());
				sbuf_add(sbuf, c = src_next());
			} else if (c == '[') {
				while (c > 0 && c != ']')
					sbuf_add(sbuf, c = src_next());
			}
		}
		c = src_next();
	}
	return c == ',' ? 0 : 1;
}

/* expand a macro; return zero on success */
static int tok_expand(void)
{
	char *args[10] = {NULL};
	struct sbuf sbufs[10];
	int i, n = 0;
	tok_preview(tok, 1);
	if (src_macro(tok)) {
		int c = src_next();
		src_back(c);
		if (c == '(') {		/* macro arguments follow */
			src_next();
			while (n <= 9) {
				sbuf_init(&sbufs[n]);
				if (tok_readarg(&sbufs[n++]))
					break;
			}
		}
		for (i = 0; i < n; i++)
			args[i] = sbuf_buf(&sbufs[i]);
		src_expand(tok, args);
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
	struct sbuf ln;
	int c;
	tok_cursep = 1;
	sbuf_init(&ln);
	while ((c = src_next()) > 0) {
		if (c == eqn_beg) {
			printf(".eo\n");
			printf(".%s %s \"%s\n",
				tok_part ? "as" : "ds", EQNS, sbuf_buf(&ln));
			sbuf_done(&ln);
			printf(".ec\n");
			tok_part = 1;
			tok_line = 1;
			return 0;
		}
		sbuf_add(&ln, c);
		if (c == '\n' && !tok_part) {
			printf("%s", sbuf_buf(&ln));
			tok_lf(sbuf_buf(&ln));
			if (tok_eq(sbuf_buf(&ln)) && !tok_en()) {
				tok_eqen = 1;
				sbuf_done(&ln);
				return 0;
			}
		}
		if (c == '\n' && tok_part) {
			printf(".lf %d\n", src_lineget());
			printf("\\*%s%s", escarg(EQNS), sbuf_buf(&ln));
			tok_part = 0;
		}
		if (c == '\n')
			sbuf_cut(&ln, 0);
	}
	sbuf_done(&ln);
	return 1;
}

/* collect the output of this eqn block */
void tok_eqnout(char *s)
{
	if (!tok_part) {
		printf(".ds %s \"%s%s%s\n", EQNS, ESAVE, s, ELOAD);
		printf(".lf %d\n", src_lineget() - 1);
		printf("\\&\\*%s\n", escarg(EQNS));
	} else {
		printf(".as %s \"%s%s%s\n", EQNS, ESAVE, s, ELOAD);
	}
}

/* return the length of a utf-8 character based on its first byte */
static int utf8len(int c)
{
	if (~c & 0x80)
		return c > 0;
	if (~c & 0x40)
		return 1;
	if (~c & 0x20)
		return 2;
	if (~c & 0x10)
		return 3;
	if (~c & 0x08)
		return 4;
	return 1;
}

/* return the type of a token */
static int char_type(char *s)
{
	int c = (unsigned char) s[0];
	int t;
	if (isdigit(c))
		return T_NUMBER;
	if (c == '"')
		return T_STRING;
	if ((t = def_type(s)) >= 0)
		return t;
	if (c == '~' || c == '^')
		return T_GAP;
	if (ispunct(c) && (c != '\\' || !s[1]))
		return T_ORD;
	return T_LETTER;
}

/* read the next token */
static int tok_read(void)
{
	char *s = tok;
	char *e = tok + sizeof(tok) - 2;
	int c, c2;
	int i;
	*s = '\0';
	c = tok_next();
	if (c <= 0)
		return 1;
	tok_prevsep = tok_cursep;
	tok_cursep = def_chopped(c);
	if (tok_cursep)
		tok_prevsep = 1;
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
			if (c2 >= '1' && c2 <= '9' && !src_arg(c2 - '0')) {
				tok_cursep = 1;
				return tok_read();
			}
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
					if (s < e)
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
				if (s < e)
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
	while (--i > 0 && s < e)
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

/* return nonzero if current token chops the equation */
int tok_chops(int soft)
{
	if (!tok_get() || tok_curtype == T_KEYWORD)
		return 1;
	if (soft)
		return strchr(T_SOFTSEP, (unsigned char) tok_get()[0]) != NULL;
	return def_chopped((unsigned char) tok_get()[0]);
}

/* read the next token, return the previous */
char *tok_pop(void)
{
	strcpy(tok_prev, tok);
	tok_read();
	return tok_prev[0] ? tok_prev : NULL;
}

/* like tok_pop() but ignore T_SPACE tokens; if sep, read until chopped */
char *tok_poptext(int sep)
{
	while (tok_type() == T_SPACE)
		tok_read();
	tok_prev[0] = '\0';
	do {
		strcat(tok_prev, tok);
		tok_read();
	} while (tok[0] && !tok_chops(!sep));
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
	tok_preview(delim, 0);
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
	c = src_next();
	while (c > 0 && isspace(c))
		c = src_next();
	delim = c;
	c = src_next();
	while (c > 0 && c != delim) {
		sbuf_add(def, c);
		c = src_next();
	}
}

/* read the next macro command */
void tok_macro(void)
{
	char name[NMLEN];
	struct sbuf def;
	tok_preview(name, 0);
	sbuf_init(&def);
	tok_macrodef(&def);
	src_define(name, sbuf_buf(&def));
	sbuf_done(&def);
}

/* return 1 if inside inline equations */
int tok_inline(void)
{
	return tok_line;
}
