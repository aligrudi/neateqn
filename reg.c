#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eqn.h"

#define NREGS		2048
#define EPREFIX		""

static int sreg_max;		/* maximum allocated string register */
static int sreg_free[NREGS];	/* free string registers */
static int sreg_n;		/* number of items in sreg_free[] */
static char sreg_name[NREGS][RLEN];
static char sreg_read[NREGS][RLEN];

static int nreg_max;
static int nreg_free[NREGS];
static int nreg_n;
static char nreg_name[NREGS][RLEN];
static char nreg_read[NREGS][RLEN];

/* allocate a troff string register */
int sregmk(void)
{
	int id = sreg_n ? sreg_free[--sreg_n] : ++sreg_max;
	sprintf(sreg_name[id], "%s%02d", EPREFIX, id);
	sprintf(sreg_read[id], "\\*%s", escarg(sreg_name[id]));
	return id;
}

/* free a troff string register */
void sregrm(int id)
{
	sreg_free[sreg_n++] = id;
}

char *sregname(int id)
{
	return sreg_name[id];
}

char *sreg(int id)
{
	return sreg_read[id];
}

/* allocate a troff number register */
int nregmk(void)
{
	int id = nreg_n ? nreg_free[--nreg_n] : ++nreg_max;
	sprintf(nreg_name[id], "%s%02d", EPREFIX, id);
	sprintf(nreg_read[id], "\\n%s", escarg(nreg_name[id]));
	return id;
}

/* free a troff number register */
void nregrm(int id)
{
	nreg_free[nreg_n++] = id;
}

char *nregname(int id)
{
	return nreg_name[id];
}

char *nreg(int id)
{
	return nreg_read[id];
}

/* free all allocated registers */
void reg_reset(void)
{
	nreg_max = 0;
	nreg_n = 0;
	sreg_max = 11;
	sreg_n = 0;
}

/* format the argument of a troff escape like \s or \f */
char *escarg(char *arg)
{
	static char buf[256];
	if (!arg[1])
		sprintf(buf, "%c", arg[0]);
	else if (!arg[2])
		sprintf(buf, "(%c%c", arg[0], arg[1]);
	else
		sprintf(buf, "[%s]", arg);
	return buf;
}
