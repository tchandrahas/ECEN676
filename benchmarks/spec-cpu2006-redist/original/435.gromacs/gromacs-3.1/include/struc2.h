/*
 * $Id: struc2.h,v 1.7 2002/02/28 21:55:51 spoel Exp $
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

#ifndef _struc2_h
#define _struc2_h

static char *SRCID_struc2_h = "$Id: struc2.h,v 1.7 2002/02/28 21:55:51 spoel Exp $";
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_IDENT
#ident	"@(#) struc2.h 1.2 15 Sep 1993"
#endif /* HAVE_IDENT */
#include "typedefs.h"

typedef struct {
  atom_id d;
  atom_id a;
  atom_id h;
} t_dah;

typedef struct {
  real  a_dist; 
  real  h_dist;
  real  d_dist;
  int   nr;	
} t_water;		
	
	
typedef struct {
  real    distance_ad;
  real    distance_ah;
  real    angle;
  bool    h_bond;
  t_water water;
} t_info;

typedef struct {
  t_info *info;
  atom_id  a;
  atom_id  d;
  atom_id  h;
  int    monitor;
} t_hbond;

typedef struct {
  int     nrt;
  int     nr;
  t_hbond *hbond;
  real    *time;
} t_list;

extern void determine_2struc(FILE *out,t_topology *top,t_list *list);

#endif	/* _struc2_h */
