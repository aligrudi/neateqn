/* reading input */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eqn.h"

#define NARGS		10	/* number of arguments */
#define NMACROS		512	/* number of macros */
#define NSRCDEP		512	/* maximum esrc_depth */

/* eqn input stream */
struct esrc {
	struct esrc *prev;	/* previous buffer */
	char *buf;		/* input buffer; NULL for stdin */
	int pos;		/* current position in buf */
	int unbuf[LNLEN];	/* push-back buffer */
	int uncnt;
	char *args[NARGS];	/* macro arguments */
	int call;		/* is a macro call */
};

static struct esrc esrc_stdin;	/* the default input stream */
static struct esrc *esrc = &esrc_stdin;
static int lineno = 1;		/* current line number */
static int esrc_depth;		/* the length of esrc chain */

static char *src_strdup(char *s)
{
	char *d = malloc(strlen(s) + 1);
	strcpy(d, s);
	return d;
}

/* push buf in the input stream; this is a macro call if args is not NULL */
static void src_push(char *buf, char **args)
{
	struct esrc *next;
	int i;
	if (esrc_depth > NSRCDEP)
		errdie("neateqn: macro recursion limit reached\n");
	next = malloc(sizeof(*next));
	memset(next, 0, sizeof(*next));
	next->prev = esrc;
	next->buf = src_strdup(buf);
	next->call = args != NULL;
	if (args)
		for (i = 0; i < NARGS; i++)
			next->args[i] = args[i] ? src_strdup(args[i]) : NULL;
	esrc = next;
	esrc_depth++;
}

/* back to the previous esrc buffer */
static void src_pop(void)
{
	struct esrc *prev = esrc->prev;
	int i;
	if (prev) {
		for (i = 0; i < NARGS; i++)
			free(esrc->args[i]);
		free(esrc->buf);
		free(esrc);
		esrc = prev;
		esrc_depth--;
	}
}

static int src_stdin(void)
{
	int c = fgetc(stdin);
	if (c == '\n')
		lineno++;
	return c;
}

/* read the next character */
int src_next(void)
{
	while (1) {
		if (esrc->uncnt)
			return esrc->unbuf[--esrc->uncnt];
		if (!esrc->prev)
			return src_stdin();
		if (esrc->buf[esrc->pos])
			return (unsigned char) esrc->buf[esrc->pos++];
		src_pop();
	}
	return 0;
}

/* push back c */
void src_back(int c)
{
	if (c > 0)
		esrc->unbuf[esrc->uncnt++] = c;
}

int src_lineget(void)
{
	return lineno;
}

void src_lineset(int n)
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

static int src_findmacro(char *name)
{
	int i;
	for (i = 0; i < nmacros; i++)
		if (!strcmp(macros[i].name, name))
			return i;
	return -1;
}

/* return nonzero if name is a macro */
int src_macro(char *name)
{
	return src_findmacro(name) >= 0;
}

/* define a macro */
void src_define(char *name, char *def)
{
	int idx = src_findmacro(name);
	if (idx < 0 && nmacros < NMACROS)
		idx = nmacros++;
	if (idx >= 0) {
		strcpy(macros[idx].name, name);
		free(macros[idx].def);
		macros[idx].def = src_strdup(def);
	}
}

void src_done(void)
{
	int i;
	for (i = 0; i < nmacros; i++)
		free(macros[i].def);
}

/* expand macro */
int src_expand(char *name, char **args)
{
	int i = src_findmacro(name);
	if (i >= 0)
		src_push(macros[i].def, args);
	return i < 0;
}

/* expand argument */
int src_arg(int i)
{
	int call = esrc->call;
	if (call && esrc->args[i - 1])
		src_push(esrc->args[i - 1], NULL);
	return call ? 0 : 1;
}

/* return one if not reading macros and their arguments */
int src_top(void)
{
	return !esrc->prev;
}
