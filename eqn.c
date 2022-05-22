/*
 * NEATEQN NEATROFF PREPROCESSOR
 *
 * Copyright (C) 2014-2017 Ali Gholami Rudi <ali at rudi dot ir>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eqn.h"

/* flags passed to eqn_box() */
#define EQN_TSMASK	0x00ffff	/* style mask */
#define EQN_SUB		0x020000	/* this is a subscript */
#define EQN_FROM	0x040000	/* this is a from block */

static char gfont[FNLEN] = "2";
static char grfont[FNLEN] = "1";
static char gbfont[FNLEN] = "3";
static char gsize[FNLEN] = "\\n[" EQNSZ "]";
static char eqn_lineup[128];	/* the lineup horizontal request */
static int eqn_lineupreg;	/* the number register holding lineup width */
static int eqn_mk;		/* the value of MK */

static struct box *eqn_box(int flg, struct box *pre, int sz0, char *fn0);

/* read equations until delim is read */
static int eqn_boxuntil(struct box *box, int sz0, char *fn0, char *delim)
{
	struct box *sub = NULL;
	while (tok_get() && tok_jmp(delim)) {
		if (!strcmp("}", tok_get()))
			return 1;
		sub = eqn_box(box->style, sub ? box : NULL, sz0, fn0);
		box_merge(box, sub, 0);
		box_free(sub);
	}
	return 0;
}

/* subscript size */
static void sizesub(int dst, int src, int style, int src_style)
{
	if (TS_SZ(style) > TS_SZ(src_style)) {
		printf(".nr %s %s*7/10\n", nregname(dst), nreg(src));
		printf(".if %s<%d .nr %s %d\n",
			nreg(dst), e_minimumsize,
			nregname(dst), e_minimumsize);
	} else {
		printf(".nr %s %s\n", nregname(dst), nreg(src));
	}
}

static char *tok_quotes(char *s)
{
	if (s && s[0] == '"') {
		s[strlen(s) - 1] = '\0';
		return s + 1;
	}
	return s ? s : "";
}

static char *tok_improve(char *s)
{
	if (s && s[0] == '-' && s[1] == '\0')
		return "\\(mi";
	if (s && s[0] == '+' && s[1] == '\0')
		return "\\(pl";
	if (s && s[0] == '\'' && s[1] == '\0')
		return "\\(fm";
	return tok_quotes(s);
}

static void eqn_bracketsizes(void)
{
	char sign[BRLEN];
	char bufs[NSIZES][BRLEN];
	char *sizes[NSIZES] = {NULL};
	int n, i;
	snprintf(sign, sizeof(sign), "%s", tok_quotes(tok_poptext(1)));
	n = atoi(tok_poptext(1));
	for (i = 0; i < n; i++) {
		char *size = tok_quotes(tok_poptext(1));
		if (i < NSIZES) {
			snprintf(bufs[i], sizeof(bufs[i]), "%s", size);
			sizes[i] = bufs[i];
		}
	}
	def_sizesput(sign, sizes);
}

static void eqn_bracketpieces(void)
{
	char sign[BRLEN], top[BRLEN], mid[BRLEN], bot[BRLEN], cen[BRLEN];
	snprintf(sign, sizeof(sign), "%s", tok_quotes(tok_poptext(1)));
	snprintf(top, sizeof(top), "%s", tok_quotes(tok_poptext(1)));
	snprintf(mid, sizeof(mid), "%s", tok_quotes(tok_poptext(1)));
	snprintf(bot, sizeof(bot), "%s", tok_quotes(tok_poptext(1)));
	snprintf(cen, sizeof(cen), "%s", tok_quotes(tok_poptext(1)));
	def_piecesput(sign, top, mid, bot, cen);
}

