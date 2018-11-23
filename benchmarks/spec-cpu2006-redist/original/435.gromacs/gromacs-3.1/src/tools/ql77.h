/*
 * $Id: ql77.h,v 1.5 2002/02/28 11:00:29 spoel Exp $
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
 * Green Red Orange Magenta Azure Cyan Skyblue
 */
static char *SRCID_ql77_h = "$Id: ql77.h,v 1.5 2002/02/28 11:00:29 spoel Exp $";
extern void ql77 (int n,real *x,real *d);
/* Determine the eigenvalues d[n] and eigenvectors   *
 * of the symmetric n x n matrix x. The eigenvectors *
 * are stored  in x as x[i+n*v], where v is the      *
 * eigenvector number and i is the component index.  *
 * The eigenvalues are stored in ascending order.    */
