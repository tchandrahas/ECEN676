/*
 * $Id: pdb2top.c,v 1.71 2002/02/28 10:54:43 spoel Exp $
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
static char *SRCID_pdb2top_c = "$Id: pdb2top.c,v 1.71 2002/02/28 10:54:43 spoel Exp $";
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include "vec.h"
#include "copyrite.h"
#include "assert.h"
#include "smalloc.h"
#include "macros.h"
#include "symtab.h"
#include "futil.h"
#include "fatal.h"
#include "pdb2top.h"
#include "topexcl.h"
#include "topdirs.h"
#include "toputil.h"
#include "h_db.h"
#include "pgutil.h"
#include "resall.h"
#include "topio.h"
#include "string2.h"
#include "physics.h"
#include "pdbio.h"
#include "gen_ad.h"
#include "filenm.h"
#include "index.h"
#include "gen_dum.h"
#include "add_par.h"

/* this must correspond to enum in pdb2top.h */
char *hh[ehisNR]   = { "HISA", "HISB", "HISH", "HIS1" };

static bool missing_atoms(t_restp *rp, int resnr,
			  t_atoms *at, int i0, int i, bool bCTer)
{
  int  j,k;
  char *name;
  bool bFound, bRet;

  bRet=FALSE;
  for (j=0; j<rp->natom; j++) {
    name=*(rp->atomname[j]);
    if ((name[0]!='H') && (name[0]!='h') && (!bCTer || (name[0]!='O'))) {
      bFound=FALSE;
      for (k=i0; k<i; k++) 
	bFound=(bFound || !strcasecmp(*(at->atomname[k]),name));
      if (!bFound) {
	bRet=TRUE;
	fprintf(stderr,"\nWARNING: "
		"atom %s is missing in residue %s %d in the pdb file\n\n",
		name,*(at->resname[resnr]),resnr+1);
      }
    }
  }

  return bRet;
}

bool is_int(double x)
{
  const double tol = 1e-4;
  int   ix;
  
  if (x < 0)
    x=-x;
  ix=gmx_nint(x);
  
  return (fabs(x-ix) < tol);
}

char *choose_ff(void)
{
  typedef struct { char *desc,*fn; } t_fff;
  static  char *fnsel;
  FILE    *in;
  t_fff   *fff;
  int     i,nff,sel;
  char    *c,buf[STRLEN],fn[32];
  
  in=libopen("FF.dat");
  fgets2(buf,255,in);
  sscanf(buf,"%d",&nff);
  snew(fff,nff);
  for(i=0; (i<nff); i++) {
    fgets2(buf,255,in);
    sscanf(buf,"%s",fn);
    fff[i].fn=strdup(fn);
    /* Search for next non-space character, there starts description */
    c=&(buf[strlen(fn)+1]);
    while (isspace(*c)) c++;
    fff[i].desc=strdup(c);
  }
  fclose(in);

  if (nff > 1) {
    printf("\nSelect the Force Field:\n");
    for(i=0; (i<nff); i++)
      printf("%2d: %s\n",i,fff[i].desc);
    do {
      fgets(buf,STRLEN,stdin);
      sscanf(buf,"%d",&sel);
    } while ((sel < 0) || (sel >= nff));
  }
  else
    sel=0;

  fnsel=strdup(fff[sel].fn);

  for(i=0; (i<nff); i++) {
    sfree(fff[i].desc);
    sfree(fff[i].fn);
  }
  sfree(fff);
  
  return fnsel;
}