static int typenum(char *s)
{
	if (!strcmp("ord", s) || !strcmp("ordinary", s))
		return T_ORD;
	if (!strcmp("op", s) || !strcmp("operator", s))
		return T_BIGOP;
	if (!strcmp("bin", s) || !strcmp("binary", s))
		return T_BINOP;
	if (!strcmp("rel", s) || !strcmp("relation", s))
		return T_RELOP;
	if (!strcmp("open", s) || !strcmp("opening", s))
		return T_LEFT;
	if (!strcmp("close", s) || !strcmp("closing", s))
		return T_RIGHT;
	if (!strcmp("punct", s) || !strcmp("punctuation", s))
		return T_PUNC;
	if (!strcmp("inner", s))
		return T_INNER;
	return T_ORD;
}

/* read chartype command arguments and perform it */
static void eqn_chartype(void)
{
	char gl[GNLEN], type[NMLEN];
	snprintf(type, sizeof(type), "%s", tok_quotes(tok_poptext(1)));
	snprintf(gl, sizeof(gl), "%s", tok_quotes(tok_poptext(1)));
	if (typenum(type) >= 0)
		def_typeput(gl, typenum(type));
}

/* read breakcost command arguments and perform it */
static void eqn_breakcost(void)
{
	char tok[NMLEN];
	int cost, type;
	snprintf(tok, sizeof(tok), "%s", tok_quotes(tok_poptext(1)));
	cost = atoi(tok_poptext(1));
	type = !strcmp("any", tok) ? 0 : typenum(tok);
	if (type >= 0)
		def_brcostput(type, cost);
}

/* read general eqn commands */
static int eqn_commands(void)
{
	char var[LNLEN];
	char *sz;
	if (!tok_jmp("delim")) {
		tok_delim();
		return 0;
	}
	if (!tok_jmp("define")) {
		tok_macro();
		return 0;
	}
	if (!tok_jmp("gfont")) {
		strcpy(gfont, tok_quotes(tok_poptext(1)));
		return 0;
	}
	if (!tok_jmp("grfont")) {
		strcpy(grfont, tok_quotes(tok_poptext(1)));
		return 0;
	}
	if (!tok_jmp("gbfont")) {
		strcpy(gbfont, tok_quotes(tok_poptext(1)));
		return 0;
	}
	if (!tok_jmp("gsize")) {
		sz = tok_quotes(tok_poptext(1));
		if (sz[0] == '-' || sz[0] == '+')
			sprintf(gsize, "\\n%s%s", escarg(EQNSZ), sz);
		else
			strcpy(gsize, sz);
		return 0;
	}
	if (!tok_jmp("set")) {
		strcpy(var, tok_poptext(1));
		def_set(var, atoi(tok_poptext(1)));
		return 0;
	}
	if (!tok_jmp("bracketsizes")) {
		eqn_bracketsizes();
		return 0;
	}
	if (!tok_jmp("bracketpieces")) {
		eqn_bracketpieces();
		return 0;
	}
	if (!tok_jmp("chartype")) {
		eqn_chartype();
		return 0;
	}
	if (!tok_jmp("breakcost")) {
		eqn_breakcost();
		return 0;
	}
	return 1;
}

/* read user-specified spaces */
static int eqn_gaps(struct box *box, int szreg)
{
	if (!tok_jmp("~")) {
		box_puttext(box, T_GAP, "\\h'%du*%sp/100u'",
				S_S3, nreg(szreg));
		return 0;
	}
	if (!tok_jmp("^")) {
		box_puttext(box, T_GAP, "\\h'%du*%sp/100u'",
				S_S1, nreg(szreg));
		return 0;
	}
	if (!tok_jmp("\t")) {
		box_puttext(box, T_GAP, "\t");
		return 0;
	}
	return 1;
}

/* return the font of the given token type */
static char *tok_font(int tok, char *fn)
{
	if (fn && fn[0])
		return fn;
	if (tok == T_LETTER || tok == T_STRING)
		return gfont;
	return grfont;
}

/* check the next token */
static void tok_expect(char *s)
{
	if (tok_jmp(s)) {
		fprintf(stderr, "neateqn: expected %s bot got %s\n",
			s, tok_get());
		exit(1);
	}
}

