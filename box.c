/* equation boxes */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eqn.h"

struct box *box_alloc(int szreg, int pre, int style)
{
	struct box *box = malloc(sizeof(*box));
	memset(box, 0, sizeof(*box));
	sbuf_init(&box->raw);
	box->szreg = szreg;
	box->atoms = 0;
	box->style = style;
	if (pre)
		box->tcur = pre;
	return box;
}

void box_free(struct box *box)
{
	if (box->reg)
		sregrm(box->reg);
	if (box->szown)
		nregrm(box->szreg);
	sbuf_done(&box->raw);
	free(box);
}

static void box_put(struct box *box, char *s)
{
	sbuf_append(&box->raw, s);
	if (box->reg)
		printf(".as %s \"%s\n", sregname(box->reg), s);
}

void box_putf(struct box *box, char *s, ...)
{
	char buf[LNLEN];
	va_list ap;
	va_start(ap, s);
	vsnprintf(buf, sizeof(buf), s, ap);
	va_end(ap);
	box_put(box, buf);
}

char *box_buf(struct box *box)
{
	return sbuf_buf(&box->raw);
}

/* change box's point size; return the number register storing it */
int box_size(struct box *box, char *val)
{
	int szreg = box->szreg;
	if (!val || !*val)
		return szreg;
	if (!box->szown) {
		box->szown = 1;
		box->szreg = nregmk();
	}
	if (val[0] == '-' || val[0] == '+')
		printf(".nr %s %s%s\n", nregname(box->szreg), nreg(szreg), val);
	else
		printf(".nr %s %s\n", nregname(box->szreg), val);
	return box->szreg;
}

void box_move(struct box *box, int dy, int dx)
{
	if (dy)
		box_putf(box, "\\v'%du*%sp/100u'", dy, nreg(box->szreg));
	if (dx)
		box_putf(box, "\\h'%du*%sp/100u'", dx, nreg(box->szreg));
}

/* T_ORD, T_BIGOP, T_BINOP, T_RELOP, T_LEFT, T_RIGHT, T_PUNC, T_INNER */
static int spacing[][8] = {
	{0, 1, 2, 3, 0, 0, 0, 1},
	{1, 1, 0, 3, 0, 0, 0, 1},
	{2, 2, 0, 0, 2, 0, 0, 2},
	{3, 3, 0, 0, 3, 0, 0, 3},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 1, 2, 3, 0, 0, 0, 1},
	{1, 1, 0, 1, 1, 1, 1, 1},
	{1, 1, 2, 3, 1, 0, 1, 1},
};

/* return the amount of automatic spacing before adding the given token */
static int eqn_gaps(struct box *box, int cur)
{
	int s = 0;
	int a1 = T_ATOM(box->tcur);	/* previous atom */
	int a2 = T_ATOM(cur);		/* current atom */
	if (!TS_SZ(box->style) && a1 && a2)
		s = spacing[T_ATOMIDX(a1)][T_ATOMIDX(a2)];
	if (s == 3)
		return S_S3;
	if (s == 2)
		return S_S2;
	return s ? S_S1 : 0;
}

/* call just before inserting a non-italic character */
static void box_italiccorrection(struct box *box)
{
	if (box->atoms && (box->tcur & T_ITALIC))
		box_put(box, "\\/");
	box->tcur &= ~T_ITALIC;
}

static int box_fixatom(int cur, int pre)
{
	if (cur == T_BINOP && (!pre || pre == T_RELOP || pre == T_BIGOP ||
				pre == T_LEFT || pre == T_PUNC))
		return T_ORD;
	if (cur == T_RELOP && (!pre || pre == T_LEFT))
		return T_ORD;
	return cur;
}

/* call before inserting a token with box_put() and box_putf()  */
static void box_beforeput(struct box *box, int type, int breakable)
{
	int autogaps;	/* automatically inserted space before this token */
	if (box->atoms) {
		autogaps = eqn_gaps(box, T_ATOM(type));
		if (!(type & T_ITALIC))
			box_italiccorrection(box);
		if (autogaps && type != T_GAP && box->tcur != T_GAP) {
			box_italiccorrection(box);
			if (breakable) {	/* enlarge a space to match autogaps */
				box_putf(box, "\\s[\\En(.s*%du*%sp/100u/\\w' 'u]\\j'%d' \\s0",
					autogaps, nreg(box->szreg),
					def_brcost(T_ATOM(box->tcur)));
			} else {
				box_putf(box, "\\h'%du*%sp/100u'",
						autogaps, nreg(box->szreg));
			}
		}
	}
	if (box->tomark) {
		printf(".nr %s 0\\w'%s'\n", box->tomark, box_toreg(box));
		box->tomark = NULL;
	}
}

/* call after inserting a token with box_put() and box_putf()  */
static void box_afterput(struct box *box, int type)
{
	box->atoms++;
	box->tcur = T_FONT(type) | box_fixatom(T_TOK(type), T_TOK(box->tcur));
	if (!box->tbeg)
		box->tbeg = box->tcur;
}

/* insert s with the given type */
void box_puttext(struct box *box, int type, char *s, ...)
{
	char buf[LNLEN];
	va_list ap;
	va_start(ap, s);
	vsnprintf(buf, sizeof(buf), s, ap);
	va_end(ap);
	box_beforeput(box, type, 0);
	if (!(box->tcur & T_ITALIC) && (type & T_ITALIC))
		box_put(box, "\\,");
	box_put(box, buf);
	box_afterput(box, type);
}

/* append sub to box */
void box_merge(struct box *box, struct box *sub, int breakable)
{
	if (box_empty(sub))
		return;
	box_beforeput(box, sub->tbeg, breakable);
	box_toreg(box);
	box_put(box, box_toreg(sub));
	if (!box->tbeg)
		box->tbeg = sub->tbeg;
	/* fix atom type only if merging a single atom */
	if (sub->atoms == 1) {
		box_afterput(box, sub->tcur);
	} else {
		box->tcur = sub->tcur;
		box->atoms += sub->atoms;
	}
}

