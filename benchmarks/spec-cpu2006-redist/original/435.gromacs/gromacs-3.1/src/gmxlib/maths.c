/*
 * $Id: maths.c,v 1.7 2002/02/28 10:49:24 spoel Exp $
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
 * Gyas ROwers Mature At Cryogenic Speed
 */
static char *SRCID_maths_c = "$Id: maths.c,v 1.7 2002/02/28 10:49:24 spoel Exp $";
#include <math.h>
#include "maths.h"

int gmx_nint(real a)
{   
  const real half = .5;
  int   result;
  
  result = (a < 0.) ? ((int)(a - half)) : ((int)(a + half));
  return result;
}

real sign(real x,real y)
{
  if (y < 0)
    return -fabs(x);
  else
    return +fabs(x);
}