/* read pile command */
static void eqn_pile(struct box *box, int sz0, char *fn0, int adj)
{
	struct box *pile[NPILES] = {NULL};
	int i;
	int n = 0;
	int rowspace = 0;
	if (tok_jmp("{")) {
		rowspace = atoi(tok_poptext(1));
		tok_expect("{");
	}
	do {
		pile[n++] = box_alloc(sz0, 0, box->style);
	} while (!eqn_boxuntil(pile[n - 1], sz0, fn0, "above"));
	tok_expect("}");
	box_pile(box, pile, adj, rowspace);
	for (i = 0; i < n; i++)
		box_free(pile[i]);
}

/* read matrix command */
static void eqn_matrix(struct box *box, int sz0, char *fn0)
{
	struct box *cols[NPILES][NPILES] = {{NULL}};
	int adj[NPILES];
	int nrows;
	int ncols = 0;
	int colspace = 0;
	int rowspace = 0;
	int i, j;
	if (tok_jmp("{")) {
		colspace = atoi(tok_poptext(1));
		tok_expect("{");
	}
	while (1) {
		if (!tok_jmp("col") || !tok_jmp("ccol"))
			adj[ncols] = 'c';
		else if (!tok_jmp("lcol"))
			adj[ncols] = 'l';
		else if (!tok_jmp("rcol"))
			adj[ncols] = 'r';
		else
			break;
		nrows = 0;
		if (tok_jmp("{")) {
			i = atoi(tok_poptext(1));
			if (i > rowspace)
				rowspace = i;
			tok_expect("{");
		}
		do {
			cols[ncols][nrows++] = box_alloc(sz0, 0, box->style);
		} while (!eqn_boxuntil(cols[ncols][nrows - 1],
				sz0, fn0, "above"));
		tok_expect("}");
		ncols++;
	}
	tok_expect("}");
	box_matrix(box, ncols, cols, adj, colspace, rowspace);
	for (i = 0; i < ncols; i++)
		for (j = 0; j < NPILES; j++)
			if (cols[i][j])
				box_free(cols[i][j]);
}

/* return nonzero if fn is italic */
static int italic(char *fn)
{
	return (!strcmp("I", fn) || !strcmp("2", fn) ||
		gfont == fn || !strcmp(gfont, fn)) ? T_ITALIC : 0;
}

