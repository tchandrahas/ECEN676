/*
 * $Id: add_par.h,v 1.10 2002/02/28 10:54:41 spoel Exp $
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

#ifndef _add_par_h
#define _add_par_h

static char *SRCID_add_par_h = "$Id: add_par.h,v 1.10 2002/02/28 10:54:41 spoel Exp $";
#include "typedefs.h"
#include "pdb2top.h"

extern void add_param(t_params *ps, int ai, int aj, real *c, char *s);

extern void add_imp_param(t_params *ps, int ai, int aj, int ak, int al,
			  real c0, real c1, char *s);
			  
extern void add_dih_param(t_params *ps,int ai,int aj,int ak,int al,
			  real c0, real c1, real c2, char *s);

extern void add_dum2_atoms(t_params *ps, int ai, int aj, int ak);

extern void add_dum3_atoms(t_params *ps, int ai, int aj, int ak, int al, 
			   bool bSwapParity);

extern void add_dum3_param(t_params *ps, int ai, int aj, int ak, int al, 
			   real c0, real c1);

extern void add_dum4_atoms(t_params *ps, int ai, int aj, int ak, int al, 
			   int am);

extern int search_jtype(t_restp *rp,char *name,bool bFirstRes);

extern void cp_param(t_param *dest,t_param *src);

#endif	/* _add_par_h */