static void name2type(t_atoms *at, int **cgnr, t_atomtype *atype, 
		      t_restp restp[])
{
  int     i,j,prevresnr,resnr,i0,prevcg,cg,curcg;
  char    *name;
  bool    bProt, bNterm;
  double  qt;
  
  resnr=-1;
  bProt=FALSE;
  bNterm=FALSE;
  i0=0;
  snew(*cgnr,at->nr);
  qt=0;
  prevcg=NOTSET;
  curcg=0;
  cg=-1;
  j=NOTSET;
  for(i=0; (i<at->nr); i++) {
    prevresnr=resnr;
    if (at->atom[i].resnr != resnr) {
      resnr=at->atom[i].resnr;
      bProt=is_protein(*(at->resname[resnr]));
      bNterm=bProt && (resnr == 0);
      if (resnr>0)
	missing_atoms(&restp[prevresnr],resnr,at,i0,i,
		      (!bProt && is_protein(restp[prevresnr].resname)));
      i0=i;
    }
    if (at->atom[i].m == 0) {
      if (debug)
	fprintf(debug,"atom %d%s: curcg=%d, prevcg=%d, cg=%d\n",
		i+1,*(at->atomname[i]),curcg,prevcg,
		j==NOTSET ? NOTSET : restp[resnr].cgnr[j]);
      qt=0;
      prevcg=cg;
      name=*(at->atomname[i]);
      j=search_jtype(&restp[resnr],name,bNterm);
      at->atom[i].type = restp[resnr].atom[j].type;
      at->atom[i].q    = restp[resnr].atom[j].q;
      at->atom[i].m    = atype->atom[restp[resnr].atom[j].type].m;
      cg = restp[resnr].cgnr[j];
      if ( (cg != prevcg) || (resnr != prevresnr) )
	curcg++;
    } else {
      if (debug)
	fprintf(debug,"atom %d%s: curcg=%d, qt=%g, is_int=%d\n",
		i+1,*(at->atomname[i]),curcg,qt,is_int(qt));
      cg=-1;
      if (is_int(qt)) {
	qt=0;
	curcg++;
      }
      qt+=at->atom[i].q;
    }
    (*cgnr)[i]=curcg;
    at->atom[i].typeB = at->atom[i].type;
    at->atom[i].qB    = at->atom[i].q;
    at->atom[i].mB    = at->atom[i].m;
  }
  missing_atoms(&restp[resnr],resnr,at,i0,i,
		(!bProt || is_protein(restp[resnr].resname)));
}

static void print_top_heavy_H(FILE *out, real mHmult)
{
  if (mHmult == 2.0) 
    fprintf(out,"; Using deuterium instead of hydrogen\n\n");
  else if (mHmult == 4.0)
    fprintf(out,"#define HEAVY_H\n\n");
  else if (mHmult != 1.0)
    fprintf(stderr,"WARNING: unsupported proton mass multiplier (%g) "
	    "in pdb2top\n",mHmult);
}

void print_top_comment(FILE *out,char *filename,char *title,bool bITP)
{
  nice_header(out,filename);
  fprintf(out,";\tThis is your %stopology file\n",bITP ? "include " : "");
  fprintf(out,";\t%s\n",title[0]?title:cool_quote());
  fprintf(out,";\n");
}

void print_top_header(FILE *out,char *filename, 
		      char *title,bool bITP,char *ff,real mHmult)
{
  print_top_comment(out,filename,title,bITP);

  print_top_heavy_H(out, mHmult);
  fprintf(out,"; Include forcefield parameters\n");
  fprintf(out,"#include \"%s.itp\"\n\n",ff);
}

static void print_top_posre(FILE *out,char *pr)
{
  fprintf(out,"; Include Position restraint file\n");
  fprintf(out,"#ifdef POSRES\n");
  fprintf(out,"#include \"%s\"\n",pr);
  fprintf(out,"#endif\n\n");
}
  
static void print_top_water(FILE *out)
{
  fprintf(out,"; Include water topology\n");
  fprintf(out,"#ifdef FLEX_SPC\n");
  fprintf(out,"#include \"flexspc.itp\"\n");
  fprintf(out,"#else\n");
  fprintf(out,"#include \"spc.itp\"\n");
  fprintf(out,"#endif\n");
  fprintf(out,"\n");
  fprintf(out,"#ifdef POSRES_WATER\n");
  fprintf(out,"; Position restraint for each water oxygen\n");
  fprintf(out,"[ position_restraints ]\n");
  fprintf(out,";%3s %5s %9s %10s %10s\n","i","funct","fcx","fcy","fcz");
  fprintf(out,"%4d %4d %10g %10g %10g\n",1,1,1000.0,1000.0,1000.0);
  fprintf(out,"#endif\n");
  fprintf(out,"\n");
}

