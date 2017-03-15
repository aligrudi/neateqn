#include <stdio.h>
#include <string.h>
#include "eqn.h"

/* null-terminated list of default macros */
char *def_macros[][2] = {
	{"<-",		"\\(<-"},
	{"<=",		"\\(<="},
	{">=",		"\\(>="},
	{"==",		"\\(=="},
	{"->",		"\\(->"},
	{"!=",		"\\(!="},
	{"+-",		"\\(+-"},
	{"...",		"vcenter roman \"\\ .\\ .\\ .\\ \""},
	{",...,",	"roman \",\\ .\\ .\\ .\\ ,\\|\""},
	{"ALPHA",	"\\(*A"},
	{"BETA",	"\\(*B"},
	{"CHI",		"\\(*X"},
	{"DELTA",	"\\(*D"},
	{"EPSILON",	"\\(*E"},
	{"ETA",		"\\(*Y"},
	{"GAMMA",	"\\(*G"},
	{"IOTA",	"\\(*I"},
	{"KAPPA",	"\\(*K"},
	{"LAMBDA",	"\\(*L"},
	{"MU",		"\\(*M"},
	{"NU",		"\\(*N"},
	{"OMEGA",	"\\(*W"},
	{"OMICRON",	"\\(*O"},
	{"PHI",		"\\(*F"},
	{"PI",		"\\(*P"},
	{"PSI",		"\\(*Q"},
	{"RHO",		"\\(*R"},
	{"SIGMA",	"\\(*S"},
	{"TAU",		"\\(*T"},
	{"THETA",	"\\(*H"},
	{"UPSILON",	"\\(*U"},
	{"XI",		"\\(*C"},
	{"ZETA",	"\\(*Z"},
	{"alpha",	"\\(*a"},
	{"beta",	"\\(*b"},
	{"chi",		"\\(*x"},
	{"delta",	"\\(*d"},
	{"epsilon",	"\\(*e"},
	{"eta",		"\\(*y"},
	{"gamma",	"\\(*g"},
	{"iota",	"\\(*i"},
	{"kappa",	"\\(*k"},
	{"lambda",	"\\(*l"},
	{"mu",		"\\(*m"},
	{"nu",		"\\(*n"},
	{"omega",	"\\(*w"},
	{"omicron",	"\\(*o"},
	{"phi",		"\\(*f"},
	{"pi",		"\\(*p"},
	{"psi",		"\\(*q"},
	{"rho",		"\\(*r"},
	{"sigma",	"\\(*s"},
	{"tau",		"\\(*t"},
	{"theta",	"\\(*h"},
	{"upsilon",	"\\(*u"},
	{"xi",		"\\(*c"},
	{"zeta",	"\\(*z"},
	{"Im",		"roman \"Im\""},
	{"Re",		"roman \"Re\""},
	{"and",		"roman \"and\""},
	{"approx",	"\"\\v'-.2m'\\z\\(ap\\v'.25m'\\(ap\\v'-.05m'\""},
	{"arc",		"roman \"arc\""},
	{"cdot",	"\\(c."},
	{"cos",		"roman \"cos\""},
	{"cosh",	"roman \"cosh\""},
	{"coth",	"roman \"coth\""},
	{"del",		"\\(gr"},
	{"det",		"roman \"det\""},
	{"dollar",	"roman $"},
	{"exp",		"roman \"exp\""},
	{"for",		"roman \"for\""},
	{"grad",	"\\(gr"},
	{"half",	"roman \\(12"},
	{"if",		"roman \"if\""},
	{"inf",		"\\(if"},
	{"infinity",	"\\(if"},
	{"int",		"{vcenter roman size +2 \\(is}"},
	{"inter",	"roman size +2 \\(ca"},
	{"lim",		"roman \"lim\""},
	{"ln",		"roman \"ln\""},
	{"log",		"roman \"log\""},
	{"max",		"roman \"max\""},
	{"min",		"roman \"min\""},
	{"nothing",	""},
	{"partial",	"\\(pd"},
	{"prime",	"roman \\(fm"},
	{"prod",	"{vcenter roman size +2 \\(pr}"},
	{"sin",		"roman \"sin\""},
	{"sinh",	"roman \"sinh\""},
	{"sum",		"{vcenter roman size +2 \\(su}"},
	{"tan",		"roman \"tan\""},
	{"tanh",	"roman \"tanh\""},
	{"times",	"\\(mu"},
	{"union",	"roman size +2 \\(cu"},
	{NULL, NULL}
};

