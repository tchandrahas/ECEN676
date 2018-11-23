/*
 * $Id: gen_ad.c,v 1.37 2002/02/28 10:54:41 spoel Exp $
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
static char *SRCID_gen_ad_c = "$Id: gen_ad.c,v 1.37 2002/02/28 10:54:41 spoel Exp $";
#include <math.h>
#include <ctype.h>
#include "sysstuff.h"
#include "macros.h"
#include "smalloc.h"
#include "string2.h"
#include "confio.h"
#include "vec.h"
#include "pbc.h"
#include "toputil.h"
#include "topio.h"
#include "topexcl.h"
#include "symtab.h"
#include "macros.h"
#include "fatal.h"
#include "pgutil.h"
#include "resall.h"

typedef bool (*peq)(t_param *p1, t_param *p2);

static int acomp(const void *a1, const void *a2)
{
  t_param *p1,*p2;
  int     ac;
  
  p1=(t_param *)a1;
  p2=(t_param *)a2;
  if ((ac=(p1->AJ-p2->AJ))!=0)
    return ac;
  else if ((ac=(p1->AI-p2->AI))!=0)
    return ac;
  else 
    return (p1->AK-p2->AK);
}

static int pcomp(const void *a1, const void *a2)
{
  t_param *p1,*p2;
  int     pc;
  
  p1=(t_param *)a1;
  p2=(t_param *)a2;
  if ((pc=(p1->AI-p2->AI))!=0)
    return pc;
  else 
    return (p1->AJ-p2->AJ);
}

static int dcomp(const void *d1, const void *d2)
{
  t_param *p1,*p2;
  int     dc;
  
  p1=(t_param *)d1;
  p2=(t_param *)d2;
  if ((dc=(p1->AJ-p2->AJ))!=0)
    return dc;
  else if ((dc=(p1->AK-p2->AK))!=0)
    return dc;
  else if ((dc=(p1->AI-p2->AI))!=0)
    return dc;
  else
    return (p1->AL-p2->AL);
}

static bool aeq(t_param *p1, t_param *p2)
{
  if (p1->AJ!=p2->AJ) 
    return FALSE;
  else if (((p1->AI==p2->AI) && (p1->AK==p2->AK)) ||
	   ((p1->AI==p2->AK) && (p1->AK==p2->AI)))
    return TRUE;
  else 
    return FALSE;
}

static bool deq2(t_param *p1, t_param *p2)
{
  /* if bAlldih is true, dihedrals are only equal when 
     ijkl = ijkl or ijkl =lkji*/
  if (((p1->AI==p2->AI) && (p1->AJ==p2->AJ) && 
       (p1->AK==p2->AK) && (p1->AL==p2->AL)) ||
      ((p1->AI==p2->AL) && (p1->AJ==p2->AK) &&
       (p1->AK==p2->AJ) && (p1->AL==p2->AI)))
    return TRUE;
  else 
    return FALSE;
}

static bool deq(t_param *p1, t_param *p2)
{
  if (((p1->AJ==p2->AJ) && (p1->AK==p2->AK)) ||
      ((p1->AJ==p2->AK) && (p1->AK==p2->AJ)))
    return TRUE;
  else 
    return FALSE;
}

static bool remove_dih(t_param *p, int i, int np)
     /* check if dihedral p[i] should be removed */
{
  bool bMidEq,bRem;
  int j;

  if (i>0)
    bMidEq = deq(&p[i],&p[i-1]);
  else
    bMidEq = FALSE;

  if (p[i].c[MAXFORCEPARAM-1]==NOTSET) {
    /* also remove p[i] if there is a dihedral on the same bond
       which has parameters set */
    bRem = bMidEq;
    j=i+1;
    while (!bRem && (j<np) && deq(&p[i],&p[j])) {
      bRem = (p[j].c[MAXFORCEPARAM-1] != NOTSET);
      j++;
    }
  } else
    bRem = bMidEq && (((p[i].AI==p[i-1].AI) && (p[i].AL==p[i-1].AL)) ||
		      ((p[i].AI==p[i-1].AL) && (p[i].AL==p[i-1].AI)));

  return bRem;
}

