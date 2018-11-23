/*
 * $Id: pdb2top.h,v 1.28 2002/02/28 10:54:44 spoel Exp $
 * 
 *                This source code is part of
 * 
 *                 G   R   O   M   A   C   S
 * 
 *          GROningen MAchine for Chemical Simulations
 * 
 *                        VERSION 3.1
 * Copyright (c) 1991-2001, University of Groningen, The Netherlands
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * If you want to redistribute modifications, please consider that
 * scientific software is very special. Version control is crucial -
 * bugs must be traceable. We will be happy to consider code for
 * inclusion in the official distribution, but derived work must not
 * be called official GROMACS. Details are found in the README & COPYING
 * files - if they are missing, get the official version at www.gromacs.org.
 * 
 * To help us fund GROMACS development, we humbly ask that you cite
 * the papers on the package - you can find them in the top README file.
 * 
 * For more info, check our website at http://www.gromacs.org
 * 
 * And Hey:
 * GROningen Mixture of Alchemy and Childrens' Stories
 */

#ifndef _pdb2top_h
#define _pdb2top_h

static char *SRCID_pdb2top_h = "$Id: pdb2top.h,v 1.28 2002/02/28 10:54:44 spoel Exp $";
#ifdef HAVE_IDENT
#ident	"@(#) pdb2top.h 1.19 2/2/97"
#endif /* HAVE_IDENT */
#include "typedefs.h"
#include "toputil.h"
#include "hackblock.h"

/* this *MUST* correspond to array in pdb2top.c */
enum { ehisA, ehisB, ehisH, ehis1, ehisNR };
extern char *hh[ehisNR];

typedef struct {
  int  res1,res2;
  char *a1,*a2;
} t_ssbond;

typedef struct {
  char *name;
  int  nr;
} t_mols;

extern char *choose_ff(void);
/* Strange place for this function... */

extern void print_top_comment(FILE *out,char *filename,char *title,bool bITP);

extern void print_top_header(FILE *out,char *filename,char *title,bool bITP, 
			     char *ff,real mHmult);

extern void print_top_mols(FILE *out, char *title, int nincl, char **incls,
			   int nmol, t_mols *mols);

extern void pdb2top(FILE *top_file, char *posre_fn, char *molname,
		    t_atoms *atoms,rvec **x,
		    t_atomtype *atype,t_symtab *tab,
		    int bts[],
		    int nrtp, t_restp rtp[],
		    int nterpairs, t_hackblock **ntdb, t_hackblock **ctdb,
		    int *rn, int *rc, bool bH14, bool bAlldih,
		    bool bDummies, bool bDummyAromatics, real mHmult,
		    int nssbonds, t_ssbond ssbonds[], int nrexcl, 
		    real long_bond_dist, real short_bond_dist,
		    bool bDeuterate);
/* Create a topology ! */
extern bool is_int(double x);
/* Returns TRUE when x is integer */

extern void print_sums(t_atoms *atoms, bool bSystem);

extern void write_top(FILE *out, char *pr,char *molname,
		      t_atoms *at,int bts[],t_params plist[],t_excls excls[],
		      t_atomtype *atype,int *cgnr, int nrexcl);
/* NOTE: nrexcl is not the size of *excl! */

#endif	/* _pdb2top_h */