/* list of binary operations */
static char *binops[] = {
	"+", "\\(pl",
	"−", "-", "\\(mi",
	"÷", "\\(-:", "\\(di",
	"×", "xx", "\\(mu",
	"±", "\\(+-",
	"⊗", "\\(Ox",
	"⊕", "\\(O+",
	"∧", "\\(l&",
	"∨", "\\(l|",
	"∩", "\\(ca",
	"∪", "\\(cu",
	"⋅", "\\(c.",
};

/* list of relations */
static char *relops[] = {
	"<", ">", ":=",
	"=", "\\(eq",
	"≅", "\\(cg",
	"≤", "\\(<=",
	"≥", "\\(>=",
	"≠", "\\(!=",
	"≡", "\\(==",
	"≈", "\\(~~",
	"⊃", "\\(sp",
	"⊇", "\\(ip",
	"⊄", "\\(!b",
	"⊂", "\\(sb",
	"⊆", "\\(ib",
	"∈", "\\(mo",
	"∉", "\\(!m",
	"↔", "\\(ab",
	"←", "\\(<-",
	"↑", "\\(ua",
	"→", "\\(->",
	"↓", "\\(da",
};

/* list of punctuations */
static char *puncs[] = {".", ",", ";", ":", "!"};

/* left and right brackets */
static char *bracketleft[] = {"(", "[", "{", "\\(lc", "\\(lf", "\\(la"};
static char *bracketright[] = {")", "]", "}", "\\(rc", "\\(rf", "\\(ra"};

/* glyphs for different bracket sizes */
static char bracketsizes[32][NSIZES][BRLEN] = {
	{"(", "(", "\\N'parenleftbig'", "\\N'parenleftBig'",
	 "\\N'parenleftbigg'", "\\N'parenleftBigg'"},
	{")", ")", "\\N'parenrightbig'", "\\N'parenrightBig'",
	 "\\N'parenrightbigg'", "\\N'parenrightBigg'"},
	{"[", "[", "\\N'bracketleftbig'", "\\N'bracketleftBig'",
	 "\\N'bracketleftbigg'", "\\N'bracketleftBigg'"},
	{"]", "]", "\\N'bracketrightbig'", "\\N'bracketrightBig'",
	 "\\N'bracketrightbigg'", "\\N'bracketrightBigg'"},
	{"{", "{", "\\N'braceleftbig'", "\\N'braceleftBig'",
	 "\\N'braceleftbigg'", "\\N'braceleftBigg'"},
	{"}", "}", "\\N'bracerightbig'", "\\N'bracerightBig'",
	 "\\N'bracerightbigg'", "\\N'bracerightBigg'"},
	{"\\(lc", "\\N'ceilingleft'", "\\N'ceilingleftbig'", "\\N'ceilingleftBig'",
	 "\\N'ceilingleftbigg'", "\\N'ceilingleftBigg'", "\\(lc"},
	{"\\(rc", "\\N'ceilingright'", "\\N'ceilingrightbig'", "\\N'ceilingrightBig'",
	 "\\N'ceilingrightbigg'", "\\N'ceilingrightBigg'", "\\(rc"},
	{"\\(lf", "\\N'floorleft'", "\\N'floorleftbig'", "\\N'floorleftBig'",
	 "\\N'floorleftbigg'", "\\N'floorleftBigg'", "\\(lf"},
	{"\\(rf", "\\N'floorright'", "\\N'floorrightbig'", "\\N'floorrightBig'",
	 "\\N'floorrightbigg'", "\\N'floorrightBigg'", "\\(rf"},
	{"\\(la", "\\(la", "\\N'angbracketleft'", "\\N'angbracketleftbig'",
	 "\\N'angbracketleftBig'", "\\N'angbracketleftbigg'", "\\N'angbracketleftBigg'"},
	{"\\(ra", "\\(ra", "\\N'angbracketright'", "\\N'angbracketrightbig'",
	 "\\N'angbracketrightBig'", "\\N'angbracketrightbigg'", "\\N'angbracketrightBigg'"},
	{"|", "|", "|", "|", "|"},
	{"\\(sr", "\\(sr", "\\N'radical'", "\\N'radicalbig'", "\\N'radicalBig'",
	 "\\N'radicalbigg'", "\\N'radicalBigg'"},
};

