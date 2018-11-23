/*
 * $Id: topdirs.h,v 1.6 2002/02/28 10:54:45 spoel Exp $
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

#ifndef _topdirs_h
#define _topdirs_h

static char *SRCID_topdirs_h = "$Id: topdirs.h,v 1.6 2002/02/28 10:54:45 spoel Exp $";
#ifdef HAVE_IDENT
#ident	"@(#) topdirs.h 1.30 9/30/97"
#endif /* HAVE_IDENT */

#include "grompp.h"

typedef struct tagDirStack {
  directive d;
  struct tagDirStack *prev;
} DirStack;

extern int ifunc_index(directive d,int type);

extern char *dir2str (directive d);

extern directive str2dir (char *dstr);

extern void DS_Init (DirStack **DS);

extern void DS_Done (DirStack **DS);

extern void DS_Push (DirStack **DS, directive d);

extern int  DS_Search (DirStack *DS, directive d);

extern int  DS_Check_Order (DirStack *DS, directive d);

#endif	/* _topdirs_h */