static void print_top_system(FILE *out, char *title)
{
  fprintf(out,"[ %s ]\n",dir2str(d_system));
  fprintf(out,"; Name\n");
  fprintf(out,"%s\n\n",title[0]?title:"Protein");
}

void print_top_mols(FILE *out, char *title, 
		    int nincl, char **incls, int nmol, t_mols *mols)
{
  int i;
  
  if (nincl>0) {
    fprintf(out,"; Include chain topologies\n");
    for (i=0; (i<nincl); i++)
      fprintf(out,"#include \"%s\"\n",incls[i]);
    fprintf(out,"\n");
  }

  print_top_water(out);
  print_top_system(out, title);
  
  if (nmol) {
    fprintf(out,"[ %s ]\n",dir2str(d_molecules));
    fprintf(out,"; %-15s %5s\n","Compound","#mols");
    for (i=0; (i<nmol); i++)
      fprintf(out,"%-15s %5d\n",mols[i].name,mols[i].nr);
  }
}

void write_top(FILE *out, char *pr,char *molname,
	       t_atoms *at,int bts[],t_params plist[],t_excls excls[],
	       t_atomtype *atype,int *cgnr, int nrexcl)
     /* NOTE: nrexcl is not the size of *excl! */
{
  if (at && atype && cgnr) {
    fprintf(out,"[ %s ]\n",dir2str(d_moleculetype));
    fprintf(out,"; %-15s %5s\n","Name","nrexcl");
    fprintf(out,"%-15s %5d\n\n",molname?molname:"Protein",nrexcl);
    
    print_atoms(out, atype, at, cgnr);
    print_bondeds(out,at->nr,d_bonds,      F_BONDS,    bts[ebtsBONDS], plist);
    print_bondeds(out,at->nr,d_constraints,F_SHAKE,    0,              plist);
    print_bondeds(out,at->nr,d_constraints,F_SHAKENC,  0,              plist);
    print_bondeds(out,at->nr,d_pairs,      F_LJ14,     0,              plist);
    print_excl(out,at->nr,excls);
    print_bondeds(out,at->nr,d_angles,     F_ANGLES,   bts[ebtsANGLES],plist);
    print_bondeds(out,at->nr,d_dihedrals,  F_PDIHS,    bts[ebtsPDIHS], plist);
    print_bondeds(out,at->nr,d_dihedrals,  F_IDIHS,    bts[ebtsIDIHS], plist);
    print_bondeds(out,at->nr,d_dum3,       F_DUMMY3,   0,              plist);
    print_bondeds(out,at->nr,d_dum3,       F_DUMMY3FD, 0,              plist);
    print_bondeds(out,at->nr,d_dum3,       F_DUMMY3FAD,0,              plist);
    print_bondeds(out,at->nr,d_dum3,       F_DUMMY3OUT,0,              plist);
    print_bondeds(out,at->nr,d_dum4,       F_DUMMY4FD, 0,              plist);
    
    if (pr)
      print_top_posre(out,pr);
  }
}

static atom_id search_res_atom(char *type,int resnr,
			       int natom,t_atom at[],char **aname[])
{
  int i;

  for(i=0; (i<natom); i++)
    if (at[i].resnr == resnr)
      return search_atom(type,i,natom,at,aname);
  
  return NO_ATID;
}

static void do_ssbonds(t_params *ps,int natoms,t_atom atom[],char **aname[],
		       int nssbonds,t_ssbond *ssbonds)
{
  int     i,ri,rj;
  atom_id ai,aj;
  
  for(i=0; (i<nssbonds); i++) {
    ri = ssbonds[i].res1;
    rj = ssbonds[i].res2;
    ai = search_res_atom(ssbonds[i].a1,ri,natoms,atom,aname);
    aj = search_res_atom(ssbonds[i].a2,rj,natoms,atom,aname);
    if ((ai == NO_ATID) || (aj == NO_ATID))
      fatal_error(0,"Trying to make impossible special bond (%s-%s)!",
		  ssbonds[i].a1,ssbonds[i].a2);
    add_param(ps,ai,aj,NULL,NULL);
  }
}

