/*
 * $Id: pgutil.c,v 1.12 2002/02/28 10:54:44 spoel Exp $
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
static char *SRCID_pgutil_c = "$Id: pgutil.c,v 1.12 2002/02/28 10:54:44 spoel Exp $";
#include "pgutil.h"
#include "string.h"
	
atom_id search_atom(char *type,int start,int natoms,t_atom at[],char **anm[])
{
  int     i,resnr=-1;
  bool    bPrevious,bNext;

  bPrevious = (strchr(type,'-') != NULL);
  bNext     = (strchr(type,'+') != NULL);

  if (!bPrevious) {
    resnr = at[start].resnr;
    if (bNext) {
      /* The next residue */
      type++;
      while ((start<natoms) && (at[start].resnr == resnr))
	start++;
      if (start < natoms)
	resnr = at[start].resnr;
    }
    for(i=start; (i<natoms) && (bNext || (at[i].resnr == resnr)); i++)
      if (strcasecmp(type,*(anm[i]))==0)
	return (atom_id) i;
  }
  else {
    /* The previous residue */
    type++;
    if (start > 0)
      resnr = at[start-1].resnr;
    for(i=start-1; (i>=0) /*&& (at[i].resnr == resnr)*/; i--)
      if (strcasecmp(type,*(anm[i]))==0)
	return (atom_id) i;
  }
  return NO_ATID;
}

void set_at(t_atom *at,real m,real q,int type,int resnr)
{
  at->m=m;
  at->q=q;
  at->type=type;
  at->resnr=resnr;
}