/* put the maximum of number registers a and b into register dst */
static void roff_max(int dst, int a, int b)
{
	printf(".ie %s>=%s .nr %s 0+%s\n",
		nreg(a), nreg(b), nregname(dst), nreg(a));
	printf(".el .nr %s 0+%s\n", nregname(dst), nreg(b));
}

/* return the width, height and depth of a string */
static void tok_dim(char *s, int wd, int ht, int dp)
{
	printf(".nr %s 0\\w'%s'\n", nregname(wd), s);
	if (ht)
		printf(".nr %s 0-\\n[bbury]\n", nregname(ht));
	if (dp)
		printf(".nr %s 0\\n[bblly]\n", nregname(dp));
}

static int box_suprise(struct box *box)
{
	if (TS_0(box->style))
		return e_sup3;
	return box->style == TS_D ? e_sup1 : e_sup2;
}

void box_sub(struct box *box, struct box *sub, struct box *sup)
{
	int box_wd = nregmk();
	int box_wdnoic = nregmk();
	int box_dp = nregmk();
	int box_ht = nregmk();
	int sub_wd = nregmk();
	int sup_wd = nregmk();
	int all_wd = nregmk();
	int sup_dp = nregmk();
	int sub_ht = nregmk();
	int sup_rise = nregmk();
	int sub_fall = nregmk();
	int tmp_18e = nregmk();
	int sub_cor = nregmk();
	if (sub)
		box_italiccorrection(sub);
	if (sup)
		box_italiccorrection(sup);
	if (sub)
		tok_dim(box_toreg(box), box_wdnoic, 0, 0);
	box_italiccorrection(box);
	printf(".ps %s\n", nreg(box->szreg));
	tok_dim(box_toreg(box), box_wd, box_ht, box_dp);
	box_putf(box, "\\h'5m/100u'");
	if (sup) {
		tok_dim(box_toreg(sup), sup_wd, 0, sup_dp);
		/* 18a */
		printf(".nr %s 0%su-(%dm/100u)\n",
			nregname(sup_rise), nreg(box_ht), e_supdrop);
		/* 18c */
		printf(".if %s<(%dm/100u) .nr %s (%dm/100u)\n",
			nreg(sup_rise), box_suprise(box),
			nregname(sup_rise), box_suprise(box));
		printf(".if %s<(%s+(%dm/100u/4)) .nr %s 0%s+(%dm/100u/4)\n",
			nreg(sup_rise), nreg(sup_dp), e_xheight,
			nregname(sup_rise), nreg(sup_dp), e_xheight);
	}
	if (sub) {
		tok_dim(box_toreg(sub), sub_wd, sub_ht, 0);
		/* 18a */
		printf(".nr %s 0%su+(%dm/100u)\n",
			nregname(sub_fall), nreg(box_dp), e_subdrop);
	}
	if (sub && !sup) {
		/* 18b */
		printf(".if %s<(%dm/100u) .nr %s (%dm/100u)\n",
			nreg(sub_fall), e_sub1,
			nregname(sub_fall), e_sub1);
		printf(".if %s<(%s-(%dm/100u*4/5)) .nr %s 0%s-(%dm/100u*4/5)\n",
			nreg(sub_fall), nreg(sub_ht), e_xheight,
			nregname(sub_fall), nreg(sub_ht), e_xheight);
	}
	if (sub && sup) {
		/* 18d */
		printf(".if %s<(%dm/100u) .nr %s (%dm/100u)\n",
			nreg(sub_fall), e_sub2,
			nregname(sub_fall), e_sub2);
		/* 18e */
		printf(".if (%s-%s)-(%s-%s)<(%dm/100u*4) \\{\\\n",
			nreg(sup_rise), nreg(sup_dp),
			nreg(sub_ht), nreg(sub_fall), e_rulethickness);
		printf(".nr %s (%dm/100u*4)+%s-(%s-%s)\n",
			nregname(sub_fall), e_rulethickness,
			nreg(sub_ht), nreg(sup_rise), nreg(sup_dp));
		printf(".nr %s (%dm/100u*4/5)-(%s-%s)\n",
			nregname(tmp_18e), e_xheight,
			nreg(sup_rise), nreg(sup_dp));
		printf(".if %s>0 .nr %s +%s\n",
			nreg(tmp_18e), nregname(sup_rise), nreg(tmp_18e));
		printf(".if %s>0 .nr %s -%s \\}\n",
			nreg(tmp_18e), nregname(sub_fall), nreg(tmp_18e));
	}
	/* writing the superscript */
	if (sup) {
		box_putf(box, "\\v'-%su'%s\\v'%su'",
			nreg(sup_rise), box_toreg(sup), nreg(sup_rise));
		if (sub)
			box_putf(box, "\\h'-%su'", nreg(sup_wd));
	}
	/* writing the subscript */
	if (sub) {
		/* subscript correction */
		printf(".nr %s (%s-%s)\n", nregname(sub_cor),
			nreg(box_wd), nreg(box_wdnoic));
		printf(".if %s>0 .nr %s (%s+%s)*(%s-%s)/%s\n",
			nreg(box_ht), nregname(sub_cor),
			nreg(box_ht), nreg(sub_fall),
			nreg(box_wd), nreg(box_wdnoic), nreg(box_ht));
		printf(".nr %s -%s\n", nregname(sub_wd), nreg(sub_cor));
		box_putf(box, "\\h'-%su'", nreg(sub_cor));
		box_putf(box, "\\v'%su'%s\\v'-%su'",
			nreg(sub_fall), box_toreg(sub), nreg(sub_fall));
		if (sup) {
			box_putf(box, "\\h'-%su'", nreg(sub_wd));
			roff_max(all_wd, sub_wd, sup_wd);
			box_putf(box, "\\h'+%su'", nreg(all_wd));
		}
	}
	box_putf(box, "\\h'%dm/100u'", e_scriptspace);
	nregrm(box_wd);
	nregrm(box_wdnoic);
	nregrm(box_dp);
	nregrm(box_ht);
	nregrm(sub_wd);
	nregrm(sup_wd);
	nregrm(all_wd);
	nregrm(sup_dp);
	nregrm(sub_ht);
	nregrm(sup_rise);
	nregrm(sub_fall);
	nregrm(tmp_18e);
	nregrm(sub_cor);
}

