/*
 * $Id: addconf.h,v 1.17 2002/02/28 11:00:22 spoel Exp $
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
static char *SRCID_addconf_h = "$Id: addconf.h,v 1.17 2002/02/28 11:00:22 spoel Exp $";
#include "typedefs.h"

extern 
void add_conf(t_atoms *atoms, rvec **x, rvec **v, real **r, bool bSrenew, 
	      matrix box, bool bInsert,
	      t_atoms *atoms_solvt,rvec *x_solvt,rvec *v_solvt,real *r_solvt, 
	      bool bVerbose,real rshell);
/* Add two conformations together, without generating overlap.
 * When not inserting, don't check overlap in the middle of the box.
 * If rshell > 0, keep all the residues around the protein (0..natoms_prot-1)
 * that are within rshell distance.
 */