/* read a box without fractions */
static struct box *eqn_left(int flg, struct box *pre, int sz0, char *fn0)
{
	struct box *box = NULL;
	struct box *sub_sub = NULL, *sub_sup = NULL;
	struct box *sub_from = NULL, *sub_to = NULL;
	struct box *sqrt, *inner;
	char left[NMLEN] = "", right[NMLEN] = "";
	char fn[FNLEN] = "";
	int sz = sz0;
	int subsz;
	int dx = 0, dy = 0;
	int style = EQN_TSMASK & flg;
	if (fn0)
		strcpy(fn, fn0);
	while (!eqn_commands())
		;
	box = box_alloc(sz, pre ? pre->tcur : 0, style);
	if (!eqn_gaps(box, sz)) {
		while (!eqn_gaps(box, sz))
			;
		return box;
	}
	while (1) {
		if (!tok_jmp("fat")) {
		} else if (!tok_jmp("roman")) {
			strcpy(fn, grfont);
		} else if (!tok_jmp("italic")) {
			strcpy(fn, gfont);
		} else if (!tok_jmp("bold")) {
			strcpy(fn, gbfont);
		} else if (!tok_jmp("font")) {
			strcpy(fn, tok_poptext(1));
		} else if (!tok_jmp("size")) {
			sz = box_size(box, tok_poptext(1));
		} else if (!tok_jmp("fwd")) {
			dx += atoi(tok_poptext(1));
		} else if (!tok_jmp("back")) {
			dx -= atoi(tok_poptext(1));
		} else if (!tok_jmp("down")) {
			dy += atoi(tok_poptext(1));
		} else if (!tok_jmp("up")) {
			dy -= atoi(tok_poptext(1));
		} else {
			break;
		}
	}
	if (!tok_jmp("sqrt")) {
		sqrt = eqn_left(TS_MK0(style), NULL, sz, fn);
		printf(".ft %s\n", grfont);
		box_sqrt(box, sqrt);
		box_free(sqrt);
	} else if (!tok_jmp("pile") || !tok_jmp("cpile")) {
		eqn_pile(box, sz, fn, 'c');
	} else if (!tok_jmp("lpile")) {
		eqn_pile(box, sz, fn, 'l');
	} else if (!tok_jmp("rpile")) {
		eqn_pile(box, sz, fn, 'r');
	} else if (!tok_jmp("matrix")) {
		eqn_matrix(box, sz, fn);
	} else if (!tok_jmp("vcenter")) {
		inner = eqn_left(flg, pre, sz, fn);
		box_vcenter(box, inner);
		box_free(inner);
	} else if (!tok_jmp("{")) {
		eqn_boxuntil(box, sz, fn, "}");
	} else if (!tok_jmp("left")) {
		inner = box_alloc(sz, 0, style);
		snprintf(left, sizeof(left), "%s", tok_quotes(tok_poptext(0)));
		eqn_boxuntil(inner, sz, fn, "right");
		snprintf(right, sizeof(right), "%s", tok_quotes(tok_poptext(0)));
		printf(".ft %s\n", grfont);
		box_wrap(box, inner, left[0] ? left : NULL,
				right[0] ? right : NULL);
		box_free(inner);
	} else if (tok_get() && tok_type() != T_KEYWORD) {
		if (dx || dy)
			box_move(box, dy, dx);
		box_putf(box, "\\s%s", escarg(nreg(sz)));
		do {
			char *cfn = tok_font(tok_type(), fn);
			int chops;
			box_puttext(box, tok_type() | italic(cfn), "\\f%s%s",
					escarg(cfn), tok_improve(tok_get()));
			chops = tok_chops(0);
			tok_pop();
			if (chops)		/* what we read was a splitting */
				break;
		} while (!tok_chops(0));	/* the next token is splitting */
		if (dx || dy)
			box_move(box, -dy, -dx);
	}
	while (tok_get()) {
		if (!tok_jmp("dyad")) {
			printf(".ft %s\n", grfont);
			box_accent(box, "\\(ab");
		} else if (!tok_jmp("bar")) {
			printf(".ft %s\n", grfont);
			box_bar(box);
		} else if (!tok_jmp("under")) {
			printf(".ft %s\n", grfont);
			box_under(box);
		} else if (!tok_jmp("vec")) {
			printf(".ft %s\n", grfont);
			box_accent(box, "\\s[\\n(.s/2u]\\(->\\s0");
		} else if (!tok_jmp("tilde")) {
			printf(".ft %s\n", grfont);
			box_accent(box, "\\s[\\n(.s*3u/4u]\\(ap\\s0");
		} else if (!tok_jmp("hat")) {
			printf(".ft %s\n", grfont);
			box_accent(box, "ˆ");
		} else if (!tok_jmp("dot")) {
			printf(".ft %s\n", grfont);
			box_accent(box, ".");
		} else if (!tok_jmp("dotdot")) {
			printf(".ft %s\n", grfont);
			box_accent(box, "..");
		} else {
			break;
		}
	}
	subsz = nregmk();
	if (!tok_jmp("sub")) {
		sizesub(subsz, sz0, ts_sup(style), style);
		sub_sub = eqn_left(ts_sup(style) | EQN_SUB, NULL, subsz, fn0);
	}
	if ((sub_sub || !(flg & EQN_SUB)) && !tok_jmp("sup")) {
		sizesub(subsz, sz0, ts_sub(style), style);
		sub_sup = eqn_left(ts_sub(style), NULL, subsz, fn0);
	}
	if (sub_sub || sub_sup)
		box_sub(box, sub_sub, sub_sup);
	if (!tok_jmp("from")) {
		sizesub(subsz, sz0, ts_sub(style), style);
		sub_from = eqn_left(ts_sub(style) | EQN_FROM, NULL, subsz, fn0);
	}
	if ((sub_from || !(flg & EQN_FROM)) && !tok_jmp("to")) {
		sizesub(subsz, sz0, ts_sup(style), style);
		sub_to = eqn_left(ts_sup(style), NULL, subsz, fn0);
	}
	if (sub_from || sub_to) {
		inner = box_alloc(sz0, 0, style);
		box_from(inner, box, sub_from, sub_to);
		box_free(box);
		box = inner;
	}
	nregrm(subsz);
	if (sub_sub)
		box_free(sub_sub);
	if (sub_sup)
		box_free(sub_sup);
	if (sub_from)
		box_free(sub_from);
	if (sub_to)
		box_free(sub_to);
	return box;
}