void box_from(struct box *box, struct box *lim, struct box *llim, struct box *ulim)
{
	int lim_wd = nregmk();		/* box's width */
	int lim_ht = nregmk();		/* box's height */
	int lim_dp = nregmk();		/* box's depth */
	int llim_wd = nregmk();		/* llim's width */
	int ulim_wd = nregmk();		/* ulim's width */
	int ulim_dp = nregmk();		/* ulim's depth */
	int llim_ht = nregmk();		/* llim's height */
	int ulim_rise = nregmk();	/* the position of ulim */
	int llim_fall = nregmk();	/* the position of llim */
	int all_wd = nregmk();		/* the width of all */
	box_italiccorrection(lim);
	box_beforeput(box, T_BIGOP, 0);
	tok_dim(box_toreg(lim), lim_wd, lim_ht, lim_dp);
	printf(".ps %s\n", nreg(box->szreg));
	if (ulim)
		tok_dim(box_toreg(ulim), ulim_wd, 0, ulim_dp);
	if (llim)
		tok_dim(box_toreg(llim), llim_wd, llim_ht, 0);
	if (ulim && llim)
		roff_max(all_wd, llim_wd, ulim_wd);
	else
		printf(".nr %s %s\n", nregname(all_wd),
			ulim ? nreg(ulim_wd) : nreg(llim_wd));
	printf(".if %s>%s .nr %s 0%s\n",
		nreg(lim_wd), nreg(all_wd),
		nregname(all_wd), nreg(lim_wd));
	box_putf(box, "\\h'%su-%su/2u'", nreg(all_wd), nreg(lim_wd));
	box_merge(box, lim, 0);
	box_putf(box, "\\h'-%su/2u'", nreg(lim_wd));
	if (ulim) {
		/* 13a */
		printf(".nr %s (%dm/100u)-%s\n",
			nregname(ulim_rise), e_bigopspacing3, nreg(ulim_dp));
		printf(".if %s<(%dm/100u) .nr %s (%dm/100u)\n",
			nreg(ulim_rise), e_bigopspacing1,
			nregname(ulim_rise), e_bigopspacing1);
		printf(".nr %s +%s+%s\n",
			nregname(ulim_rise), nreg(lim_ht), nreg(ulim_dp));
		box_putf(box, "\\h'-%su/2u'\\v'-%su'%s\\v'%su'\\h'-%su/2u'",
			nreg(ulim_wd), nreg(ulim_rise), box_toreg(ulim),
			nreg(ulim_rise), nreg(ulim_wd));
	}
	if (llim) {
		/* 13a */
		printf(".nr %s (%dm/100u)-%s\n",
			nregname(llim_fall), e_bigopspacing4, nreg(llim_ht));
		printf(".if %s<(%dm/100u) .nr %s (%dm/100u)\n",
			nreg(llim_fall), e_bigopspacing2,
			nregname(llim_fall), e_bigopspacing2);
		printf(".nr %s +%s+%s\n",
			nregname(llim_fall), nreg(lim_dp), nreg(llim_ht));
		box_putf(box, "\\h'-%su/2u'\\v'%su'%s\\v'-%su'\\h'-%su/2u'",
			nreg(llim_wd), nreg(llim_fall), box_toreg(llim),
			nreg(llim_fall), nreg(llim_wd));
	}
	box_putf(box, "\\h'%su/2u'", nreg(all_wd));
	box_afterput(box, T_BIGOP);
	nregrm(lim_wd);
	nregrm(lim_ht);
	nregrm(lim_dp);
	nregrm(llim_wd);
	nregrm(ulim_wd);
	nregrm(ulim_dp);
	nregrm(llim_ht);
	nregrm(ulim_rise);
	nregrm(llim_fall);
	nregrm(all_wd);
}

/* return the width of s; len is the height plus depth */
static void tok_len(char *s, int wd, int len, int ht, int dp)
{
	printf(".nr %s 0\\w'%s'\n", nregname(wd), s);
	if (len)
		printf(".nr %s 0\\n[bblly]-\\n[bbury]-2\n", nregname(len));
	if (dp)
		printf(".nr %s 0\\n[bblly]-1\n", nregname(dp));
	if (ht)
		printf(".nr %s 0-\\n[bbury]-1\n", nregname(ht));
}

/* len[0]: width, len[1]: vertical length, len[2]: height, len[3]: depth */
static void blen_mk(char *s, int len[4])
{
	int i;
	for (i = 0; i < 4; i++)
		len[i] = nregmk();
	tok_len(s, len[0], len[1], len[2], len[3]);
}

/* free the registers allocated with blen_mk() */
static void blen_rm(int len[4])
{
	int i;
	for (i = 0; i < 4; i++)
		nregrm(len[i]);
}