/* large glyph pieces: name, top, mid, bot, centre */
static char bracketpieces[32][8][BRLEN] = {
	{"(", "\\(LT", "\\(LX", "\\(LB"},
	{")", "\\(RT", "\\(RX", "\\(RB"},
	{"[", "\\(lc", "\\(lx", "\\(lf"},
	{"]", "\\(rc", "\\(rx", "\\(rf"},
	{"{", "\\(lt", "\\(bv", "\\(lb", "\\(lk"},
	{"}", "\\(rt", "\\(bv", "\\(rb", "\\(rk"},
	{"\\(lc", "\\(lc", "\\(lx", "\\(lx"},
	{"\\(rc", "\\(rc", "\\(rx", "\\(rx"},
	{"\\(lf", "\\(lx", "\\(lx", "\\(lf"},
	{"\\(rf", "\\(rx", "\\(rx", "\\(rf"},
	{"|", "|", "|", "|"},
	{"\\(sr", "\\N'radicaltp'", "\\N'radicalvertex'", "\\N'radicalbt'"},
};

/* custom glyph types */
static struct gtype {
	char g[GNLEN];
	int type;
} gtypes[128];

void def_typeput(char *s, int type)
{
	int i;
	for (i = 0; i < LEN(gtypes) && gtypes[i].g[0]; i++)
		if (!strcmp(s, gtypes[i].g))
			break;
	if (i < LEN(gtypes)) {
		strcpy(gtypes[i].g, s);
		gtypes[i].type = type;
	}
}

/* find an entry in an array */
static char *alookup(char **a, int len, char *s)
{
	int i;
	for (i = 0; i < len; i++)
		if (!strcmp(s, a[i]))
			return a[i];
	return NULL;
}

int def_type(char *s)
{
	int i;
	for (i = 0; i < LEN(gtypes) && gtypes[i].g[0]; i++)
		if (!strcmp(s, gtypes[i].g))
			return gtypes[i].type;
	if (alookup(puncs, LEN(puncs), s))
		return T_PUNC;
	if (alookup(binops, LEN(binops), s))
		return T_BINOP;
	if (alookup(relops, LEN(relops), s))
		return T_RELOP;
	if (alookup(bracketleft, LEN(bracketleft), s))
		return T_LEFT;
	if (alookup(bracketright, LEN(bracketright), s))
		return T_RIGHT;
	return -1;
}

static int pieces_find(char *sign)
{
	int i;
	for (i = 0; i < LEN(bracketpieces); i++)
		if (!strcmp(bracketpieces[i][0], sign))
			return i;
	return -1;
}

/* find the pieces for creating the given bracket */
void def_pieces(char *sign, char **top, char **mid, char **bot, char **cen)
{
	int i = pieces_find(sign);
	if (i >= 0) {
		*top = bracketpieces[i][1][0] ? bracketpieces[i][1] : NULL;
		*mid = bracketpieces[i][2][0] ? bracketpieces[i][2] : NULL;
		*bot = bracketpieces[i][3][0] ? bracketpieces[i][3] : NULL;
		*cen = bracketpieces[i][4][0] ? bracketpieces[i][4] : NULL;
	}
}

void def_piecesput(char *sign, char *top, char *mid, char *bot, char *cen)
{
	int i = pieces_find(sign);
	if (i < 0 && (i = pieces_find("")) < 0)
		return;
	snprintf(bracketpieces[i][0], sizeof(bracketpieces[i][0]), "%s", sign);
	snprintf(bracketpieces[i][1], sizeof(bracketpieces[i][1]), "%s", top);
	snprintf(bracketpieces[i][2], sizeof(bracketpieces[i][2]), "%s", mid);
	snprintf(bracketpieces[i][3], sizeof(bracketpieces[i][3]), "%s", bot);
	snprintf(bracketpieces[i][4], sizeof(bracketpieces[i][4]), "%s", cen);
}

static int sizes_find(char *sign)
{
	int i;
	for (i = 0; i < LEN(bracketsizes); i++)
		if (!strcmp(bracketsizes[i][0], sign))
			return i;
	return -1;
}

/* return different sizes of the given bracket */
void def_sizes(char *sign, char *sizes[])
{
	int idx = sizes_find(sign);
	int i;
	sizes[0] = sign;
	for (i = 1; idx >= 0 && i < NSIZES; i++)
		sizes[i - 1] = bracketsizes[idx][i][0] ? bracketsizes[idx][i] : NULL;
}

void def_sizesput(char *sign, char *sizes[])
{
	int idx = sizes_find(sign);
	int i;
	if (idx < 0 && (idx = sizes_find("")) < 0)
		return;
	snprintf(bracketsizes[idx][0], sizeof(bracketsizes[idx][0]), "%s", sign);
	for (i = 1; i < NSIZES; i++)
		snprintf(bracketsizes[idx][i], sizeof(bracketsizes[idx][i]),
			"%s", sizes[i - 1] ? sizes[i - 1] : "");
}