static void at2bonds(t_params *psb, t_hackblock *hb,
		     int natoms, t_atom atom[], char **aname[], 
		     int nres, rvec x[], 
		     real long_bond_dist, real short_bond_dist)
{
  int     resnr,i,j,k;
  atom_id ai,aj;
  real    dist2, long_bond_dist2, short_bond_dist2;
  
  long_bond_dist2  = sqr(long_bond_dist);
  short_bond_dist2 = sqr(short_bond_dist);
  
  fprintf(stderr,"Making bonds...\n");
  i=0;
  for(resnr=0; (resnr < nres) && (i<natoms); resnr++) {
    /* add bonds from list of bonded interactions */
    for(j=0; j < hb[resnr].rb[ebtsBONDS].nb; j++) {
      ai=search_atom(hb[resnr].rb[ebtsBONDS].b[j].AI,i,natoms,atom,aname);
      aj=search_atom(hb[resnr].rb[ebtsBONDS].b[j].AJ,i,natoms,atom,aname);
      if ( ai != NO_ATID && aj != NO_ATID ) {
	dist2 = distance2(x[ai],x[aj]);
	if (dist2 > long_bond_dist2 )
	  fprintf(stderr,"Warning: Long Bond (%d-%d = %g nm)\n",
		  ai+1,aj+1,sqrt(dist2));
	else if (dist2 < short_bond_dist2 )
	  fprintf(stderr,"Warning: Short Bond (%d-%d = %g nm)\n",
		  ai+1,aj+1,sqrt(dist2));
	add_param(psb,ai,aj,NULL,hb[resnr].rb[ebtsBONDS].b[j].s);
      }
    }
    /* add bonds from list of hacks (each added atom gets a bond) */
    while( (i<natoms) && (atom[i].resnr == resnr) ) {
      for(j=0; j < hb[resnr].nhack; j++)
	if ( ( hb[resnr].hack[j].tp > 0 ||
	       hb[resnr].hack[j].oname==NULL ) &&
	     strcmp(hb[resnr].hack[j].AI,*(aname[i])) == 0 ) {
	  switch(hb[resnr].hack[j].tp) {
	  case 9:          /* COOH terminus */
	    add_param(psb,i,i+1,NULL,NULL);     /* C-O  */
	    add_param(psb,i,i+2,NULL,NULL);     /* C-OA */
	    add_param(psb,i+2,i+3,NULL,NULL);   /* OA-H */
	    break;
	  default:
	    for(k=0; (k<hb[resnr].hack[j].nr); k++)
	      add_param(psb,i,i+k+1,NULL,NULL);
	  }
	}
      i++;
    }
    /* we're now at the start of the next residue */
  }
}

static int pcompar(const void *a, const void *b)
{
  t_param *pa,*pb;
  int     d;
  pa=(t_param *)a;
  pb=(t_param *)b;
  
  d = pa->AI - pb->AI;
  if (d == 0) 
    d = pa->AJ - pb->AJ;
  if (d == 0)
    return strlen(pb->s) - strlen(pa->s);
  else
    return d;
}

static void clean_bonds(t_params *ps)
{
  int     i,j;
  atom_id a;
  
  if (ps->nr > 0) {
    /* swap atomnumbers in bond if first larger than second: */
    for(i=0; (i<ps->nr); i++)
      if ( ps->param[i].AJ < ps->param[i].AI ) {
	a = ps->param[i].AI;
	ps->param[i].AI = ps->param[i].AJ;
	ps->param[i].AJ = a;
      }
    
    /* Sort bonds */
    qsort(ps->param,ps->nr,(size_t)sizeof(ps->param[0]),pcompar);
    
    /* remove doubles, keep the first one always. */
    j = 1;
    for(i=1; (i<ps->nr); i++) {
      if ((ps->param[i].AI != ps->param[j-1].AI) ||
	  (ps->param[i].AJ != ps->param[j-1].AJ) ) {
	cp_param(&(ps->param[j]),&(ps->param[i]));
	j++;
      } 
    }
    fprintf(stderr,"Number of bonds was %d, now %d\n",ps->nr,j);
    ps->nr=j;
  }
  else
    fprintf(stderr,"No bonds\n");
}

