/*
 * NEATEQN MAIN HEADER
 *
 * In Neateqn equations are recursively decomposed into boxes.  eqn.c
 * reads the input and makes eqn boxes by calling appropriate functions
 * from box.c.
 */
/* predefined array sizes */
#define FNLEN		32	/* font name length */
#define SZLEN		32	/* point size length */
#define LNLEN		1000	/* line length */
#define NMLEN		32	/* macro name length */
#define RLEN		8	/* register name length */
#define NPILES		32	/* number of piled items */
#define NSIZES		8	/* number of bracket sizes */
#define GNLEN		32	/* glyph name length */
#define BRLEN		64	/* bracket definition length */

/* registers used by neateqn */
#define EQNSZ		".eqnsz"	/* register for surrounding point size */
#define EQNFN		".eqnfn"	/* register for surrounding font */
#define EQNS		"10"		/* eqn-collected line register */
#define EQNMK		".eqnmk"	/* eqn horizontal mark */

/* helpers */
#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) < (b) ? (b) : (a))
#define LEN(a)		(sizeof(a) / sizeof((a)[0]))

/* token and atom types */
#define T_TOK(t)	((t) & 0x00ff)
#define T_ATOM(t)	((t) & 0x00f0)
#define T_FONT(t)	((t) & T_ITALIC)
#define T_ATOMIDX(t)	((((t) >> 4) & 15) - 1)

#define T_EOF		0x0000		/* parser-specific */
#define T_SPACE		0x0001
#define T_TAB		0x0002
#define T_KEYWORD	0x0003
#define T_ORD		0x0010		/* ordinary */
#define T_LETTER	0x0011
#define T_NUMBER	0x0012
#define T_STRING	0x0013
#define T_GAP		0x0014
#define T_BIGOP		0x0020		/* big operators */
#define T_BINOP		0x0030		/* binary operators */
#define T_RELOP		0x0040		/* relational symbol */
#define T_LEFT		0x0050		/* opening token */
#define T_RIGHT		0x0060		/* closing token */
#define T_PUNC		0x0070		/* punctuation symbol */
#define T_INNER		0x0080		/* like fractions */

#define T_ITALIC	0x0100		/* atom with italic font */

/* spaces in hundredths of em */
#define S_S1		e_thinspace	/* thin space */
#define S_S2		e_mediumspace	/* medium space */
#define S_S3		e_thickspace	/* thick space */

/* small helper functions */
void errdie(char *msg);

/* reading the source */
int src_next(void);
void src_back(int c);
void src_define(char *name, char *def);
int src_expand(char *name, char **args);
int src_macro(char *name);
int src_arg(int i);
int src_top(void);
int src_lineget(void);
void src_lineset(int n);
void src_done(void);

/* tokenizer */
int tok_eqn(void);
void tok_eqnout(char *s);
char *tok_get(void);
char *tok_pop(void);
char *tok_poptext(int sep);
int tok_jmp(char *kwd);
int tok_type(void);
int tok_chops(int soft);
void tok_delim(void);
void tok_macro(void);
int tok_inline(void);

/* default definitions and operators */
int def_type(char *s);
void def_typeput(char *s, int type);
int def_chopped(int c);
void def_choppedset(char *s);
void def_pieces(char *sign, char **top, char **mid, char **bot, char **cen);
void def_sizes(char *sign, char *sizes[]);
int def_brcost(int type);
void def_piecesput(char *sign, char *top, char *mid, char *bot, char *cen);
void def_sizesput(char *sign, char *sizes[]);
void def_brcostput(int type, int cost);
extern char *def_macros[][2];

/* variable length string buffer */
struct sbuf {
	char *s;		/* allocated buffer */
	int sz;			/* buffer size */
	int n;			/* length of the string stored in s */
};

