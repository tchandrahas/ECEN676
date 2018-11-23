/*
 * $Id: nhash.h,v 1.7 2002/02/28 21:55:50 spoel Exp $
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

#ifndef _nhash_h
#define _nhash_h

static char *SRCID_nhash_h = "$Id: nhash.h,v 1.7 2002/02/28 21:55:50 spoel Exp $";
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_IDENT
#ident	"@(#) nhash.h 1.3 11/23/92"
#endif /* HAVE_IDENT */

#define MAX_LJQQ 997

typedef struct {
  float c6,c12,qq;
} t_ljqq;

extern t_ljqq LJQQ[MAX_LJQQ];

int h_enter(FILE *log,float c6, float c12, float qq);
/* Enters the constants denoted above in the hash-table.
 * Returns the index in LJQQ.
 */

void h_stat(FILE *log);
/* Print statistics for hashing */

#endif	/* _nhash_h */