void print_sums(t_atoms *atoms, bool bSystem)
{
  double m,qtot;
  int    i;
  char   *where;
  
  if (bSystem)
    where=" in system";
  else
    where="";
  
  m=0;
  qtot=0;
  for(i=0; (i<atoms->nr); i++) {
    m+=atoms->atom[i].m;
    qtot+=atoms->atom[i].q;
  }
  
  fprintf(stderr,"Total mass%s %.3f a.m.u.\n",where,m);
  fprintf(stderr,"Total charge%s %.3f e\n",where,qtot);
}

void get_hackblocks_rtp(t_hackblock **hb, t_restp **restp, 
			int nrtp, t_restp rtp[], int nres, char **resname[], 
			int nterpairs, t_hackblock **ntdb, t_hackblock **ctdb,
			int *rn, int *rc)
{
  int i, j, k, l;
  t_restp *res;
  char buf[STRLEN];
  char Hnum[6]="123456";
  bool bN,bC;

  snew(*hb,nres);
  snew(*restp,nres);
  /* first the termini */
  for(i=0; i<nterpairs; i++) {
    if (rn[i]>=0)
      copy_t_hackblock(ntdb[i], &(*hb)[rn[i]]);
    if (rc[i]>=0)
      merge_t_hackblock(ctdb[i], &(*hb)[rc[i]]);
  }  

  /* then the whole rtp */
  for(i=0; i < nres; i++) {
    res = search_rtp(*resname[i],nrtp,rtp);
    copy_t_restp(res, &(*restp)[i]);
    bN = FALSE;
    for(j=0; j<nterpairs && !bN; j++)
      bN = i==rn[j];
    bC = FALSE;
    for(j=0; j<nterpairs && !bC; j++)
      bC = i==rc[j];
    merge_t_bondeds(res->rb, (*hb)[i].rb,bN,bC);
  }
  
  /* now perform t_hack's on t_restp's,
     i.e. add's and deletes from termini database will be 
     added to/removed from residue topology 
     we'll do this on one big dirty loop, so it won't make easy reading! */
  for(i=0; i < nres; i++)
    for(j=0; j < (*hb)[i].nhack; j++)
      if ( (*hb)[i].hack[j].nr ) {
	/* find atom in restp */
	for(l=0; l < (*restp)[i].natom; l++)
	  if ( ( (*hb)[i].hack[j].oname==NULL && 
		 strcmp((*hb)[i].hack[j].AI, *(*restp)[i].atomname[l])==0 ) ||
	       ( (*hb)[i].hack[j].oname!=NULL &&
		 strcmp((*hb)[i].hack[j].oname,*(*restp)[i].atomname[l])==0 ) )
	    break;
	if (l == (*restp)[i].natom) 
	  fatal_error(0,"atom %s not found in residue %d%s "
		      "while combining tdb and rtp",
		      (*hb)[i].hack[j].oname!=NULL ? 
		      (*hb)[i].hack[j].oname : (*hb)[i].hack[j].AI, 
		      i+1,*resname[i]);
	if ( (*hb)[i].hack[j].oname==NULL ) {
	  /* we're adding: */
	  if (debug) 
	    fprintf(debug,"adding atom(s) %s to atom %s in res %d%s in rtp\n",
		    (*hb)[i].hack[j].nname,
		    *(*restp)[i].atomname[l], i+1, (*restp)[i].resname);
	  strcpy(buf, (*hb)[i].hack[j].nname);
	  buf[strlen(buf)+1]='\0';
	  if ( (*hb)[i].hack[j].nr > 1 )
	    buf[strlen(buf)]='-';
	  /* make space */
	  (*restp)[i].natom += (*hb)[i].hack[j].nr;
	  srenew((*restp)[i].atom,     (*restp)[i].natom);
	  srenew((*restp)[i].atomname, (*restp)[i].natom);
	  srenew((*restp)[i].cgnr,     (*restp)[i].natom);
	  /* shift the rest */
	  for(k=(*restp)[i].natom-1; k > l+(*hb)[i].hack[j].nr; k--) {
	    (*restp)[i].atom[k] =
	      (*restp)[i].atom    [k - (*hb)[i].hack[j].nr];
	    (*restp)[i].atomname[k] =
	      (*restp)[i].atomname[k - (*hb)[i].hack[j].nr];
	    (*restp)[i].cgnr[k] =
	      (*restp)[i].cgnr    [k - (*hb)[i].hack[j].nr];
	  }
	  /* now add them */
	  for(k=0; k < (*hb)[i].hack[j].nr; k++) {
	    /* set counter in atomname */
	    if ( (*hb)[i].hack[j].nr > 1 )
	      buf[strlen(buf)-1]=Hnum[k];
	    snew( (*restp)[i].atomname[l+1+k], 1);
	    (*restp)[i].atom     [l+1+k] = *(*hb)[i].hack[j].atom;
	    *(*restp)[i].atomname[l+1+k] = strdup(buf);
	    if ( (*hb)[i].hack[j].cgnr != NOTSET )
	      (*restp)[i].cgnr   [l+1+k] = (*hb)[i].hack[j].cgnr;
	    else
	      (*restp)[i].cgnr   [l+1+k] = (*restp)[i].cgnr[l];
	  }
	} else /* oname != NULL */
	  if ( (*hb)[i].hack[j].nname==NULL ) {
	    /* we're deleting */
	    if (debug) 
	      fprintf(debug, "deleting atom %s from res %d%s in rtp\n",
			       *(*restp)[i].atomname[l], 
			       i+1,(*restp)[i].resname);
	    /* shift the rest */
	    (*restp)[i].natom--;
	    for(k=l; k < (*restp)[i].natom; k++) {
	      (*restp)[i].atom    [k] = (*restp)[i].atom    [k+1];
	      (*restp)[i].atomname[k] = (*restp)[i].atomname[k+1];
	      (*restp)[i].cgnr    [k] = (*restp)[i].cgnr    [k+1];
	    }
	    /* give back space */
	    srenew((*restp)[i].atom,     (*restp)[i].natom);
	    srenew((*restp)[i].atomname, (*restp)[i].natom);
	    srenew((*restp)[i].cgnr,     (*restp)[i].natom);
	  } else { /* nname != NULL */
	    /* we're replacing */
	    if (debug) 
	      fprintf(debug, "replacing atom %s by %s in res %d%s in rtp\n",
		      *(*restp)[i].atomname[l], (*hb)[i].hack[j].nname, 
		      i+1,(*restp)[i].resname);
	    snew( (*restp)[i].atomname[l], 1);
	    (*restp)[i].atom[l]      =       *(*hb)[i].hack[j].atom;
	    *(*restp)[i].atomname[l] = strdup((*hb)[i].hack[j].nname);
	    if ( (*hb)[i].hack[j].cgnr != NOTSET )
	      (*restp)[i].cgnr   [l] = (*hb)[i].hack[j].cgnr;
	  }
      }
}