/* global variables */
int e_axisheight = 23;	/* axis height */
int e_minimumsize = 5;	/* minimum size */
int e_overhang = 7;
int e_nulldelim = 12;
int e_scriptspace = 12;
int e_thinspace = 17;
int e_mediumspace = 22;
int e_thickspace = 28;
int e_num1 = 70;	/* minimum numerator rise */
int e_num2 = 40;
int e_denom1 = 70;	/* minimum denominator fall */
int e_denom2 = 36;
int e_sup1 = 42;
int e_sup2 = 37;
int e_sup3 = 28;
int e_sub1 = 20;
int e_sub2 = 23;
int e_supdrop = 38;
int e_subdrop = 5;
int e_xheight = 45;
int e_rulethickness = 4;
int e_bigopspacing1 = 11;
int e_bigopspacing2 = 17;
int e_bigopspacing3 = 20;
int e_bigopspacing4 = 60;
int e_bigopspacing5 = 10;
int e_columnsep = 100;
int e_baselinesep = 140;
int e_bodyheight = 70;
int e_bodydepth = 25;

static struct gvar {
	char *name;
	int *ref;
} gvars[] = {
	{"axis_height", &e_axisheight},
	{"minimum_size", &e_minimumsize},
	{"over_hang", &e_overhang},
	{"null_delimiter_space", &e_nulldelim},
	{"script_space", &e_scriptspace},
	{"thin_space", &e_thinspace},
	{"medium_space", &e_mediumspace},
	{"thick_space", &e_thickspace},
	{"num1", &e_num1},
	{"num2", &e_num2},
	{"denom1", &e_denom1},
	{"denom2", &e_denom2},
	{"sup1", &e_sup1},
	{"sup2", &e_sup2},
	{"sup3", &e_sup3},
	{"sub1", &e_sub1},
	{"sub2", &e_sub2},
	{"sup_drop", &e_supdrop},
	{"sub_drop", &e_subdrop},
	{"x_height", &e_xheight},
	{"default_rule_thickness", &e_rulethickness},
	{"big_op_spacing1", &e_bigopspacing1},
	{"big_op_spacing2", &e_bigopspacing2},
	{"big_op_spacing3", &e_bigopspacing3},
	{"big_op_spacing4", &e_bigopspacing4},
	{"big_op_spacing5", &e_bigopspacing5},
	{"column_sep", &e_columnsep},
	{"baseline_sep", &e_baselinesep},
	{"body_height", &e_bodyheight},
	{"body_depth", &e_bodydepth},
};

void def_set(char *name, int val)
{
	int i;
	for (i = 0; i < LEN(gvars); i++)
		if (!strcmp(gvars[i].name, name))
			*gvars[i].ref = val;
}

/* superscript style */
int ts_sup(int style)
{
	int sz = MIN(2, TS_SZ(style) + 1);
	return TS_MK(sz, TS_0(style));
}

/* subscript style */
int ts_sub(int style)
{
	int sz = MIN(2, TS_SZ(style) + 1);
	return TS_MK(sz, 1);
}

/* numerator style */
int ts_num(int style)
{
	int sz;
	if (TS_DX(style))
		return TS_0(style) ? TS_T0 : TS_T;
	sz = MIN(2, TS_SZ(style) + 1);
	return TS_MK(sz, TS_0(style));
}

/* denominator style */
int ts_denom(int style)
{
	int sz;
	if (TS_DX(style))
		return TS_T0;
	sz = MIN(2, TS_SZ(style) + 1);
	return TS_MK(sz, 1);
}

/* extra line-break cost */
static int brcost_type[32];
static int brcost_cost[32];
static int brcost_n;

int def_brcost(int type)
{
	int i;
	for (i = 0; i < brcost_n; i++)
		if (brcost_type[i] == type && brcost_cost[i] > 0)
			return brcost_cost[i];
	return 100000;
}

void def_brcostput(int type, int cost)
{
	int i;
	if (type == 0)
		brcost_n = 0;
	for (i = 0; i < brcost_n; i++)
		if (brcost_type[i] == type)
			break;
	if (type <= 0 || i + (i >= brcost_n) >= LEN(brcost_type))
		return;
	brcost_type[i] = type;
	brcost_cost[i] = cost;
	if (i >= brcost_n)
		brcost_n = i + 1;
}

/* at which characters equations are chopped */
static char chopped[256] = "^~\"\t";

int def_chopped(int c)
{
	return strchr("\n {}", c) != NULL || strchr(chopped, c) != NULL;
}

void def_choppedset(char *c)
{
	strcpy(chopped, c);
}