static bool preq(t_param *p1, t_param *p2)
{
  if ((p1->AI==p2->AI) && (p1->AJ==p2->AJ))
    return TRUE;
  else 
    return FALSE;
}

static void rm2par(t_param p[], int *np, peq eq)
{
  int *index,nind;
  int i,j;

  if ((*np)==0)
    return;

  snew(index,*np);
  nind=0;
    index[nind++]=0;
  for(i=1; (i<(*np)); i++) 
    if (!eq(&p[i],&p[i-1]))
      index[nind++]=i;
  /* Index now holds pointers to all the non-equal params,
   * this only works when p is sorted of course
   */
  for(i=0; (i<nind); i++) {
    for(j=0; (j<MAXATOMLIST); j++)
      p[i].a[j]=p[index[i]].a[j];
    for(j=0; (j<MAXFORCEPARAM); j++)
      p[i].c[j]=p[index[i]].c[j];
    if (p[index[i]].a[0] == p[index[i]].a[1]) {
      if (debug)  
	fprintf(debug,
		"Something VERY strange is going on in rm2par (gen_ad.c)\n"
		"a[0] %u a[1] %u a[2] %u a[3] %u\n",
		p[i].a[0],p[i].a[1],p[i].a[2],p[i].a[3]);
      strcpy(p[i].s,"");
    } else if (index[i] > i) {
      /* Copy the string only if it comes from somewhere else 
       * otherwise we will end up copying a random (newly freed) pointer.
       * Since the index is sorted we only have to test for index[i] > i.
       */ 
      strcpy(p[i].s,p[index[i]].s);
    }
  }
  (*np)=nind;

  sfree(index);
}

static void cppar(t_param p[], int np, t_params plist[], int ftype)
{
  int      i,j,nral,nrfp;
  t_params *ps;

  ps   = &plist[ftype];
  nral = NRAL(ftype);
  nrfp = NRFP(ftype);
  
  /* Keep old stuff */
  pr_alloc(np,ps);
  for(i=0; (i<np); i++) {
    for(j=0; (j<nral); j++)
      ps->param[ps->nr].a[j] = p[i].a[j];
    for(j=0; (j<nrfp); j++)
      ps->param[ps->nr].c[j] = p[i].c[j];
    set_p_string(&(ps->param[ps->nr]),p[i].s);
    ps->nr++;
  }
}

static void cpparam(t_param *dest, t_param *src)
{
  int j;

  for(j=0; (j<MAXATOMLIST); j++)
    dest->a[j]=src->a[j];
  for(j=0; (j<MAXFORCEPARAM); j++)
    dest->c[j]=src->c[j];
  strcpy(dest->s,src->s);
}

static void set_p(t_param *p, atom_id ai[4], real *c, char *s)
{
  int j;

  for(j=0; (j<4); j++)
    p->a[j]=ai[j];
  for(j=0; (j<MAXFORCEPARAM); j++)
    if (c)
      p->c[j]=c[j];
    else
      p->c[j]=NOTSET;

  set_p_string(p,s);
}

static int int_comp(const void *a,const void *b)
{
  return (*(int *)a) - (*(int *)b);
}

static int atom_id_comp(const void *a,const void *b)
{
  return (*(atom_id *)a) - (*(atom_id *)b);
}

static int eq_imp(atom_id a1[],atom_id a2[])
{
  int b1[4],b2[4];
  int j;

  for(j=0; (j<4); j++) {
    b1[j]=a1[j];
    b2[j]=a2[j];
  }
  qsort(b1,4,(size_t)sizeof(b1[0]),int_comp);
  qsort(b2,4,(size_t)sizeof(b2[0]),int_comp);

  for(j=0; (j<4); j++)
    if (b1[j] != b2[j])
      return FALSE;

  return TRUE;
}

static bool ideq(t_param *p1, t_param *p2)
{
  return eq_imp(p1->a,p2->a);
}

static int idcomp(const void *a,const void *b)
{
  t_param *pa,*pb;
  int     d;
  
  pa=(t_param *)a;
  pb=(t_param *)b;
  if ((d=(pa->a[0]-pb->a[0])) != 0)
    return d;
  else if ((d=(pa->a[3]-pb->a[3])) != 0)
    return d;
  else if ((d=(pa->a[1]-pb->a[1])) != 0)
    return d;
  else
    return (int) (pa->a[2]-pb->a[2]);
}