/* read a box */
static struct box *eqn_box(int flg, struct box *pre, int sz0, char *fn0)
{
	struct box *box;
	struct box *sub_num = NULL, *sub_den = NULL;
	int style = flg & EQN_TSMASK;
	box = eqn_left(flg, pre, sz0, fn0);
	while (!tok_jmp("over")) {
		sub_num = box;
		sub_den = eqn_left(TS_MK0(style), NULL, sz0, fn0);
		box = box_alloc(sz0, pre ? pre->tcur : 0, style);
		printf(".ft %s\n", grfont);
		box_over(box, sub_num, sub_den);
		box_free(sub_num);
		box_free(sub_den);
	}
	return box;
}

/* read an equation, either inline or block */
static struct box *eqn_read(int style)
{
	struct box *box, *sub;
	int szreg = nregmk();
	printf(".nr %s %s\n", nregname(szreg), gsize);
	box = box_alloc(szreg, 0, style);
	while (tok_get()) {
		if (!tok_jmp("mark")) {
			eqn_mk = !eqn_mk ? 1 : eqn_mk;
			box_markpos(box, EQNMK);
			continue;
		}
		if (!tok_jmp("lineup")) {
			eqn_mk = 2;
			box_markpos(box, nregname(eqn_lineupreg));
			sprintf(eqn_lineup, "\\h'\\n%su-%su'",
				escarg(EQNMK), nreg(eqn_lineupreg));
			continue;
		}
		sub = eqn_box(style, box, szreg, NULL);
		box_merge(box, sub, 1);
		box_free(sub);
	}
	box_vertspace(box);
	nregrm(szreg);
	return box;
}

void errdie(char *msg)
{
	fprintf(stderr, msg);
	exit(1);
}

int main(int argc, char **argv)
{
	struct box *box;
	char eqnblk[128];
	int i;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-' || !argv[i][1])
			break;
		if (argv[i][1] == 'c') {
			def_choppedset(argv[i][2] ? argv[i] + 2 : argv[++i]);
		} else {
			fprintf(stderr, "Usage: neateqn [options] <input >output\n\n"
					"Options:\n"
					"  -c chars  \tcharacters that chop equations\n");
			return 1;
		}
	}
	for (i = 0; def_macros[i][0]; i++)
		src_define(def_macros[i][0], def_macros[i][1]);
	while (!tok_eqn()) {
		reg_reset();
		eqn_mk = 0;
		tok_pop();
		printf(".nr %s \\n(.s\n", EQNSZ);
		printf(".nr %s \\n(.f\n", EQNFN);
		eqn_lineupreg = nregmk();
		box = eqn_read(tok_inline() ? TS_T : TS_D);
		printf(".nr MK %d\n", eqn_mk);
		if (!box_empty(box)) {
			sprintf(eqnblk, "%s%s", eqn_lineup, box_toreg(box));
			tok_eqnout(eqnblk);
			printf(".ps \\n%s\n", escarg(EQNSZ));
			printf(".ft \\n%s\n", escarg(EQNFN));
		}
		printf(".lf %d\n", src_lineget());
		eqn_lineup[0] = '\0';
		nregrm(eqn_lineupreg);
		box_free(box);
	}
	src_done();
	return 0;
}