/* build a fraction; the correct font should be set up beforehand */
void box_over(struct box *box, struct box *num, struct box *den)
{
	int num_wd = nregmk();
	int num_dp = nregmk();
	int den_wd = nregmk();
	int den_ht = nregmk();
	int all_wd = nregmk();
	int num_rise = nregmk();
	int den_fall = nregmk();
	int bar_wd = nregmk();
	int bar_dp = nregmk();
	int bar_ht = nregmk();
	int bar_fall = nregmk();
	int tmp_15d = nregmk();
	int bargap = (TS_DX(box->style) ? 7 : 3) * e_rulethickness / 2;
	box_beforeput(box, T_INNER, 0);
	box_italiccorrection(num);
	box_italiccorrection(den);
	tok_dim(box_toreg(num), num_wd, 0, num_dp);
	tok_dim(box_toreg(den), den_wd, den_ht, 0);
	roff_max(all_wd, num_wd, den_wd);
	printf(".ps %s\n", nreg(box->szreg));
	tok_len("\\(ru", bar_wd, 0, bar_ht, bar_dp);
	/* 15b */
	printf(".nr %s 0%dm/100u\n",
		nregname(num_rise), TS_DX(box->style) ? e_num1 : e_num2);
	printf(".nr %s 0%dm/100u\n",
		nregname(den_fall), TS_DX(box->style) ? e_denom1 : e_denom2);
	/* 15d */
	printf(".nr %s (%s-%s)-((%dm/100u)+(%dm/100u/2))\n",
		nregname(tmp_15d), nreg(num_rise), nreg(num_dp),
		e_axisheight, e_rulethickness);
	printf(".if %s<(%dm/100u) .nr %s +(%dm/100u)-%s\n",
		nreg(tmp_15d), bargap, nregname(num_rise),
		bargap, nreg(tmp_15d));
	printf(".nr %s ((%dm/100u)-(%dm/100u/2))-(%s-%s)\n",
		nregname(tmp_15d), e_axisheight, e_rulethickness,
		nreg(den_ht), nreg(den_fall));
	printf(".if %s<(%dm/100u) .nr %s +(%dm/100u)-%s\n",
		nreg(tmp_15d), bargap, nregname(den_fall),
		bargap, nreg(tmp_15d));
	/* calculating the vertical position of the bar */
	printf(".nr %s 0-%s+%s/2-(%dm/100u)\n",
		nregname(bar_fall), nreg(bar_dp),
		nreg(bar_ht), e_axisheight);
	/* making the bar longer */
	printf(".nr %s +2*(%dm/100u)\n",
		nregname(all_wd), e_overhang);
	/* null delimiter space */
	box_putf(box, "\\h'%sp*%du/100u'",nreg(box->szreg), e_nulldelim);
	/* drawing the bar */
	box_putf(box, "\\v'%su'\\f[\\n(.f]\\s[%s]\\l'%su'\\v'-%su'\\h'-%su/2u'",
		nreg(bar_fall), nreg(box->szreg), nreg(all_wd),
		nreg(bar_fall), nreg(all_wd));
	/* output the numerator */
	box_putf(box, "\\h'-%su/2u'", nreg(num_wd));
	box_putf(box, "\\v'-%su'%s\\v'%su'",
		nreg(num_rise), box_toreg(num), nreg(num_rise));
	box_putf(box, "\\h'-%su/2u'", nreg(num_wd));
	/* output the denominator */
	box_putf(box, "\\h'-%su/2u'", nreg(den_wd));
	box_putf(box, "\\v'%su'%s\\v'-%su'",
		nreg(den_fall), box_toreg(den), nreg(den_fall));
	box_putf(box, "\\h'(-%su+%su)/2u'", nreg(den_wd), nreg(all_wd));
	box_putf(box, "\\h'%sp*%du/100u'",nreg(box->szreg), e_nulldelim);
	box_afterput(box, T_INNER);
	box_toreg(box);
	nregrm(num_wd);
	nregrm(num_dp);
	nregrm(den_wd);
	nregrm(den_ht);
	nregrm(all_wd);
	nregrm(num_rise);
	nregrm(den_fall);
	nregrm(bar_wd);
	nregrm(bar_dp);
	nregrm(bar_ht);
	nregrm(bar_fall);
	nregrm(tmp_15d);
}

/* choose the smallest bracket among br[], large enough for \n(ht+\n(dp */
static void box_bracketsel(int dst, int ht, int dp, char **br, int any, int both)
{
	int i;
	for (i = 0; br[i]; i++) {
		printf(".if '%s'' ", sreg(dst));
		printf(".if \\w'%s' ", br[i]);	/* is this bracket available? */
		if (both) {	/* check both the height and the depth */
			printf(".if (%s-(%dm/100)*2)<=(-\\n[bbury]+\\n[bblly]+(%dm/100*2)) ",
				nreg(ht), e_rulethickness, e_axisheight);
			printf(".if (%s*2)<=(-\\n[bbury]+\\n[bblly]-(%dm/100*2)) ",
				nreg(dp), e_axisheight);
		} else {
			printf(".if (%s+%s)<=(-\\n[bbury]+\\n[bblly]) ", nreg(ht), nreg(dp));
		}
		printf(".ds %s \"%s\n", sregname(dst), br[i]);
	}
	if (any)		/* choose the largest bracket, if any is 1 */
		while (--i >= 0)
			printf(".if '%s'' .if \\w'%s' .ds %s \"%s\n",
				sreg(dst), br[i], sregname(dst), br[i]);
}