static void sort_id(int nr,t_param ps[])
{
  int i,tmp;
  
  /* First swap order of atoms around if necessary */
  for(i=0; (i<nr); i++) {
    if (ps[i].a[3] < ps[i].a[0]) {
      tmp = ps[i].a[3]; ps[i].a[3] = ps[i].a[0]; ps[i].a[0] = tmp;
      tmp = ps[i].a[2]; ps[i].a[2] = ps[i].a[1]; ps[i].a[1] = tmp;
    }
  }
  /* Now sort it */
  if (nr > 1)
    qsort(ps,nr,(size_t)sizeof(ps[0]),idcomp);
}

static void dump_param(FILE *fp,char *title,int n,t_param ps[])
{
  int i,j;
  
  fprintf(fp,"%s: %d entries\n",title,n);
  for(i=0; (i<n); i++) {
    fprintf(fp,"%3d:  A=[ ",i);
    for(j=0; (j<MAXATOMLIST); j++)
      fprintf(fp," %5d",ps[i].a[j]);
    fprintf(fp,"]  C=[");
    for(j=0; (j<MAXFORCEPARAM); j++)
      fprintf(fp," %10.5e",ps[i].c[j]);
    fprintf(fp,"]\n");  
  }
}

static t_rbonded *is_imp(t_param *p,t_atoms *atoms,t_hackblock hb[])
{
  int        j,n,maxresnr,start;
  atom_id    aa0,a0[MAXATOMLIST];
  char      *atom;
  t_rbondeds *idihs;

  if (!hb)
    return NULL;
  /* Find the max residue number in this dihedral */
  maxresnr=0;
  for(j=0; j<4; j++)
    maxresnr=max(maxresnr,atoms->atom[p->a[j]].resnr);
  
  /* Now find the start of this residue */
  for(start=0; start < atoms->nr; start++)
    if (atoms->atom[start].resnr == maxresnr)
      break;

  /* See if there are any impropers defined for this residue */
  idihs = &hb[atoms->atom[start].resnr].rb[ebtsIDIHS];
  for(n=0; n < idihs->nb; n++) {
    for(j=0; (j<4); j++) {
      atom=idihs->b[n].a[j];
      aa0=search_atom(atom,start,atoms->nr,atoms->atom,atoms->atomname);
      if (aa0 == NO_ATID) {
	if (debug) 
	  fprintf(debug,"Atom %s not found in res %d (maxresnr=%d) "
		  "in is_imp\n",atom,atoms->atom[start].resnr,maxresnr);
	break;
      } else 
	a0[j] = aa0;
    }
    if (j==4) /* Not broken out */
      if (eq_imp(p->a,a0))
	return &idihs->b[n];
  }
  return NULL;
}

static int n_hydro(atom_id a[],char ***atomname)
{
  int i,nh=0;
  char c0,c1,*aname;

  for(i=0; (i<4); i+=3) {
    aname=*atomname[a[i]];
    c0=toupper(aname[0]);
    if (c0 == 'H')
      nh++;
    else if (((int)strlen(aname) > 1) && (c0 >= '0') && (c0 <= '9')) {
      c1=toupper(aname[1]);
      if (c1 == 'H')
	nh++;
    }
  }
  return nh;
}

