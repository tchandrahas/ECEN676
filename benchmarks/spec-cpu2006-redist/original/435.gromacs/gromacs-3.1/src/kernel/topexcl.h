/*
 * $Id: topexcl.h,v 1.8 2002/02/28 10:54:45 spoel Exp $
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

#ifndef _topexcl_h
#define _topexcl_h

static char *SRCID_topexcl_h = "$Id: topexcl.h,v 1.8 2002/02/28 10:54:45 spoel Exp $";
#ifdef HAVE_IDENT
#ident	"@(#) topexcl.h 1.11 11/23/92"
#endif /* HAVE_IDENT */

#include "topio.h"

typedef struct {
  int nr;		/* nr atoms (0 <= i < nr) (atoms->nr)	      	*/
  int nrex;		/* with nrex lists of neighbours		*/
			/* respectively containing zeroth, first	*/
			/* second etc. neigbours (0 <= nre < nrex)	*/
  int **nrexcl;		/* with (0 <= nrx < nrexcl[i][nre]) neigbours 	*/ 
			/* per list stored in one 2d array of lists	*/ 
  int ***a;		/* like this: a[i][nre][nrx]			*/
} t_nextnb;

extern void init_nnb(t_nextnb *nnb, int nr, int nrex);
/* Initiate the arrays for nnb (see above) */

extern void done_nnb(t_nextnb *nnb);
/* Cleanup the nnb struct */

#ifdef DEBUG
#define print_nnb(nnb, s) __print_nnb(nnb, s)
extern void print_nnb(t_nextnb *nnb, char *s);
/* Print the nnb struct */
#else
#define print_nnb(nnb, s)
#endif

extern void gen_nnb(t_nextnb *nnb,t_params plist[]);
/* Generate a t_nextnb structure from bond information. 
 * With the structure you can either generate exclusions
 * or generate angles and dihedrals. The structure must be
 * initiated using init_nnb.
 */

extern void nnb2excl (t_nextnb *nnb, t_block *excl);
/* generate exclusions from nnb */

extern void generate_excl (int nrexcl, int nratoms,
			   t_params plist[],t_block *excl);
/* Generate an exclusion block from bonds and constraints in
 * plist.
 */
#endif	/* _topexcl_h */