/* build a bracket using the provided pieces */
static void box_bracketmk(int dst, int len,
			char *top, char *mid, char *bot, char *cen)
{
	int toplen[4];
	int midlen[4];
	int botlen[4];
	int cenlen[4];
	int mid_cnt = nregmk();	/* number of mid glyphs to insert */
	int mid_cur = nregmk();	/* the number of mid glyphs inserted */
	int cen_pos = nregmk();
	int buildmacro = sregmk();
	blen_mk(top, toplen);
	blen_mk(mid, midlen);
	blen_mk(bot, botlen);
	if (cen)
		blen_mk(cen, cenlen);
	/* the number of mid tokens necessary to cover sub */
	if (!cen) {
		printf(".nr %s %s*2-%s-%s*11/10/%s\n",
			nregname(mid_cnt), nreg(len),
			nreg(toplen[1]), nreg(botlen[1]), nreg(midlen[1]));
		printf(".if %s<0 .nr %s 0\n", nreg(mid_cnt), nregname(mid_cnt));
	} else {	/* for brackets with a center like { */
		printf(".nr %s %s-(%s+%s+%s/2)*11/10/%s\n",
			nregname(cen_pos), nreg(len), nreg(cenlen[1]),
			nreg(toplen[1]), nreg(botlen[1]), nreg(midlen[1]));
		printf(".if %s<0 .nr %s 0\n", nreg(cen_pos), nregname(cen_pos));
		printf(".nr %s 0%s*2\n", nregname(mid_cnt), nreg(cen_pos));
	}
	/* the macro to create the bracket; escaping backslashes */
	printf(".de %s\n", sregname(buildmacro));
	if (cen)		/* inserting cen */
		printf(".if \\%s=\\%s .as %s \"\\v'-\\%su'%s\\h'-\\%su'\\v'-\\%su'\n",
			nreg(mid_cur), nreg(cen_pos), sregname(dst),
			nreg(cenlen[3]), cen, nreg(cenlen[0]), nreg(cenlen[2]));
	printf(".if \\%s<\\%s .as %s \"\\v'-\\%su'%s\\h'-\\%su'\\v'-\\%su'\n",
		nreg(mid_cur), nreg(mid_cnt),
		sregname(dst), nreg(midlen[3]),
		mid, nreg(midlen[0]), nreg(midlen[2]));
	printf(".if \\\\n+%s<\\%s .%s\n",
		escarg(nregname(mid_cur)), nreg(mid_cnt), sregname(buildmacro));
	printf("..\n");
	/* constructing the bracket */
	printf(".ds %s \"\\v'-%su'%s\\h'-%su'\\v'-%su'\n",
		sregname(dst), nreg(botlen[3]),
		bot, nreg(botlen[0]), nreg(botlen[2]));
	printf(".nr %s 0 1\n", nregname(mid_cur));
	printf(".%s\n", sregname(buildmacro));
	printf(".as %s \"\\v'-%su'%s\\h'-%su'\\v'-%su'\n",
		sregname(dst), nreg(toplen[3]), top,
		nreg(toplen[0]), nreg(toplen[2]));
	/* moving back vertically */
	printf(".as %s \"\\v'%su*%su+%su+%su+%su'\n",
		sregname(dst), nreg(mid_cnt), nreg(midlen[1]), nreg(botlen[1]),
		nreg(toplen[1]), cen ? nreg(cenlen[1]) : "0");
	/* moving right */
	printf(".as %s \"\\h'%su'\n",
		sregname(dst), cen ? nreg(cenlen[0]) : nreg(midlen[0]));
	blen_rm(toplen);
	blen_rm(midlen);
	blen_rm(botlen);
	if (cen)
		blen_rm(cenlen);
	nregrm(mid_cnt);
	nregrm(mid_cur);
	nregrm(cen_pos);
	sregrm(buildmacro);
}

static void box_bracket(struct box *box, char *brac, int ht, int dp)
{
	char *sizes[NSIZES] = {NULL};
	char *top = NULL, *mid = NULL, *bot = NULL, *cen = NULL;
	int dst = sregmk();
	int len = nregmk();
	int fall = nregmk();
	int parlen[4];
	roff_max(len, ht, dp);
	def_sizes(brac, sizes);
	printf(".ds %s \"\n", sregname(dst));
	def_pieces(brac, &top, &mid, &bot, &cen);
	box_bracketsel(dst, ht, dp, sizes, !mid, 1);
	if (mid) {
		printf(".if '%s'' \\{\\\n", sreg(dst));
		box_bracketmk(dst, len, top, mid, bot, cen);
		printf(".  \\}\n");
	}
	/* calculating the total vertical length of the bracket */
	blen_mk(sreg(dst), parlen);
	/* calculating the amount the bracket should be moved downwards */
	printf(".nr %s 0-%s+%s/2-(%sp*%du/100u)\n", nregname(fall),
		nreg(parlen[3]), nreg(parlen[2]), nreg(box->szreg), e_axisheight);
	/* printing the output */
	box_putf(box, "\\f[\\n(.f]\\s[\\n(.s]\\v'%su'%s\\v'-%su'",
		nreg(fall), sreg(dst), nreg(fall));
	box_toreg(box);
	blen_rm(parlen);
	sregrm(dst);
	nregrm(len);
	nregrm(fall);
}

static char *bracsign(char *brac, int left)
{
	if (brac[0] == 'c' && !strncmp("ceiling", brac, strlen(brac)))
		return left ? "\\(lc" : "\\(rc";
	if (brac[0] == 'f' && !strncmp("floor", brac, strlen(brac)))
		return left ? "\\(lf" : "\\(rf";
	if (brac[0] == '<' && brac[1] == '\0')
		return "\\(la";
	if (brac[0] == '>' && brac[1] == '\0')
		return "\\(ra";
	return brac;
}

/* build large brackets; the correct font should be set up beforehand */
void box_wrap(struct box *box, struct box *sub, char *left, char *right)
{
	int sublen[4];
	blen_mk(box_toreg(sub), sublen);
	printf(".ps %s\n", nreg(box->szreg));
	if (left) {
		box_beforeput(box, T_LEFT, 0);
		box_bracket(box, bracsign(left, 1), sublen[2], sublen[3]);
		box_afterput(box, T_LEFT);
	}
	box_merge(box, sub, 0);
	if (right) {
		box_beforeput(box, T_RIGHT, 0);
		box_bracket(box, bracsign(right, 0), sublen[2], sublen[3]);
		box_afterput(box, T_RIGHT);
	}
	blen_rm(sublen);
}