static void pdih2idih(t_param *alldih, int *nalldih,t_param idih[],int *nidih,
		      t_atoms *atoms, t_hackblock hb[],bool bAlldih)
{
  t_param   *dih;
  int       ndih;
  char      *a0;
  t_rbondeds *idihs;
  t_rbonded  *hbidih;
  int       i,j,k,l,start,aa0;
  int       *index,nind;
  atom_id   ai[MAXATOMLIST];
  bool      bStop,bIsSet,bKeep;
  int bestl,nh,minh;
  
  /* First add all the impropers from the residue database
   * to the list.
   */
  start=0;
  if (hb != NULL) {
    for(i=0; (i<atoms->nres); i++) {
      idihs=&hb[i].rb[ebtsIDIHS];
      for(j=0; (j<idihs->nb); j++) {
	bStop=FALSE;
	for(k=0; (k<4) && !bStop; k++) {
	  ai[k]=search_atom(idihs->b[j].a[k],start,
			    atoms->nr,atoms->atom,atoms->atomname);
	  if (ai[k] == NO_ATID) {
	    if (debug) 
	      fprintf(debug,"Atom %s (%d) not found in res %d in pdih2idih\n",
		      idihs->b[j].a[k], k, atoms->atom[start].resnr);
	    bStop=TRUE;
	  }
	}
	if (!bStop) {
	  /* Not broken out */
	  set_p(&idih[*nidih],ai,NULL,idihs->b[j].s);
	  (*nidih)++;
	}
      }
      while ((start<atoms->nr) && (atoms->atom[start].resnr==i))
	start++;
    }
  }
  if (*nalldih == 0)
    return;
  
  /* Copy the impropers and dihedrals to separate arrays. */
  snew(dih,*nalldih);
  ndih = 0;
  for(i=0; i<*nalldih; i++) {
    hbidih = is_imp(&alldih[i],atoms,hb);
    if ( hbidih ) {
      set_p(&idih[*nidih],alldih[i].a,NULL,hbidih->s);
      (*nidih)++;
    } else {
      cpparam(&dih[ndih],&alldih[i]);
      ndih++;
    }
  }
  
  /* Now, because this list still contains the double entries,
   * keep the dihedral with parameters or the first one.
   */
    
  snew(index,ndih);
  nind=0;
  if (bAlldih) {
    fprintf(stderr,"Keeping all generated dihedrals\n");
    for(i=0; i<ndih; i++) 
      if ((i==0) || !deq2(&dih[i],&dih[i-1]))
	index[nind++]=i;
  } else {
    for(i=0; i<ndih; i++) 
      if (!remove_dih(dih,i,ndih)) 
	index[nind++]=i;
  }
  index[nind]=ndih;

  /* if we don't want all dihedrals, we need to select the ones with the 
   *  fewest hydrogens
   */
  
  k=0;
  for(i=0; i<nind; i++) {
    bIsSet = (dih[index[i]].c[MAXFORCEPARAM-1] != NOTSET);
    bKeep = TRUE;
    if (!bIsSet)
      /* remove the dihedral if there is an improper on the same bond */
      for(j=0; (j<(*nidih)) && bKeep; j++)
	bKeep = !deq(&dih[index[i]],&idih[j]);

    if (bKeep) {
      /* Now select the "fittest" dihedral:
       * the one with as few hydrogens as possible 
       */
      
      /* Best choice to get dihedral from */
      bestl=index[i];
      if (!bAlldih && !bIsSet) {
	/* Minimum number of hydrogens for i and l atoms */
	minh=2;
	for(l=index[i]; (l<index[i+1]) && deq(&dih[index[i]],&dih[l]); l++) {
	  if ((nh=n_hydro(dih[l].a,atoms->atomname)) < minh) {
	    minh=nh;
	    bestl=l;
	  }
	  if (minh == 0)
	    break;
	}
      }
      for(j=0; (j<MAXATOMLIST); j++)
	alldih[k].a[j] = dih[bestl].a[j];
      for(j=0; (j<MAXFORCEPARAM); j++)
	alldih[k].c[j] = dih[bestl].c[j];
      set_p_string(&(alldih[k]),dih[bestl].s);
      k++;
    }
  }
  for (i=k; i < *nalldih; i++)
    strcpy(alldih[i].s,"");
  *nalldih = k;

  sfree(dih);
  sfree(index);
}

static int nb_dist(t_nextnb *nnb,int ai,int aj)
{
  int nre,nrx,NRE;
  int *nrexcl;
  int *a;
  
  if (ai == aj)
    return 0;
  
  NRE=-1;
  nrexcl=nnb->nrexcl[ai];
  for(nre=1; (nre < nnb->nrex); nre++) {
    a=nnb->a[ai][nre];
    for(nrx=0; (nrx < nrexcl[nre]); nrx++) {
      if ((aj == a[nrx]) && (NRE == -1))
	NRE=nre;
    }
  }
  return NRE;
}

bool is_hydro(t_atoms *atoms,int ai)
{
  return ((*(atoms->atomname[ai]))[0] == 'H');
}

