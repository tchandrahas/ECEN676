/*
 * $Id: topdirs.c,v 1.22 2002/02/28 10:54:45 spoel Exp $
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
static char *SRCID_topdirs_c = "$Id: topdirs.c,v 1.22 2002/02/28 10:54:45 spoel Exp $";
#include <stdio.h>
#include <stdarg.h>

#include "sysstuff.h"
#include "smalloc.h"
#include "macros.h"
#include "string2.h"
#include "topdirs.h"

int ifunc_index(directive d,int type)
{
  switch (d) {
  case d_bondtypes:
  case d_bonds:
    if (type == 1)
      return F_BONDS;
    else if (type == 2)
      return F_G96BONDS;
    else if (type == 3)
      return F_MORSE;
    else if (type == 4)
      return F_CUBICBONDS;
    else if (type == 5)
      return F_CONNBONDS;
    else if (type == 6)
      return F_HARMONIC;
    else
      fatal_error(0,"Invalid bond type %d",type);
  case d_angles:
  case d_angletypes:
    if (type == 1)
      return F_ANGLES;
    else if (type == 2)
      return F_G96ANGLES;
    else
      fatal_error(0,"Invalid angle type %d",type);
  case d_pairs:
  case d_pairtypes:
    return F_LJ14;
  case d_dihedrals:
  case d_dihedraltypes:
    switch (type) {
    case 1:
      return F_PDIHS;
    case 2:
      return F_IDIHS;
    case 3:
      return F_RBDIHS;
    default:
      fatal_error(0,"Invalid dihedral type %d",type);
    }
    break;
  case d_nonbond_params:
    if (type == 1)
      return F_LJ;
    else
      return F_BHAM;
  case d_dum2:
    return F_DUMMY2;
  case d_dum3:
    switch (type) {
    case 1:
      return F_DUMMY3;
    case 2: 
      return F_DUMMY3FD;
    case 3:
      return F_DUMMY3FAD;
    case 4:
      return F_DUMMY3OUT;  
    default:
      fatal_error(0,"Invalid dummies3 type %d",type);
    }
  case d_dum4:
    return F_DUMMY4FD; 
  case d_constraints:
  case d_constrainttypes:
    switch (type) {
    case 1:
      return F_SHAKE;
    case 2:
      return F_SHAKENC;
    default:
      fatal_error(0,"Invalid constraints type %d",type);
    }
  case d_settles:
    return F_SETTLE;
  case d_position_restraints:
    switch (type) {
    case 1: 
      return F_POSRES;
    case 2:
      return F_WPOL;
    default:
      fatal_error(0,"Invalid position restraint type %d",type);
    }
  case d_angle_restraints:
    return F_ANGRES;
  case d_angle_restraints_z:
    return F_ANGRESZ;
  case d_distance_restraints:
    return F_DISRES;
  case d_orientation_restraints:
    return F_ORIRES;
  default:
    fatal_error(0,"invalid directive %s in ifunc_index (%s:%s)",
		dir2str(d),__FILE__,__LINE__);
  }
  return -1;
}
  
char *dir2str (directive d)
{
  if (d < d_maxdir)
    return ds[d];
  else
    return ds[d_maxdir];
}

directive str2dir (char *dstr)
{
  directive d;
  
  for (d=(directive)0; (d<d_maxdir); d++)
    if (strcasecmp_min(dstr,dir2str(d)) == 0)
      return d;

  return d_invalid;
}

static directive **necessary = NULL;

static void set_nec(directive **n, ...)
/* Must always have at least one extra argument */
{
  va_list   ap;
  int       ind=0;
  directive d;

  va_start(ap,n);
  do {
    d=va_arg(ap,directive);
    srenew(*n,++ind);
    (*n)[ind-1]=d;
  } while (d != d_none);
  va_end(ap);
}

void DS_Init(DirStack **DS)
{
  if (necessary==NULL) {
    int i;

    snew(necessary,d_maxdir);
    set_nec(&(necessary[d_defaults]),d_none);
    set_nec(&(necessary[d_atomtypes]),d_defaults,d_none);
    set_nec(&(necessary[d_bondtypes]),d_atomtypes,d_none);
    set_nec(&(necessary[d_constrainttypes]),d_atomtypes,d_none);
    set_nec(&(necessary[d_pairtypes]),d_atomtypes,d_none);
    set_nec(&(necessary[d_angletypes]),d_atomtypes,d_none);
    set_nec(&(necessary[d_dihedraltypes]),d_atomtypes,d_none);
    set_nec(&(necessary[d_nonbond_params]),d_atomtypes,d_none);
    set_nec(&(necessary[d_blocktype]),d_atomtypes,d_none);
    set_nec(&(necessary[d_moleculetype]),d_atomtypes,d_none);
    set_nec(&(necessary[d_atoms]),d_blocktype,d_moleculetype,d_none);
    set_nec(&(necessary[d_dum2]),d_atoms,d_none);
    set_nec(&(necessary[d_dum3]),d_atoms,d_none);
    set_nec(&(necessary[d_dum4]),d_atoms,d_none);
    set_nec(&(necessary[d_bonds]),d_atoms,d_none);
    set_nec(&(necessary[d_exclusions]),d_bonds,d_constraints,d_settles,d_none);
    set_nec(&(necessary[d_pairs]),d_atoms,d_none);
    set_nec(&(necessary[d_angles]),d_atoms,d_none);
    set_nec(&(necessary[d_dihedrals]),d_atoms,d_none);
    set_nec(&(necessary[d_constraints]),d_atoms,d_none);
    set_nec(&(necessary[d_settles]),d_atoms,d_none);
    set_nec(&(necessary[d_system]),d_moleculetype,d_none);
    set_nec(&(necessary[d_molecules]),d_system,d_none);
    set_nec(&(necessary[d_position_restraints]),d_atoms,d_none);
    set_nec(&(necessary[d_angle_restraints]),d_atoms,d_none);
    set_nec(&(necessary[d_angle_restraints_z]),d_atoms,d_none);
    set_nec(&(necessary[d_distance_restraints]),d_atoms,d_none);
    set_nec(&(necessary[d_orientation_restraints]),d_atoms,d_none);
    for(i=0; (i<d_maxdir); i++) {
      if (debug)
	fprintf(debug,"%20s:  ",dir2str((directive)i));
      if(necessary[i]) {
	directive d;
	int       j=0;
	do {
	  d=necessary[i][j++];
	  if (debug)
	    fprintf(debug,"%20s  ",dir2str(d));
	} while (d!=d_none);
      }
      if (debug)
	fprintf(debug,"\n");
    }
  } 
  *DS = NULL;

}

void DS_Done (DirStack **DS)
{
  DirStack *D;

  while (*DS != NULL) {
    D = *DS;
    *DS = (*DS)->prev;
    sfree (D);
  }
}

void DS_Push (DirStack **DS, directive d)
{
  DirStack *D;

  snew(D,1);
  D->d = d;
  D->prev = *DS;
  *DS = D;
}

int DS_Search(DirStack *DS, directive d)
{
  DirStack *D;
  
  D = DS;
  while ((D != NULL) && (D->d != d))
    D = D->prev;

  return (D != NULL);
}

int DS_Check_Order(DirStack *DS,directive d)
{
  directive d0;
  int       i=0;

  if (necessary[d][0] == d_none)
    return TRUE;
  else {
    do {
      d0=necessary[d][i++];
      if (DS_Search(DS,d0))
	return TRUE;
    } while(d0!=d_none);
  }
  return FALSE;
}