/* construct a radical with height at least len and width wd in dst register */
static void sqrt_rad(int dst, int len, int wd)
{
	char *sizes[NSIZES] = {NULL};
	int srlen[4];
	int rnlen[4];
	int sr_sz = nregmk();
	int wd_diff = nregmk();		/* if wd is shorter than \(rn */
	int sr_rx = nregmk();		/* the right-most horizontal position of \(sr */
	int rn_dx = nregmk();		/* horizontal displacement necessary for \(rn */
	int len2 = nregmk();
	int rad = sregmk();
	char *top = NULL, *mid = NULL, *bot = NULL, *cen;
	printf(".nr %s 0%s/2*11/10\n", nregname(len2), nreg(len));
	printf(".ds %s \"\n", sregname(rad));
	/* selecting a radical of the appropriate size */
	def_pieces("\\(sr", &top, &mid, &bot, &cen);
	def_sizes("\\(sr", sizes);
	box_bracketsel(rad, len2, len2, sizes, 0, 0);
	/* constructing the bracket if needed */
	if (mid) {
		printf(".if \\w'%s' ", mid);
		printf(".if '%s'' \\{\\\n", sreg(rad));
		box_bracketmk(rad, len2, top, mid, bot, NULL);
		printf(".  \\}\n");
	}
	/* enlarging \(sr if no suitable glyph was found */
	printf(".if '%s'' \\{\\\n", sreg(rad));
	blen_mk("\\(sr", srlen);
	printf(".ie %s<(%s+%s) .nr %s 0\\n(.s\n",
		nreg(len), nreg(srlen[2]), nreg(srlen[3]), nregname(sr_sz));
	printf(".el .nr %s 0%s*\\n(.s/(%s+%s-(%dm/100u))+1\n",
		nregname(sr_sz), nreg(len),
		nreg(srlen[2]), nreg(srlen[3]), e_rulethickness);
	printf(".ps %s\n", nreg(sr_sz));
	printf(".ds %s \"\\(sr\n", sregname(rad));
	blen_rm(srlen);
	printf(".  \\}\n");
	/* adding the handle */
	blen_mk(sreg(rad), srlen);
	printf(".nr %s \\n[bburx]\n", nregname(sr_rx));
	blen_mk("\\(rn", rnlen);
	printf(".nr %s 0%s-\\n[bbllx]-(%dm/100u)\n",
		nregname(rn_dx), nreg(sr_rx), e_rulethickness);
	printf(".nr %s 0\n", nregname(wd_diff));
	printf(".if %s<%s .nr %s 0%s-%s\n",
		nreg(wd), nreg(rnlen[0]),
		nregname(wd_diff), nreg(rnlen[0]), nreg(wd));
	/* output the radical; align the top of the radical to the baseline */
	printf(".ds %s \"\\s[\\n(.s]\\f[\\n(.f]"
		"\\v'%su'\\h'%su'\\l'%su+%su\\(rn'\\h'-%su'\\v'-%su'"
		"\\h'-%su-%su'\\v'%su'%s\\v'-%su'\\h'%su+%su'\n",
		nregname(dst),
		nreg(rnlen[2]), nreg(rn_dx), nreg(wd), nreg(wd_diff),
		nreg(rn_dx), nreg(rnlen[2]), nreg(wd), nreg(wd_diff),
		nreg(srlen[2]), sreg(rad), nreg(srlen[2]), nreg(wd), nreg(wd_diff));
	blen_rm(srlen);
	blen_rm(rnlen);
	nregrm(sr_sz);
	nregrm(wd_diff);
	nregrm(sr_rx);
	nregrm(rn_dx);
	sregrm(rad);
}

void box_sqrt(struct box *box, struct box *sub)
{
	int sublen[4];
	int radlen[4];
	int rad = sregmk();
	int rad_rise = nregmk();
	int min_ht = nregmk();
	box_italiccorrection(sub);
	box_beforeput(box, T_ORD, 0);
	blen_mk(box_toreg(sub), sublen);
	printf(".ps %s\n", nreg(box->szreg));
	/* 11 */
	printf(".nr %s 0%s+%s+(2*%dm/100u)+(%dm/100u/4)\n",
		nregname(min_ht), nreg(sublen[2]), nreg(sublen[3]),
		e_rulethickness,
		TS_DX(box->style) ? e_xheight : e_rulethickness);
	sqrt_rad(rad, min_ht, sublen[0]);
	blen_mk(sreg(rad), radlen);
	printf(".nr %s 0(%dm/100u)+(%dm/100u/4)\n",
		nregname(rad_rise), e_rulethickness,
		TS_DX(box->style) ? e_xheight : e_rulethickness);
	printf(".if %s>(%s+%s+%s) .nr %s (%s+%s-%s-%s)/2\n",
		nreg(radlen[3]), nreg(sublen[2]), nreg(sublen[3]),
		nreg(rad_rise), nregname(rad_rise),
		nreg(rad_rise), nreg(radlen[3]), nreg(sublen[2]),
		nreg(sublen[3]));
	printf(".nr %s +%s\n", nregname(rad_rise), nreg(sublen[2]));
	/* output the radical */
	box_putf(box, "\\v'-%su'%s\\v'%su'\\h'-%su'%s",
		nreg(rad_rise), sreg(rad), nreg(rad_rise),
		nreg(sublen[0]), box_toreg(sub));
	box_afterput(box, T_ORD);
	box_toreg(box);
	blen_rm(sublen);
	blen_rm(radlen);
	sregrm(rad);
	nregrm(rad_rise);
	nregrm(min_ht);
}

void box_bar(struct box *box)
{
	int box_wd = nregmk();
	int box_ht = nregmk();
	int bar_wd = nregmk();
	int bar_dp = nregmk();
	int bar_rise = nregmk();
	box_italiccorrection(box);
	printf(".ps %s\n", nreg(box->szreg));
	tok_len("\\(ru", bar_wd, 0, 0, bar_dp);
	tok_dim(box_toreg(box), box_wd, box_ht, 0);
	printf(".if %su<(%dm/100u) .nr %s 0%dm/100u\n",
		nreg(box_ht), e_xheight, nregname(box_ht), e_xheight);
	printf(".nr %s 0%su+%su+(3*%dm/100u)\n",
		nregname(bar_rise), nreg(box_ht),
		nreg(bar_dp), e_rulethickness);
	box_putf(box, "\\v'-%su'\\s%s\\f[\\n(.f]\\l'-%su\\(ru'\\v'%su'",
		nreg(bar_rise), escarg(nreg(box->szreg)),
		nreg(box_wd), nreg(bar_rise));
	nregrm(box_wd);
	nregrm(box_ht);
	nregrm(bar_wd);
	nregrm(bar_dp);
	nregrm(bar_rise);
}