void sbuf_init(struct sbuf *sbuf);
void sbuf_done(struct sbuf *sbuf);
char *sbuf_buf(struct sbuf *sbuf);
void sbuf_add(struct sbuf *sbuf, int c);
void sbuf_append(struct sbuf *sbuf, char *s);
void sbuf_printf(struct sbuf *sbuf, char *s, ...);
void sbuf_cut(struct sbuf *sbuf, int n);
int sbuf_len(struct sbuf *sbuf);
int sbuf_empty(struct sbuf *sbuf);

/* tex styles */
#define TS_D		0x00
#define TS_D0		0x01
#define TS_T		0x02
#define TS_T0		0x03
#define TS_S		0x10
#define TS_S0		0x11
#define TS_SS		0x20
#define TS_SS0		0x21

#define TS_MK(s, p)	(((s) << 4) | (p))
#define TS_0(s)		((s) & 0x01)	/* primed variant */
#define TS_MK0(s)	((s) | 0x01)	/* make style primed */
#define TS_SZ(s)	((s) >> 4)	/* character size */
#define TS_DX(s)	((s) == TS_D || (s) == TS_D0)
#define TS_TXT(s)	((s) & 0x02)	/* text style */

int ts_sup(int style);
int ts_sub(int style);
int ts_denom(int style);
int ts_num(int style);

/* equations */
struct box {
	struct sbuf raw;	/* the contents */
	int szreg, szown;	/* number register holding box size */
	int reg;		/* register holding the contents */
	int atoms;		/* the number of atoms inserted */
	int tbeg, tcur;		/* type of the first and the last atoms */
	int style;		/* tex style (TS_*) */
	char *tomark;		/* register for saving box width */
};

struct box *box_alloc(int szreg, int at_pre, int style);
void box_free(struct box *box);
void box_puttext(struct box *box, int type, char *s, ...);
void box_putf(struct box *box, char *s, ...);
int box_size(struct box *box, char *val);
void box_merge(struct box *box, struct box *sub, int breakable);
void box_sub(struct box *box, struct box *sub, struct box *sup);
void box_from(struct box *box, struct box *lim, struct box *llim, struct box *ulim);
void box_over(struct box *box, struct box *num, struct box *denom);
void box_sqrt(struct box *box, struct box *sub);
void box_bar(struct box *box);
void box_under(struct box *box);
void box_accent(struct box *box, char *c);
void box_wrap(struct box *box, struct box *sub, char *left, char *right);
void box_move(struct box *box, int dy, int dx);
void box_gap(struct box *box, int n);
char *box_buf(struct box *box);
char *box_toreg(struct box *box);
void box_vertspace(struct box *box);
int box_empty(struct box *box);
void box_markpos(struct box *box, char *regname);
void box_vcenter(struct box *box, struct box *sub);
void box_pile(struct box *box, struct box **pile, int adj, int rowspace);
void box_matrix(struct box *box, int ncols, struct box *cols[][NPILES],
		int *adj, int colspace, int rowspace);

/* managing registers */
char *escarg(char *arg);
int sregmk(void);
void sregrm(int id);
int nregmk(void);
void nregrm(int id);
char *nreg(int id);
char *sreg(int id);
char *nregname(int id);
char *sregname(int id);
void reg_reset(void);

/* eqn global variables */
extern int e_axisheight;
extern int e_minimumsize;
extern int e_overhang;
extern int e_nulldelim;
extern int e_scriptspace;
extern int e_thinspace;
extern int e_mediumspace;
extern int e_thickspace;
extern int e_num1;
extern int e_num2;
extern int e_denom1;
extern int e_denom2;
extern int e_sup1;
extern int e_sup2;
extern int e_sup3;
extern int e_sub1;
extern int e_sub2;
extern int e_supdrop;
extern int e_subdrop;
extern int e_xheight;
extern int e_rulethickness;
extern int e_bigopspacing1;
extern int e_bigopspacing2;
extern int e_bigopspacing3;
extern int e_bigopspacing4;
extern int e_bigopspacing5;
extern int e_columnsep;
extern int e_baselinesep;
extern int e_bodyheight;
extern int e_bodydepth;
void def_set(char *name, int val);
