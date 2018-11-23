/*
 * $Id: readir.h,v 1.14 2002/02/28 10:54:44 spoel Exp $
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

#ifndef _readir_h
#define _readir_h

static char *SRCID_readir_h = "$Id: readir.h,v 1.14 2002/02/28 10:54:44 spoel Exp $";
#ifdef HAVE_IDENT
#ident	"@(#) readir.h 1.20 9/30/97"
#endif /* HAVE_IDENT */

#include "typedefs.h"
#include "string2.h"
#include "grompp.h"

enum { eshNONE, eshHBONDS, eshALLBONDS, eshHANGLES, eshALLANGLES, eshNR };

static char *constraints[eshNR+1]    = { 
    "none", "h-bonds", "all-bonds", "h-angles", "all-angles", NULL 
  };

typedef struct {
  int warnings;
  int nshake;
  real fourierspacing;
  int nprocs;
  int splitalg;
  char *title;
  char *cpp;
  char *include;
  char *define;
  char *SolventOpt;
  bool bGenVel;
  bool bGenPairs;
  real tempi;
  int  seed;
  int  eDisre;
  bool bOrire;
  bool bMorse;
} t_gromppopts;


extern void init_ir(t_inputrec *ir, t_gromppopts *opts);
/* Initiate stuff */

extern void check_ir(t_inputrec *ir, t_gromppopts *opts,int *nerror);
/* Validate inputrec data.
 * Fatal errors will be added to nerror.
 */
 
extern void double_check(t_inputrec *ir,matrix box,t_molinfo *mol,int *nerror);
/* Do more checks */

extern void triple_check(char *mdparin,t_inputrec *ir,t_topology *sys,
			 int *nerror);
/* Do even more checks */

extern void get_ir(char *mdparin,char *mdparout,
		   t_inputrec *ir,t_gromppopts *opts,int *nerror);
/* Read the input file, and retrieve data for inputrec.
 * More data are read, but the are only evaluated when the next
 * function is called. Also prints the input file back to mdparout.
 * Add errors no nerror.
 */
 
extern void do_index(char *ndx,
		     t_symtab   *symtab,
		     t_atoms    *atoms,bool bVerbose,
		     t_inputrec *ir,t_idef *idef,int *forward,
		     rvec *v);
/* Read the index file and assign grp numbers to atoms.
 * If v is not NULL, the velocities will be scaled to the correct number
 * of degrees of freedom.
 */

#endif	/* _readir_h */
