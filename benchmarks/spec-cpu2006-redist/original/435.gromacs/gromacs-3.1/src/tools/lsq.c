/*
 * $Id: lsq.c,v 1.4 2002/02/28 11:00:28 spoel Exp $
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
static char *SRCID_lsq_c = "$Id: lsq.c,v 1.4 2002/02/28 11:00:28 spoel Exp $";
#include "typedefs.h"
#include "gstat.h"
#include "vec.h"

void init_lsq(t_lsq *lsq)
{
  lsq->yy=lsq->yx=lsq->xx=lsq->sx=lsq->sy=0.0;
  lsq->np=0;
}

int npoints_lsq(t_lsq *lsq)
{
  return lsq->np;
}

void done_lsq(t_lsq *lsq)
{
  init_lsq(lsq);
}

void add_lsq(t_lsq *lsq,real x,real y)
{
  lsq->yy+=y*y;
  lsq->yx+=y*x;
  lsq->xx+=x*x;
  lsq->sx+=x;
  lsq->sy+=y;
  lsq->np++;
}

void get_lsq_ab(t_lsq *lsq,real *a,real *b)
{
  double yx,xx,sx,sy;
  
  yx=lsq->yx/lsq->np;
  xx=lsq->xx/lsq->np;
  sx=lsq->sx/lsq->np;
  sy=lsq->sy/lsq->np;
  
  (*a)=(yx-sx*sy)/(xx-sx*sx);
  (*b)=(sy)-(*a)*(sx);
}

real aver_lsq(t_lsq *lsq)
{
  if (lsq->np == 0)
    fatal_error(0,"No points in distribution\n");
  
  return (lsq->sy/lsq->np);
}

real sigma_lsq(t_lsq *lsq)
{
  if (lsq->np == 0)
    fatal_error(0,"No points in distribution\n");
    
  return sqrt(lsq->yy/lsq->np - sqr(lsq->sy/lsq->np));
}

real error_lsq(t_lsq *lsq)
{
  return sigma_lsq(lsq)/sqrt(lsq->np);
}

