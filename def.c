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
	{"cdot",	"vcenter ."},
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
	{"int",		"roman size +2 \\(in"},
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

/* open brackets are in even indices */
static struct brac {
	char *one;
	char *top;
	char *mid;
	char *bot;
	char *cen;
	char *sizes[8];		/* different sizes of the bracket */
} bracs[] = {
	{"(", "\\(LT", "\\(LX", "\\(LB", NULL,
		{"\\N'parenleftbig'", "\\N'parenleftBig'",
		 "\\N'parenleftbigg'", "\\N'parenleftBigg'"}},
	{")", "\\(RT", "\\(RX", "\\(RB", NULL,
		{"\\N'parenrightbig'", "\\N'parenrightBig'",
		 "\\N'parenrightbigg'", "\\N'parenrightBigg'"}},
	{"[", "\\(lc", "\\(lx", "\\(lf", NULL,
		{"\\N'bracketleftbig'", "\\N'bracketleftBig'",
		 "\\N'bracketleftbigg'", "\\N'bracketleftBigg'"}},
	{"]", "\\(rc", "\\(rx", "\\(rf", NULL,
		{"\\N'bracketrightbig'", "\\N'bracketrightBig'",
		 "\\N'bracketrightbigg'", "\\N'bracketrightBigg'"}},
	{"{", "\\(lt", "\\(bv", "\\(lb", "\\(lk",
		{"\\N'braceleftbig'", "\\N'braceleftBig'",
		 "\\N'braceleftbigg'", "\\N'braceleftBigg'"}},
	{"}", "\\(rt", "\\(bv", "\\(rb", "\\(rk",
		{"\\N'bracerightbig'", "\\N'bracerightBig'",
		 "\\N'bracerightbigg'", "\\N'bracerightBigg'"}},
	{"\\(lc", "\\(lc", "\\(lx", "\\(lx", NULL,
		{"\\N'ceilingleft'",
		 "\\N'ceilingleftbig'", "\\N'ceilingleftBig'",
		 "\\N'ceilingleftbigg'", "\\N'ceilingleftBigg'"}},
	{"\\(rc", "\\(rc", "\\(rx", "\\(rx", NULL,
		{"\\N'ceilingright'",
		 "\\N'ceilingrightbig'", "\\N'ceilingrightBig'",
		 "\\N'ceilingrightbigg'", "\\N'ceilingrightBigg'"}},
	{"\\(lf", "\\(lx", "\\(lx", "\\(lf", NULL,
		{"\\N'floorleft'",
		 "\\N'floorleftbig'", "\\N'floorleftBig'",
		 "\\N'floorleftbigg'", "\\N'floorleftBigg'"}},
	{"\\(rf", "\\(rx", "\\(rx", "\\(rf", NULL,
		{"\\N'floorright'",
		 "\\N'floorrightbig'", "\\N'floorrightBig'",
		 "\\N'floorrightbigg'", "\\N'floorrightBigg'"}},
	{"\\(la", NULL, NULL, NULL, NULL,
		{"\\N'angbracketleft'",
		 "\\N'angbracketleftbig'", "\\N'angbracketleftBig'",
		 "\\N'angbracketleftbigg'", "\\N'angbracketleftBigg'"}},
	{"\\(ra", NULL, NULL, NULL, NULL,
		{"\\N'angbracketright'",
		 "\\N'angbracketrightbig'", "\\N'angbracketrightBig'",
		 "\\N'angbracketrightbigg'", "\\N'angbracketrightBigg'"}},
	{"|", "\\(br", "\\(br", "\\(br"},
	{"|", "\\(br", "\\(br", "\\(br"},
};

static char *sqrt_sizes[8] = {
	"\\(sr", "\\N'radical'",
	"\\N'radicalbig'", "\\N'radicalBig'",
	"\\N'radicalbigg'", "\\N'radicalBigg'",
};

int def_binop(char *s)
{
	int i;
	for (i = 0; i < LEN(binops); i++)
		if (!strcmp(binops[i], s))
			return 1;
	return 0;
}

int def_relop(char *s)
{
	int i;
	for (i = 0; i < LEN(relops); i++)
		if (!strcmp(relops[i], s))
			return 1;
	return 0;
}

int def_punc(char *s)
{
	return !s[1] && strchr(".,;:!", s[0]);
}

int def_left(char *s)
{
	int i;
	for (i = 0; i < LEN(bracs); i += 2)
		if (!strcmp(bracs[i].one, s) && (s[0] != '|' || s[1] != '\0'))
			return 1;
	return 0;
}

int def_right(char *s)
{
	int i;
	for (i = 1; i < LEN(bracs); i += 2)
		if (!strcmp(bracs[i].one, s) && (s[0] != '|' || s[1] != '\0'))
			return 1;
	return 0;
}

/* find the pieces for creating the given bracket */
void def_pieces(char *sign, char **top, char **mid, char **bot, char **cen)
{
	int i;
	for (i = 0; i < LEN(bracs); i++) {
		if (!strcmp(bracs[i].one, sign)) {
			*top = bracs[i].top;
			*mid = bracs[i].mid;
			*bot = bracs[i].bot;
			*cen = bracs[i].cen;
		}
	}
}

/* return different sizes of the given bracket */
void def_sizes(char *sign, char *sizes[])
{
	int i, j;
	sizes[0] = sign;
	for (i = 0; i < LEN(bracs); i++)
		if (!strcmp(bracs[i].one, sign))
			for (j = 0; bracs[i].sizes[j]; j++)
				sizes[j + 1] = bracs[i].sizes[j];
}

/* return sqrt pieces and sizes */
char **def_sqrtpieces(char **top, char **mid, char **bot)
{
	*top = "\\N'radicaltp'";
	*mid = "\\N'radicalvertex'";
	*bot = "\\N'radicalbt'";
	return sqrt_sizes;
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
