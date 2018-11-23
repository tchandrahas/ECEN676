/*
 * $Id: toppush.h,v 1.8 2002/02/28 10:54:45 spoel Exp $
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
 * Gromacs Runs One Microsecond At Cannonball Speeds
 */

#ifndef _toppush_h
#define _toppush_h

static char *SRCID_toppush_h = "$Id: toppush.h,v 1.8 2002/02/28 10:54:45 spoel Exp $";
#ifdef HAVE_IDENT
#ident	"@(#) toppush.h 1.31 9/30/97"
#endif /* HAVE_IDENT */

#include "typedefs.h"
#include "toputil.h"

typedef struct {
  int     nr;	/* The number of entries in the list 			*/
  int     nra2; /* The total number of entries in a			*/
  atom_id *nra;	/* The number of entries in each a array (dim nr) 	*/
  atom_id **a;	/* The atom numbers (dim nr) the length of each element	*/
		/* i is nra[i]						*/
} t_block2;

extern void generate_nbparams(int comb,int funct,t_params plist[],
			      t_atomtype *atype,real npow);
			      
extern void push_at (t_symtab *symtab, t_atomtype *at, char *line,int nb_funct);

extern void push_bt(directive d,t_params bt[], int nral, 
		    t_atomtype *at, char *line);

extern void push_nbt(directive d,t_params nbt[],t_atomtype *atype,
		     char *plines,int nb_funct);

extern void push_atom(t_symtab   *symtab, 
		      t_block    *cgs,
		      t_atoms    *at,
		      t_atomtype *atype,
		      char       *line);

extern void push_bondnow (t_params *bond, t_param *b);

extern void push_bond(directive d,t_params bondtype[],t_params bond[],
		      t_atoms *at,char *line);

extern void push_mol(int nrmols,t_molinfo mols[],char *pline,
		     int *whichmol,int *nrcopies);

extern void push_molt(t_symtab *symtab,t_molinfo *newmol,char *line);

extern void init_block2(t_block2 *b2, int natom);
	
extern void done_block2(t_block2 *b2);

extern void push_excl(char *line, t_block2 *b2);

extern void merge_excl(t_block *excl, t_block2 *b2);

extern void b_to_b2(t_block *b, t_block2 *b2);

extern void b2_to_b(t_block2 *b2, t_block *b);

#endif	/* _toppush_h */