void box_accent(struct box *box, char *c)
{
	int box_wd = nregmk();
	int box_ht = nregmk();
	int ac_rise = nregmk();
	int ac_wd = nregmk();
	int ac_dp = nregmk();
	box_italiccorrection(box);
	printf(".ps %s\n", nreg(box->szreg));
	tok_len(c, ac_wd, 0, 0, ac_dp);
	tok_dim(box_toreg(box), box_wd, box_ht, 0);
	printf(".if %su<(%dm/100u) .nr %s 0%dm/100u\n",
		nreg(box_ht), e_xheight, nregname(box_ht), e_xheight);
	printf(".nr %s 0%su+%su+(%sp*10u/100u)\n",
		nregname(ac_rise), nreg(box_ht),
		nreg(ac_dp), nreg(box->szreg));
	box_putf(box, "\\v'-%su'\\h'-%su-%su/2u'\\s%s\\f[\\n(.f]%s\\h'%su-%su/2u'\\v'%su'",
		nreg(ac_rise), nreg(box_wd), nreg(ac_wd),
		escarg(nreg(box->szreg)), c, nreg(box_wd),
		nreg(ac_wd), nreg(ac_rise));
	nregrm(box_wd);
	nregrm(box_ht);
	nregrm(ac_rise);
	nregrm(ac_wd);
	nregrm(ac_dp);
}

void box_under(struct box *box)
{
	int box_wd = nregmk();
	int box_dp = nregmk();
	int bar_wd = nregmk();
	int bar_ht = nregmk();
	int bar_fall = nregmk();
	box_italiccorrection(box);
	printf(".ps %s\n", nreg(box->szreg));
	tok_len("\\(ul", bar_wd, 0, bar_ht, 0);
	tok_dim(box_toreg(box), box_wd, 0, box_dp);
	printf(".if %s<0 .nr %s 0\n", nreg(box_dp), nregname(box_dp));
	printf(".nr %s 0%su+%su+(3*%dm/100u)\n",
		nregname(bar_fall), nreg(box_dp),
		nreg(bar_ht), e_rulethickness);
	box_putf(box, "\\v'%su'\\s%s\\f[\\n(.f]\\l'-%su\\(ul'\\v'-%su'",
		nreg(bar_fall), escarg(nreg(box->szreg)),
		nreg(box_wd), nreg(bar_fall));
	nregrm(box_wd);
	nregrm(box_dp);
	nregrm(bar_wd);
	nregrm(bar_ht);
	nregrm(bar_fall);
}

char *box_toreg(struct box *box)
{
	if (!box->reg) {
		box->reg = sregmk();
		printf(".ds %s \"%s\n", sregname(box->reg), box_buf(box));
	}
	return sreg(box->reg);
}

int box_empty(struct box *box)
{
	return !strlen(box_buf(box));
}

void box_vcenter(struct box *box, struct box *sub)
{
	int wd = nregmk();
	int ht = nregmk();
	int dp = nregmk();
	int fall = nregmk();
	box_beforeput(box, sub->tbeg, 0);
	tok_dim(box_toreg(sub), wd, ht, dp);
	printf(".nr %s 0-%s+%s/2-(%sp*%du/100u)\n", nregname(fall),
		nreg(dp), nreg(ht), nreg(box->szreg), e_axisheight);
	box_putf(box, "\\v'%su'%s\\v'-%su'",
		nreg(fall), box_toreg(sub), nreg(fall));
	box_toreg(box);
	box_afterput(box, sub->tcur);
	nregrm(wd);
	nregrm(ht);
	nregrm(dp);
	nregrm(fall);
}

/* include line-space requests */
void box_vertspace(struct box *box)
{
	int box_wd = nregmk();
	int htroom = nregmk();
	int dproom = nregmk();
	box_italiccorrection(box);
	/* amount of room available before and after this line */
	printf(".nr %s 0+\\n(.vu-%sp+(%sp*%du/100u)\n",
		nregname(htroom), nreg(box->szreg),
		nreg(box->szreg), e_bodyheight);
	printf(".nr %s 0+\\n(.vu-%sp+(%sp*%du/100u)\n",
		nregname(dproom), nreg(box->szreg),
		nreg(box->szreg), e_bodydepth);
	/* appending \x requests */
	tok_dim(box_toreg(box), box_wd, 0, 0);
	printf(".if -\\n[bbury]>%s .as %s \"\\x'\\n[bbury]u+%su'\n",
		nreg(htroom), sregname(box->reg), nreg(htroom));
	printf(".if \\n[bblly]>%s .as %s \"\\x'\\n[bblly]u-%su'\n",
		nreg(dproom), sregname(box->reg), nreg(dproom));
	nregrm(box_wd);
	nregrm(htroom);
	nregrm(dproom);
}

/* put the current width to the given number register */
void box_markpos(struct box *box, char *reg)
{
	box->tomark = reg;
}

/* initialize the length of a pile or column of a matrix */
static void box_colinit(struct box **pile, int n,
			int plen[][4], int wd, int ht)
{
	int i;
	for (i = 0; i < n; i++)
		if (pile[i])
			box_italiccorrection(pile[i]);
	for (i = 0; i < n; i++)
		blen_mk(pile[i] ? box_toreg(pile[i]) : "", plen[i]);
	printf(".nr %s 0%s\n", nregname(wd), nreg(plen[0][0]));
	printf(".nr %s 0%s\n", nregname(ht), nreg(plen[0][2]));
	/* finding the maximum width */
	for (i = 1; i < n; i++) {
		printf(".if %s>%s .nr %s 0+%s\n",
			nreg(plen[i][0]), nreg(wd),
			nregname(wd), nreg(plen[i][0]));
	}
	/* finding the maximum height (vertical length) */
	for (i = 1; i < n; i++) {
		printf(".if %s+%s>%s .nr %s 0+%s+%s\n",
			nreg(plen[i - 1][3]), nreg(plen[i][2]), nreg(ht),
			nregname(ht), nreg(plen[i - 1][3]), nreg(plen[i][2]));
	}
	/* maximum height and the depth of the last row */
	printf(".if %s>%s .nr %s 0+%s\n",
		nreg(plen[n - 1][3]), nreg(ht),
		nregname(ht), nreg(plen[n - 1][3]));
}