static void get_atomnames_min(int n,char anm[4][12],
			      int res,t_atoms *atoms,atom_id *a)
{
  int m;

  /* Assume ascending residue numbering */
  for(m=0; m<n; m++) {
    if (atoms->atom[a[m]].resnr < res)
      strcpy(anm[m],"-");
    else if (atoms->atom[a[m]].resnr > res)
      strcpy(anm[m],"+");
    else
      strcpy(anm[m],"");
    strcat(anm[m],*(atoms->atomname[a[m]]));
  }
}

static void gen_excls(t_atoms *atoms, t_excls *excls, t_hackblock hb[])
{
  int        r;
  atom_id    a,astart,i1,i2,itmp;
  t_rbondeds *hbexcl;
  int        e;
  char       *anm;

  astart = 0;
  for(a=0; a<atoms->nr; a++) {
    r = atoms->atom[a].resnr;
    if (a==atoms->nr-1 || atoms->atom[a+1].resnr!=r) {
      hbexcl = &hb[r].rb[ebtsEXCLS];
      
      for(e=0; e<hbexcl->nb; e++) {
	anm = hbexcl->b[e].a[0];
	i1 = search_atom(anm,astart,atoms->nr,atoms->atom,atoms->atomname);
	if (i1 == NO_ATID)
	  fatal_error(0,"atom name %s not found in residue %s %d while "
		      "generating exclusions",anm,*atoms->resname[r],r+1);
	anm = hbexcl->b[e].a[1];
	i2 = search_atom(anm,astart,atoms->nr,atoms->atom,atoms->atomname);
	if (i2 == NO_ATID)
	  fatal_error(0,"atom name %s not found in residue %s %d while "
		      "generating exclusions",anm,*atoms->resname[r],r+1);
	if (i1 > i2) {
	  itmp = i1;
	  i1 = i2;
	  i2 = itmp;
	}
	srenew(excls[i1].e,excls[i1].nr+1);
	excls[i1].e[excls[i1].nr] = i2;
	excls[i1].nr++;
      }
      
      astart = a+1;
    }
  }

  for(a=0; a<atoms->nr; a++)
    if (excls[a].nr > 1)
      qsort(excls[a].e,excls[a].nr,(size_t)sizeof(atom_id),atom_id_comp);
}

static void remove_excl(t_excls *excls, int remove)
{
  int i;

  for(i=remove+1; i<excls->nr; i++)
    excls->e[i-1] = excls->e[i];
  
  excls->nr--;
}

static void clean_excls(t_nextnb *nnb, int nrexcl, t_excls excls[])
{
  int i,j,j1,k,k1,l,l1,m,n,e;
  t_excls *excl;

  if (nrexcl >= 1)
    /* extract all i-j-k-l neighbours from nnb struct */
    for(i=0; (i<nnb->nr); i++) {
      /* For all particles */
      excl = &excls[i];
      
      for(j=0; (j<nnb->nrexcl[i][1]); j++) {
	/* For all first neighbours */
	j1=nnb->a[i][1][j];
	
	for(e=0; e<excl->nr; e++)
	  if (excl->e[e] == j1)
	    remove_excl(excl,e);
	
	if (nrexcl >= 2)
	  for(k=0; (k<nnb->nrexcl[j1][1]); k++) {
	    /* For all first neighbours of j1 */
	    k1=nnb->a[j1][1][k];
	  
	    for(e=0; e<excl->nr; e++)
	      if (excl->e[e] == k1)
		remove_excl(excl,e);
	    
	    if (nrexcl >= 3)
	      for(l=0; (l<nnb->nrexcl[k1][1]); l++) {
		/* For all first neighbours of k1 */
		l1=nnb->a[k1][1][l];

		for(e=0; e<excl->nr; e++)
		  if (excl->e[e] == l1)
		    remove_excl(excl,e);
	      }
	  }
      }
    }
}

