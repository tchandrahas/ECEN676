/*
 * $Id: edittop.c,v 1.7 2002/02/28 11:00:24 spoel Exp $
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
static char *SRCID_edittop_c = "$Id: edittop.c,v 1.7 2002/02/28 11:00:24 spoel Exp $";
#include "smalloc.h"
#include "string2.h"
#include "fatal.h"
#include "assert.h"
#include "symtab.h"

void replace_atom(t_topology *top,int inr,char *anm,char *resnm,
		  real q,real m,int type)
{
  t_atoms *atoms;

  atoms = &(top->atoms);
  
  /* Replace important properties of an atom by other properties */  
  if ((inr < 0) || (inr > atoms->nr))
    fatal_error(0,"Replace_atom: inr (%d) not in %d .. %d",inr,0,atoms->nr);
  if (debug)
    fprintf(debug,"Replacing atom %d ... ",inr);
  /* Charge, mass and type */
  atoms->atom[inr].q    = atoms->atom[inr].qB    = q;
  atoms->atom[inr].m    = atoms->atom[inr].mB    = m;
  atoms->atom[inr].type = atoms->atom[inr].typeB = type;
  
  /* Residue name */
  atoms->resname[atoms->atom[inr].resnr] = put_symtab(&top->symtab,resnm);
  /* Atom name */
  atoms->atomname[inr] = put_symtab(&top->symtab,anm);
  if (debug)
    fprintf(debug," done\n");
}

static void delete_from_interactions(t_idef *idef,int inr)
{
  int  i,j,k,nra,nnr;
  t_iatom *niatoms;
  bool bDel;
  
  /* Delete interactions including atom inr from lists */
  for(i=0; (i<F_NRE); i++) {
    nra = interaction_function[i].nratoms;
    nnr = 0;
    snew(niatoms,idef->il[i].nr);
    for(j=0; (j<idef->il[i].nr); j+=nra+1) {
      bDel = FALSE;
      for(k=0; (k<nra); k++)
	if (idef->il[i].iatoms[j+k+1] == inr)
	  bDel = TRUE;
      if (!bDel) {
	/* If this does not need to be deleted, then copy it to temp array */
	for(k=0; (k<nra+1); k++)
	  niatoms[nnr+k] = idef->il[i].iatoms[j+k];
	nnr+=nra+1;
      }
    }
    /* Copy temp array back */
    for(j=0; (j<nnr); j++)
      idef->il[i].iatoms[j] = niatoms[j];
    idef->il[i].nr = nnr;
    sfree(niatoms);
    
    /* Reduce multinr if necessary */
    for(j=0; (j<MAXNODES); j++)
      if (idef->il[i].multinr[j] >= nnr)
	idef->il[i].multinr[j] = nnr;
  }
}

static void delete_from_block(t_block *block,int inr) 
{
  /* Update block data structure */
  int i,i1,j1,j,k;
  
  for(i=0; (i<block->nr); i++) {
    for(j=block->index[i]; (j<block->index[i+1]); j++) {
      k = block->a[j];
      if (k == inr) {
	/* This atom has to go */
	for(j1=j; (j1<block->nra-1); j1++)
	  block->a[j1] = block->a[j1+1];
	block->nra--;
	/* Change indices too */
	for(i1=i+1; (i1<=block->nr); i1++)
	  block->index[i1]--;
      }
    }
  }
}

static void delete_from_atoms(t_atoms *atoms,int inr)
{
  int i,nrei,ind0;
  
  /* Delete inr as an exclusion from other atoms */
  delete_from_block(&(atoms->excl),inr);
  /* Now delete the exclusions with inr as i atom */
  ind0 = atoms->excl.index[inr];
  nrei = atoms->excl.index[inr+1]-ind0;
  for(i=ind0+nrei; (i<atoms->excl.nra); i++)
    atoms->excl.a[i-nrei] = atoms->excl.a[i];
  atoms->excl.nra -= nrei;
  for(i=inr; (i<atoms->excl.nr); i++)
    atoms->excl.index[i] = atoms->excl.index[i+1] - nrei;
  atoms->excl.nr--;
  assert(atoms->excl.index[atoms->excl.nr] == atoms->excl.nra);
  
  /* Shift the atomnames down */
  for(i=inr; (i<atoms->nr-1); i++)
    atoms->atomname[i] = atoms->atomname[i+1];
  
  /* Shift the atom struct down */
  for(i=inr; (i<atoms->nr-1); i++)
    atoms->atom[i] = atoms->atom[i+1];
    
  if (atoms->pdbinfo) {
    /* Shift the pdbatom struct down */
    for(i=inr; (i<atoms->nr-1); i++)
      atoms->pdbinfo[i] = atoms->pdbinfo[i+1];
  }
  atoms->nr--;
}

void delete_atom(t_topology *top,int inr)
{
  int k;
  
  if ((inr < 0) || (inr >= top->atoms.nr))
    fatal_error(0,"Delete_atom: inr (%d) not in %d .. %d",inr,0,
		top->atoms.nr);
  if (debug)
    fprintf(debug,"Deleting atom %d ...",inr);

  /* First remove bonds etc. */
  delete_from_interactions(&top->idef,inr);
  /* Now charge groups etc. */
  for(k=0; (k<ebNR); k++)
    delete_from_block(&(top->blocks[k]),inr);
  /* Now from the atoms struct */
  delete_from_atoms(&top->atoms,inr);
  if (debug)
    fprintf(debug," done\n");
}
