/* reading input */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eqn.h"

#define NARGS		10
#define NMACROS		512

/* eqn input stream */
struct ein {
	struct ein *prev;	/* previous buffer */
	char *buf;		/* input buffer; NULL for stdin */
	int pos;		/* current position in buf */
	int unbuf[LNLEN];	/* push-back buffer */
	int uncnt;
	char *args[NARGS];	/* macro arguments */
	int call;		/* is a macro call */
};

static struct ein ein_stdin;	/* the default input stream */
static struct ein *ein = &ein_stdin;
static int lineno = 1;		/* current line number */

static char *in_strdup(char *s)
{
	char *d = malloc(strlen(s) + 1);
	strcpy(d, s);
	return d;
}

/* push buf in the input stream; this is a macro call if args is not NULL */
static void in_push(char *buf, char **args)
{
	struct ein *next = malloc(sizeof(*next));
	int i;
	memset(next, 0, sizeof(*next));
	next->prev = ein;
	next->buf = in_strdup(buf);
	next->call = args != NULL;
	if (args)
		for (i = 0; i < NARGS; i++)
			next->args[i] = args[i] ? in_strdup(args[i]) : NULL;
	ein = next;
}

/* back to the previous ein buffer */
static void in_pop(void)
{
	struct ein *prev = ein->prev;
	int i;
	if (prev) {
		for (i = 0; i < NARGS; i++)
			free(ein->args[i]);
		free(ein->buf);
		free(ein);
		ein = prev;
	}
}

static int in_stdin(void)
{
	int c = fgetc(stdin);
	if (c == '\n')
		lineno++;
	return c;
}

/* read the next character */
int in_next(void)
{
	while (1) {
		if (ein->uncnt)
			return ein->unbuf[--ein->uncnt];
		if (!ein->prev)
			return in_stdin();
		if (ein->buf[ein->pos])
			return (unsigned char) ein->buf[ein->pos++];
		in_pop();
	}
	return 0;
}

/* push back c */
void in_back(int c)
{
	if (c > 0)
		ein->unbuf[ein->uncnt++] = c;
}

int in_lineget(void)
{
	return lineno;
}

void in_lineset(int n)
{
	lineno = n;
}

/* eqn macros */
struct macro {
	char name[NMLEN];
	char *def;
};
static struct macro macros[NMACROS];
static int nmacros;

static int in_findmacro(char *name)
{
	int i;
	for (i = 0; i < nmacros; i++)
		if (!strcmp(macros[i].name, name))
			return i;
	return -1;
}

/* define a macro */
void in_define(char *name, char *def)
{
	int idx = in_findmacro(name);
	if (idx < 0 && nmacros < NMACROS)
		idx = nmacros++;
	if (idx >= 0) {
		strcpy(macros[idx].name, name);
		free(macros[idx].def);
		macros[idx].def = in_strdup(def);
	}
}

void in_done(void)
{
	int i;
	for (i = 0; i < nmacros; i++)
		free(macros[i].def);
}

/* see if name starts with a macro name followed by a '(' */
int in_macrocall(char *name)
{
	int mlen, i;
	for (i = 0; i < nmacros; i++) {
		mlen = strlen(macros[i].name);
		if (!strncmp(macros[i].name, name, mlen) && name[mlen] == '(')
			return mlen;
	}
	return 0;
}

/* expand macro */
int in_expand(char *name, char **args)
{
	int i = in_findmacro(name);
	if (i >= 0)
		in_push(macros[i].def, args);
	return i < 0;
}

/* expand argument */
int in_arg(int i)
{
	int call = ein->call;
	if (call && ein->args[i - 1])
		in_push(ein->args[i - 1], NULL);
	return call ? 0 : 1;
}

/* return one if not reading macros and their arguments */
int in_top(void)
{
	return !ein->prev;
}