void gen_pad(t_nextnb *nnb, t_atoms *atoms, int nrexcl, bool bH14,
	     t_params plist[], t_excls excls[], t_hackblock hb[], bool bAlldih)
{
  t_param *ang,*dih,*pai,*idih;
  t_rbondeds *hbang, *hbdih;
  char    anm[4][12];
  int     res,minres,maxres;
  int     i,j,j1,k,k1,l,l1,m,n,i1,i2;
  int     maxang,maxdih,maxidih,maxpai;
  int     nang,ndih,npai,nidih,nbd;
  int     dang,ddih;
  bool    bFound,bExcl;
  
  /* These are the angles, pairs, impropers and dihedrals that we generate
   * from the bonds. The ones that are already there from the rtp file
   * will be retained.
   */
  nang    = 0;
  nidih   = 0;
  npai    = 0;
  ndih    = 0;
  dang    = 6*nnb->nr;
  ddih    = 24*nnb->nr;
  maxang  = dang;
  maxdih  = maxpai = maxidih = ddih;
  snew(ang, maxang);
  snew(dih, maxdih);
  snew(pai, maxpai);
  snew(idih,maxidih);

  if (hb)
    gen_excls(atoms,excls,hb);
  
  /* extract all i-j-k-l neighbours from nnb struct */
  for(i=0; (i<nnb->nr); i++) 
    /* For all particles */
    for(j=0; (j<nnb->nrexcl[i][1]); j++) {
      /* For all first neighbours */
      j1=nnb->a[i][1][j];
      for(k=0; (k<nnb->nrexcl[j1][1]); k++) {
	/* For all first neighbours of j1 */
	k1=nnb->a[j1][1][k];
	if (k1 != i) {
	  if (nang == maxang) {
	    srenew(ang,maxang+dang);
	    memset(ang+maxang,0,sizeof(ang[0]));
	    maxang += dang;
	  }
	  ang[nang].AJ=j1;
	  if (i < k1) {
	    ang[nang].AI=i;
	    ang[nang].AK=k1;
	  }
	  else {
	    ang[nang].AI=k1;
	    ang[nang].AK=i;
	  }
	  ang[nang].C0=NOTSET;
	  ang[nang].C1=NOTSET;
	  set_p_string(&(ang[nang]),"");
	  if (hb) {
	    minres = atoms->atom[ang[nang].a[0]].resnr;
	    maxres = minres;
	    for(m=1; m<3; m++) {
	      minres = min(minres,atoms->atom[ang[nang].a[m]].resnr);
	      maxres = max(maxres,atoms->atom[ang[nang].a[m]].resnr);
	    }
	    res = 2*minres-maxres;
	    do {
	      res += maxres-minres;
	      hbang=&hb[res].rb[ebtsANGLES];
	      for(l=0; (l<hbang->nb); l++) {
		get_atomnames_min(3,anm,res,atoms,ang[nang].a); 
		if (strcmp(anm[1],hbang->b[l].AJ)==0) {
		  bFound=FALSE;
		  for (m=0; m<3; m+=2)
		    bFound=(bFound ||
			    ((strcmp(anm[m],hbang->b[l].AI)==0) &&
			     (strcmp(anm[2-m],hbang->b[l].AK)==0)));
		  if (bFound) {
		    set_p_string(&(ang[nang]),hbang->b[l].s);
		  }
		}
	      }
	    } while (res < maxres);
	  }
	  nang++;
	  for(l=0; (l<nnb->nrexcl[k1][1]); l++) {
	    /* For all first neighbours of k1 */
	    l1=nnb->a[k1][1][l];
	    if ((l1 != i) && (l1 != j1)) {
	      if (ndih == maxdih) {
		srenew(dih,maxdih+ddih);
		memset(dih+maxdih,0,sizeof(dih[0]));
		srenew(idih,maxdih+ddih);
		memset(idih+maxdih,0,sizeof(idih[0]));
		srenew(pai,maxdih+ddih);
		memset(pai+maxdih,0,sizeof(pai[0]));
		maxdih += ddih;
	      }
	      if (j1 < k1) {
		dih[ndih].AI=i;
		dih[ndih].AJ=j1;
		dih[ndih].AK=k1;
		dih[ndih].AL=l1;
	      }
	      else {
		dih[ndih].AI=l1;
		dih[ndih].AJ=k1;
		dih[ndih].AK=j1;
		dih[ndih].AL=i;
	      }
	      for (m=0; m<MAXFORCEPARAM; m++)
		dih[ndih].c[m]=NOTSET;
	      set_p_string(&(dih[ndih]),"");
	      if (hb) {
		minres = atoms->atom[dih[ndih].a[0]].resnr;
		maxres = minres;
		for(m=1; m<4; m++) {
		  minres = min(minres,atoms->atom[dih[ndih].a[m]].resnr);
		  maxres = max(maxres,atoms->atom[dih[ndih].a[m]].resnr);
		}
		res = 2*minres-maxres;
		do {
		  res += maxres-minres;
		  hbdih=&hb[res].rb[ebtsPDIHS];
		  for(n=0; (n<hbdih->nb); n++) {
		    get_atomnames_min(4,anm,res,atoms,dih[ndih].a);
		    bFound=FALSE;
		    for (m=0; m<2; m++)
		      bFound=(bFound ||
			      ((strcmp(anm[3*m],  hbdih->b[n].AI)==0) &&
			       (strcmp(anm[1+m],  hbdih->b[n].AJ)==0) &&
			       (strcmp(anm[2-m],  hbdih->b[n].AK)==0) &&
			       (strcmp(anm[3-3*m],hbdih->b[n].AL)==0)));
		    if (bFound) {
		      set_p_string(&dih[ndih],hbdih->b[n].s);
			
		      /* Set the last parameter to be able to see
			 if the dihedral was in the rtp list */
		      dih[ndih].c[MAXFORCEPARAM-1] = 0;
		    }
		  }
		} while (res < maxres);
	      }
	      nbd=nb_dist(nnb,i,l1);
	      if (debug)
		fprintf(debug,"Distance (%d-%d) = %d\n",i+1,l1+1,nbd);
	      if (nbd == 3) {
		i1 = min(i,l1);
		i2 = max(i,l1);
		bExcl = FALSE;
		for(m=0; m<excls[i1].nr; m++)
		  bExcl = bExcl || excls[i1].e[m]==i2;
		if (!bExcl) {
		  if (bH14 || !(is_hydro(atoms,i1) && is_hydro(atoms,i2))) {
		    pai[npai].AI=i1;
		    pai[npai].AJ=i2;
		    pai[npai].C0=NOTSET;
		    pai[npai].C1=NOTSET;
		    set_p_string(&(pai[npai]),"");
		    npai++;
		  }
		}
	      }
	      
	      ndih++;
	    }
	  }
	}
      }
    }
  
  /* We now have a params list with double entries for each angle,
   * and even more for each dihedral. We will remove these now.
   */
  /* Sort angles with respect to j-i-k (middle atom first) */
  if (nang > 1)
    qsort(ang,nang,(size_t)sizeof(ang[0]),acomp);
  rm2par(ang,&nang,aeq);

  /* Sort dihedrals with respect to j-k-i-l (middle atoms first) */
  fprintf(stderr,"before sorting: %d dihedrals\n",ndih);
  if (ndih > 1)
    qsort(dih,ndih,(size_t)sizeof(dih[0]),dcomp);
  pdih2idih(dih,&ndih,idih,&nidih,atoms,hb,bAlldih);
  fprintf(stderr,"after sorting: %d dihedrals\n",ndih);
  
  /* Now the dihedrals are sorted and doubles removed, this has to be done
   * for impropers too
   */
  if (debug) dump_param(debug,"Before sort",nidih,idih);
  sort_id(nidih,idih);
  if (debug) dump_param(debug,"After sort",nidih,idih);
  rm2par(idih,&nidih,ideq);
  if (debug) dump_param(debug,"After rm2par",nidih,idih);
  
  /* And for the pairs */
  fprintf(stderr,"There are %d pairs before sorting\n",npai);
  if (npai > 1)
    qsort(pai,npai,(size_t)sizeof(pai[0]),pcomp);
  rm2par(pai,&npai,preq);

  /* Now we have unique lists of angles and dihedrals 
   * Copy them into the destination struct
   */
  cppar(ang, nang, plist,F_ANGLES);
  cppar(dih, ndih, plist,F_PDIHS);
  cppar(idih,nidih,plist,F_IDIHS);
  cppar(pai, npai, plist,F_LJ14);

  /* Remove all exclusions which are within nrexcl */
  clean_excls(nnb,nrexcl,excls);

  sfree(ang);
  sfree(dih);
  sfree(idih);
  sfree(pai);
}