/* append the give pile to box */
static void box_colput(struct box **pile, int n, struct box *box,
			int adj, int plen[][4], int wd, int ht)
{
	int i;
	box_putf(box, "\\v'-%du*%su/2u'", n - 1, nreg(ht));
	/* adding the entries */
	for (i = 0; i < n; i++) {
		if (adj == 'c')
			box_putf(box, "\\h'%su-%su/2u'",
				nreg(wd), nreg(plen[i][0]));
		if (adj == 'r')
			box_putf(box, "\\h'%su-%su'",
				nreg(wd), nreg(plen[i][0]));
		box_putf(box, "\\v'%su'%s", i ? nreg(ht) : "0",
			pile[i] ? box_toreg(pile[i]) : "");
		if (adj == 'l')
			box_putf(box, "\\h'-%su'", nreg(plen[i][0]));
		if (adj == 'c')
			box_putf(box, "\\h'-%su+(%su-%su/2u)'",
				nreg(wd), nreg(wd), nreg(plen[i][0]));
		if (adj == 'r')
			box_putf(box, "\\h'-%su'", nreg(wd));
	}
	box_putf(box, "\\v'-%du*%su/2u'\\h'%su'", n - 1, nreg(ht), nreg(wd));
}

/* free the registers allocated for this pile */
static void box_coldone(struct box **pile, int n, int plen[][4])
{
	int i;
	for (i = 0; i < n; i++)
		blen_rm(plen[i]);
}

/* calculate the number of entries in the given pile */
static int box_colnrows(struct box *cols[])
{
	int n = 0;
	while (n < NPILES && cols[n])
		n++;
	return n;
}

void box_pile(struct box *box, struct box **pile, int adj, int rowspace)
{
	int plen[NPILES][4];
	int max_wd = nregmk();
	int max_ht = nregmk();
	int n = box_colnrows(pile);
	box_beforeput(box, T_INNER, 0);
	box_colinit(pile, n, plen, max_wd, max_ht);
	/* inserting spaces between entries */
	printf(".if %s<(%sp*%du/100u) .nr %s (%sp*%du/100u)\n",
		nreg(max_ht), nreg(box->szreg), e_baselinesep,
		nregname(max_ht), nreg(box->szreg), e_baselinesep);
	if (rowspace)
		printf(".nr %s +(%sp*%du/100u)\n",
			nregname(max_ht), nreg(box->szreg), rowspace);
	/* adding the entries */
	box_colput(pile, n, box, adj, plen, max_wd, max_ht);
	box_coldone(pile, n, plen);
	box_afterput(box, T_INNER);
	box_toreg(box);
	nregrm(max_wd);
	nregrm(max_ht);
}

void box_matrix(struct box *box, int ncols, struct box *cols[][NPILES],
		int *adj, int colspace, int rowspace)
{
	int plen[NPILES][NPILES][4];
	int wd[NPILES];
	int ht[NPILES];
	int max_ht = nregmk();
	int max_wd = nregmk();
	int nrows = 0;
	int i;
	box_beforeput(box, T_INNER, 0);
	for (i = 0; i < ncols; i++)
		if (box_colnrows(cols[i]) > nrows)
			nrows = box_colnrows(cols[i]);
	for (i = 0; i < ncols; i++)
		wd[i] = nregmk();
	for (i = 0; i < ncols; i++)
		ht[i] = nregmk();
	/* initializing the columns */
	for (i = 0; i < ncols; i++)
		box_colinit(cols[i], nrows, plen[i], wd[i], ht[i]);
	/* finding the maximum width and height */
	printf(".nr %s 0%s\n", nregname(max_wd), nreg(wd[0]));
	printf(".nr %s 0%s\n", nregname(max_ht), nreg(ht[0]));
	for (i = 1; i < ncols; i++) {
		printf(".if %s>%s .nr %s 0+%s\n",
			nreg(wd[i]), nreg(max_wd),
			nregname(max_wd), nreg(wd[i]));
	}
	for (i = 1; i < ncols; i++) {
		printf(".if %s>%s .nr %s 0+%s\n",
			nreg(ht[i]), nreg(max_ht),
			nregname(max_ht), nreg(ht[i]));
	}
	/* inserting spaces between rows */
	printf(".if %s<(%sp*%du/100u) .nr %s (%sp*%du/100u)\n",
		nreg(max_ht), nreg(box->szreg), e_baselinesep,
		nregname(max_ht), nreg(box->szreg), e_baselinesep);
	if (rowspace)
		printf(".nr %s +(%sp*%du/100u)\n",
			nregname(max_ht), nreg(box->szreg), rowspace);
	/* printing the columns */
	for (i = 0; i < ncols; i++) {
		if (i)		/* space between columns */
			box_putf(box, "\\h'%sp*%du/100u'",
				nreg(box->szreg), e_columnsep + colspace);
		box_colput(cols[i], nrows, box, adj[i],
				plen[i], max_wd, max_ht);
	}
	box_afterput(box, T_INNER);
	box_toreg(box);
	for (i = 0; i < ncols; i++)
		box_coldone(cols[i], nrows, plen[i]);
	for (i = 0; i < ncols; i++)
		nregrm(ht[i]);
	for (i = 0; i < ncols; i++)
		nregrm(wd[i]);
	nregrm(max_wd);
	nregrm(max_ht);
}
