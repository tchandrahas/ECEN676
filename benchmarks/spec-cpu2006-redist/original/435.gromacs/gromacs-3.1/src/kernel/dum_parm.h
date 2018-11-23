/*
 * $Id: dum_parm.h,v 1.7 2002/02/28 10:54:41 spoel Exp $
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

#ifndef _dum_parm_h
#define _dum_parm_h

static char *SRCID_dum_parm_h = "$Id: dum_parm.h,v 1.7 2002/02/28 10:54:41 spoel Exp $";
#include "typedefs.h"
#include "grompp.h"

extern int set_dummies(bool bVerbose, t_atoms *atoms,  t_atomtype atype,
		       t_params plist[]);
/* set parameters for dummy atoms, return number of dummies */

extern void set_dummies_ptype(bool bVerbose, t_idef *idef, t_atoms *atoms);
/* set ptype to Dummy for dummy atoms */

extern void clean_dum_bondeds(t_params *ps, int natoms, bool bRmDumBds);
/* remove all bonded interaction (bonds, angles and diherals) that
   have become obsolete due to dummy atom constructions */

#endif	/* _dum_parm_h */