void pdb2top(FILE *top_file, char *posre_fn, char *molname,
	     t_atoms *atoms, rvec **x, t_atomtype *atype, t_symtab *tab,
	     int bts[], int nrtp, t_restp   rtp[],
	     int nterpairs,t_hackblock **ntdb, t_hackblock **ctdb,
	     int *rn, int *rc, bool bH14, bool bAlldih,
	     bool bDummies, bool bDummyAromatics, real mHmult,
	     int nssbonds, t_ssbond *ssbonds, int nrexcl, 
	     real long_bond_dist, real short_bond_dist,
	     bool bDeuterate)
{
  t_hackblock *hb;
  t_restp  *restp;
  t_params plist[F_NRE];
  t_excls  *excls;
  t_nextnb nnb;
  int      *cgnr;
  int      *dummy_type;
  int      i;
  
  init_plist(plist);

  /* lookup hackblocks and rtp for all residues */
  get_hackblocks_rtp(&hb, &restp, nrtp, rtp, atoms->nres, atoms->resname, 
		     nterpairs, ntdb, ctdb, rn, rc);
  /* ideally, now we would not need the rtp itself anymore, but do 
     everything using the hb and restp arrays. Unfortunately, that 
     requires some re-thinking of code in gen_dum.c, which I won't 
     do now :( AF 26-7-99 */
  
  if (debug) {
    print_resall(debug, bts, atoms->nres, restp, atype);
    dump_hb(debug, atoms->nres, hb);
  }
  
  /* Make bonds */
  at2bonds(&(plist[F_BONDS]), hb, 
	   atoms->nr, atoms->atom, atoms->atomname, atoms->nres, *x, 
	   long_bond_dist, short_bond_dist);
  
  /* specbonds: disulphide bonds & heme-his */
  do_ssbonds(&(plist[F_BONDS]),
	     atoms->nr, atoms->atom, atoms->atomname, nssbonds, ssbonds);
  
  name2type(atoms, &cgnr, atype, restp);
  
  /* Cleanup bonds (sort and rm doubles) */ 
  clean_bonds(&(plist[F_BONDS]));
  
  snew(dummy_type,atoms->nr);
  for(i=0; i<atoms->nr; i++)
    dummy_type[i]=NOTSET;
  if (bDummies)
    /* determine which atoms will be dummies and add dummy masses 
       also renumber atom numbers in plist[0..F_NRE]! */
    do_dummies(nrtp, rtp, atype, atoms, tab, x, plist, 
	       &dummy_type, &cgnr, mHmult, bDummyAromatics);
  
  /* Make Angles and Dihedrals */
  fprintf(stderr,"Generating angles and dihedrals...\n");
  snew(excls,atoms->nr);
  init_nnb(&nnb,atoms->nr,4);
  gen_nnb(&nnb,plist);
  print_nnb(&nnb,"NNB");
  gen_pad(&nnb,atoms,nrexcl,bH14,plist,excls,hb,bAlldih);
  done_nnb(&nnb);
  
  /* set mass of all remaining hydrogen atoms */
  if (mHmult != 1.0)
    do_h_mass(&(plist[F_BONDS]),dummy_type,atoms,mHmult,bDeuterate);
  sfree(dummy_type);
  
  /* Cleanup bonds (sort and rm doubles) */ 
  /* clean_bonds(&(plist[F_BONDS]));*/
   
  fprintf(stderr,
	  "There are %4d dihedrals, %4d impropers, %4d angles\n"
	  "          %4d pairs,     %4d bonds and  %4d dummies\n",
	  plist[F_PDIHS].nr, plist[F_IDIHS].nr, plist[F_ANGLES].nr,
	  plist[F_LJ14].nr, plist[F_BONDS].nr,
	  plist[F_DUMMY2].nr +
	  plist[F_DUMMY3].nr +
	  plist[F_DUMMY3FD].nr +
	  plist[F_DUMMY3FAD].nr +
	  plist[F_DUMMY3OUT].nr +
	  plist[F_DUMMY4FD].nr );
  
  print_sums(atoms, FALSE);
  
  if (top_file) {
    fprintf(stderr,"Writing topology\n");
    write_top(top_file, posre_fn, molname,
	      atoms, bts, plist, excls, atype, cgnr, nrexcl);
  }
  
  /* cleaning up */
  free_t_hackblock(atoms->nres, &hb);
  free_t_restp(atoms->nres, &restp);
    
  /* we should clean up hb and restp here, but that is a *L*O*T* of work! */
  sfree(cgnr);
  for (i=0; i<F_NRE; i++)
    sfree(plist[i].param);
  for (i=0; i<atoms->nr; i++)
    sfree(excls[i].e);
  sfree(excls);
}
