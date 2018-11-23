/*
 * $Id: renum.h,v 1.7 2002/02/28 21:55:51 spoel Exp $
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
 * Getting the Right Output Means no Artefacts in Calculating Stuff
 */

#ifndef _renum_h
#define _renum_h

static char *SRCID_renum_h = "$Id: renum.h,v 1.7 2002/02/28 21:55:51 spoel Exp $";
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_IDENT
#ident	"@(#) renum.h 1.6 11/23/92"
#endif /* HAVE_IDENT */

#include "typedefs.h"

extern void renum_params(t_topology *top,int renum[]);
     /*
      * The atom id's in the parameters for the bonded forces will be 
      * renumbered according to the order specified in renum. 
      *
      * renum[i]=j specifies that atom i will be at postion j after 
      * renum_params.
      */

extern void renumber_top(t_topology *top,rvec *x,rvec *v,rvec *f,int renum[]);
     /*
      * All atoms in the topology top will be renumbered according to the order
      * specified in renum. The vector array's x,v & f will also be renumbered.
      *
      * renum[i]=j specifies that atom i will be at postion j after 
      * renumber_top.
      */

#endif	/* _renum_h */
