/*
 * $Id: dum_parm.c,v 1.23 2002/02/28 10:54:41 spoel Exp $
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
static char *SRCID_dum_parm_c = "$Id: dum_parm.c,v 1.23 2002/02/28 10:54:41 spoel Exp $";
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "assert.h"
#include "dum_parm.h"
#include "smalloc.h"
#include "resall.h"
#include "add_par.h"
#include "vec.h"
#include "toputil.h"
#include "physics.h"
#include "index.h"
#include "names.h"
#include "fatal.h"

typedef struct {
  t_iatom a[4];
  real    c;
} t_mybonded;

static void enter_bonded(int nratoms, int *nrbonded, t_mybonded **bondeds, 
			 t_param param)
{
  int j;

  srenew(*bondeds, *nrbonded+1);
  
  /* copy atom numbers */
  for(j=0; j<nratoms; j++)
    (*bondeds)[*nrbonded].a[j] = param.a[j];
  /* copy parameter */
  (*bondeds)[*nrbonded].c = param.C0;
  
  (*nrbonded)++;
}

static void get_bondeds(int nrat, t_iatom atoms[], 
			t_params plist[],
			int *nrbond, t_mybonded **bonds,
			int *nrang,  t_mybonded **angles,
			int *nridih, t_mybonded **idihs )
{
  int     i,j,k,ftype;
  int     nra,nrd,tp,nrcheck;
  t_iatom *ia,adum;
  bool    bCheck;
  t_param param;
  
  if (debug) {
    fprintf(debug,"getting bondeds for %u (nr=%d):",atoms[0]+1,nrat);
    for(k=1; k<nrat; k++)
      fprintf(debug," %u",atoms[k]+1);
    fprintf(debug,"\n");
  }
  for(ftype=0; (ftype<F_NRE); ftype++) {
    if (interaction_function[ftype].flags & (IF_BTYPE | IF_CONSTRAINT))
      /* this is a bond or a constraint */
      nrcheck = 2;
    else if ( interaction_function[ftype].flags & IF_ATYPE )
      /* this is an angle */
      nrcheck = 3;
    else {
      switch(ftype) {
      case F_IDIHS: 
	/* this is an improper dihedral */
	nrcheck = 4;
	break;
      default:
	/* ignore this */
	nrcheck = 0;
      } /* case */
    } /* else */
    
    if (nrcheck)
      for(i=0; (i<plist[ftype].nr); i++) {
	/* now we have something, check includes one of atoms[*] */
	bCheck=FALSE;
	for(j=0; j<nrcheck && !bCheck; j++)
	  for(k=0; k<nrat; k++)
	    bCheck = bCheck || (plist[ftype].param[i].a[j]==atoms[k]);
	
	if (bCheck)
	  /* abuse nrcheck to see if we're adding bond, angle or idih */
	  switch (nrcheck) {
	  case 2: 
	    enter_bonded(nrcheck,nrbond,bonds, plist[ftype].param[i]);
	    break;
	  case 3: 
	    enter_bonded(nrcheck,nrang, angles,plist[ftype].param[i]);
	    break;
	  case 4: 
	    enter_bonded(nrcheck,nridih,idihs, plist[ftype].param[i]);
	    break;
	  } /* case */
      } /* for i */
  } /* for ftype */
}

/* for debug */
static void print_bad(FILE *fp, 
		      int nrbond, t_mybonded *bonds,
		      int nrang,  t_mybonded *angles,
		      int nridih, t_mybonded *idihs )
{
  int i;
  
  if (nrbond) {
    fprintf(fp,"bonds:");
    for(i=0; i<nrbond; i++)
      fprintf(fp," %u-%u (%g)", 
	      bonds[i].AI+1, bonds[i].AJ+1, bonds[i].c);
    fprintf(fp,"\n");
  }
  if (nrang) {
    fprintf(fp,"angles:");
    for(i=0; i<nrang; i++)
      fprintf(fp," %u-%u-%u (%g)", 
	      angles[i].AI+1, angles[i].AJ+1, 
	      angles[i].AK+1, angles[i].c);
    fprintf(fp,"\n");
  }
  if (nridih) {
    fprintf(fp,"idihs:");
    for(i=0; i<nridih; i++)
      fprintf(fp," %u-%u-%u-%u (%g)", 
	      idihs[i].AI+1, idihs[i].AJ+1, 
	      idihs[i].AK+1, idihs[i].AL+1, idihs[i].c);
    fprintf(fp,"\n");
  }
}

static void print_param(FILE *fp, int ftype, int i, t_param *param)
{
  static int pass = 0;
  static int prev_ftype= NOTSET;
  static int prev_i    = NOTSET;
  int j;
  
  if ( (ftype!=prev_ftype) || (i!=prev_i) ) {
    pass = 0;
    prev_ftype= ftype;
    prev_i    = i;
  }
  fprintf(fp,"(%d) plist[%s].param[%d]",
	  pass,interaction_function[ftype].name,i);
  for(j=0; j<NRFP(ftype); j++)
    fprintf(fp,".c[%d]=%g ",j,param->c[j]);
  fprintf(fp,"\n");
  pass++;
}

static real get_bond_length(int nrbond, t_mybonded bonds[], 
			    t_iatom ai, t_iatom aj)
{
  int  i;
  real bondlen;
  
  bondlen=NOTSET;
  for (i=0; i < nrbond && (bondlen==NOTSET); i++) {
    /* check both ways */
    if ( ( (ai == bonds[i].AI) && (aj == bonds[i].AJ) ) || 
	 ( (ai == bonds[i].AJ) && (aj == bonds[i].AI) ) )
      bondlen = bonds[i].c; /* note: bonds[i].c might be NOTSET */
  }
  return bondlen;
}

static real get_angle(int nrang, t_mybonded angles[], 
		      t_iatom ai, t_iatom aj, t_iatom ak)
{
  int  i;
  real angle;
  
  angle=NOTSET;
  for (i=0; i < nrang && (angle==NOTSET); i++) {
    /* check both ways */
    if ( ( (ai==angles[i].AI) && (aj==angles[i].AJ) && (ak==angles[i].AK) ) || 
	 ( (ai==angles[i].AK) && (aj==angles[i].AJ) && (ak==angles[i].AI) ) )
      angle = DEG2RAD*angles[i].c;
  }
  return angle;
}

static bool calc_dum3_param(t_atomtype *atype,
			    t_param *param, t_atoms *at,
			    int nrbond, t_mybonded *bonds,
			    int nrang,  t_mybonded *angles )
{
  /* i = dummy atom            |    ,k
   * j = 1st bonded heavy atom | i-j
   * k,l = 2nd bonded atoms    |    `l
   */
  
  bool bXH3,bError;
  real bjk,bjl,a=-1,b=-1;
  /* check if this is part of a NH3 or CH3 group,
   * i.e. if atom k and l are dummy masses (MNH3 or MCH3) */
  if (debug) {
    int i;
    for (i=0; i<4; i++)
      fprintf(debug,"atom %u type %s ",
	      param->a[i]+1,type2nm(at->atom[param->a[i]].type,atype));
    fprintf(debug,"\n");
  }
  bXH3 = 
    ( (strcasecmp(type2nm(at->atom[param->AK].type,atype),"MNH3")==0) &&
      (strcasecmp(type2nm(at->atom[param->AL].type,atype),"MNH3")==0) ) ||
    ( (strcasecmp(type2nm(at->atom[param->AK].type,atype),"MCH3")==0) &&
      (strcasecmp(type2nm(at->atom[param->AL].type,atype),"MCH3")==0) );
  
  bjk = get_bond_length(nrbond, bonds, param->AJ, param->AK);
  bjl = get_bond_length(nrbond, bonds, param->AJ, param->AL);
  bError = (bjk==NOTSET) || (bjl==NOTSET);
  if (bXH3) {
    /* now we get some XH3 group specific construction */
    /* note: we call the heavy atom 'C' and the X atom 'N' */
    real bMM,bCM,bCN,bNH,aCNH,dH,rH,dM,rM;
    int aN;
    
    /* check if bonds from heavy atom (j) to dummy masses (k,l) are equal: */
    bError = bError || (bjk!=bjl);
    
    /* the X atom (C or N) in the XH3 group is the first after the masses: */
    aN = max(param->AK,param->AL)+1;
    
    /* get common bonds */
    bMM = get_bond_length(nrbond, bonds, param->AK, param->AL);
    bCM = bjk;
    bCN = get_bond_length(nrbond, bonds, param->AJ, aN);
    bError = bError || (bMM==NOTSET) || (bCN==NOTSET);
    
    /* calculate common things */
    rM  = 0.5*bMM;
    dM  = sqrt( sqr(bCM) - sqr(rM) );
    
    /* are we dealing with the X atom? */
    if ( param->AI == aN ) {
      /* this is trivial */
      a = b = 0.5 * bCN/dM;
      
    } else {
      /* get other bondlengths and angles: */
      bNH = get_bond_length(nrbond, bonds, aN, param->AI);
      aCNH= get_angle      (nrang, angles, param->AJ, aN, param->AI);
      bError = bError || (bNH==NOTSET) || (aCNH==NOTSET);
      
      /* calculate */
      dH  = bCN - bNH * cos(aCNH);
      rH  = bNH * sin(aCNH);
      
      a = 0.5 * ( dH/dM + rH/rM );
      b = 0.5 * ( dH/dM - rH/rM );
    }
  } else
    fatal_error(0,"calc_dum3_param not implemented for the general case "
		"(atom %d)",param->AI+1);
  
  param->C0 = a;
  param->C1 = b;
  
  if (debug)
    fprintf(debug,"params for dummy3 %u: %g %g\n",
	    param->AI+1,param->C0,param->C1);
  
  return bError;
}

static bool calc_dum3fd_param(t_param *param,
			      int nrbond, t_mybonded *bonds,
			      int nrang,  t_mybonded *angles)
{
  /* i = dummy atom            |    ,k
   * j = 1st bonded heavy atom | i-j
   * k,l = 2nd bonded atoms    |    `l
   */

  bool bError;
  real bij,bjk,bjl,aijk,aijl,rk,rl;
  
  bij = get_bond_length(nrbond, bonds, param->AI, param->AJ);
  bjk = get_bond_length(nrbond, bonds, param->AJ, param->AK);
  bjl = get_bond_length(nrbond, bonds, param->AJ, param->AL);
  aijk= get_angle      (nrang, angles, param->AI, param->AJ, param->AK);
  aijl= get_angle      (nrang, angles, param->AI, param->AJ, param->AL);
  bError = (bij==NOTSET) || (bjk==NOTSET) || (bjl==NOTSET) || 
    (aijk==NOTSET) || (aijl==NOTSET);
  
  rk = bjk * sin(aijk);
  rl = bjl * sin(aijl);
  param->C0 = rk / (rk + rl);
  param->C1 = -bij; /* 'bond'-length for fixed distance dummy */
  
  if (debug)
    fprintf(debug,"params for dummy3fd %u: %g %g\n",
	    param->AI+1,param->C0,param->C1);
  return bError;
}

static bool calc_dum3fad_param(t_param *param,
			       int nrbond, t_mybonded *bonds,
			       int nrang,  t_mybonded *angles)
{
  /* i = dummy atom            |
   * j = 1st bonded heavy atom | i-j
   * k = 2nd bonded heavy atom |    `k-l
   * l = 3d bonded heavy atom  |
   */

  bool bSwapParity,bError;
  real bij,aijk;
  
  bSwapParity = ( param->C1 == -1 );
  
  bij  = get_bond_length(nrbond, bonds, param->AI, param->AJ);
  aijk = get_angle      (nrang, angles, param->AI, param->AJ, param->AK);
  bError = (bij==NOTSET) || (aijk==NOTSET);
  
  param->C1 = bij;          /* 'bond'-length for fixed distance dummy */
  param->C0 = RAD2DEG*aijk; /* 'bond'-angle for fixed angle dummy */
  
  if (bSwapParity)
    param->C0 = 360 - param->C0;
  
  if (debug)
    fprintf(debug,"params for dummy3fad %u: %g %g\n",
	    param->AI+1,param->C0,param->C1);
  return bError;
}

static bool calc_dum3out_param(t_atomtype *atype,
			       t_param *param, t_atoms *at,
			       int nrbond, t_mybonded *bonds,
			       int nrang,  t_mybonded *angles)
{
  /* i = dummy atom            |    ,k
   * j = 1st bonded heavy atom | i-j
   * k,l = 2nd bonded atoms    |    `l
   * NOTE: i is out of the j-k-l plane!
   */
  
  bool bXH3,bError,bSwapParity;
  real bij,bjk,bjl,aijk,aijl,akjl,pijk,pijl,a,b,c;
  
  /* check if this is part of a NH3 or CH3 group,
   * i.e. if atom k and l are dummy masses (MNH3 or MCH3) */
  if (debug) {
    int i;
    for (i=0; i<4; i++)
      fprintf(debug,"atom %u type %s ",
	      param->a[i]+1,type2nm(at->atom[param->a[i]].type,atype));
    fprintf(debug,"\n");
  }
  bXH3 = 
    ( (strcasecmp(type2nm(at->atom[param->AK].type,atype),"MNH3")==0) &&
      (strcasecmp(type2nm(at->atom[param->AL].type,atype),"MNH3")==0) ) ||
    ( (strcasecmp(type2nm(at->atom[param->AK].type,atype),"MCH3")==0) &&
      (strcasecmp(type2nm(at->atom[param->AL].type,atype),"MCH3")==0) );
  
  /* check if construction parity must be swapped */  
  bSwapParity = ( param->C1 == -1 );
  
  bjk = get_bond_length(nrbond, bonds, param->AJ, param->AK);
  bjl = get_bond_length(nrbond, bonds, param->AJ, param->AL);
  bError = (bjk==NOTSET) || (bjl==NOTSET);
  if (bXH3) {
    /* now we get some XH3 group specific construction */
    /* note: we call the heavy atom 'C' and the X atom 'N' */
    real bMM,bCM,bCN,bNH,aCNH,dH,rH,rHx,rHy,dM,rM;
    int aN;
    
    /* check if bonds from heavy atom (j) to dummy masses (k,l) are equal: */
    bError = bError || (bjk!=bjl);
    
    /* the X atom (C or N) in the XH3 group is the first after the masses: */
    aN = max(param->AK,param->AL)+1;
    
    /* get all bondlengths and angles: */
    bMM = get_bond_length(nrbond, bonds, param->AK, param->AL);
    bCM = bjk;
    bCN = get_bond_length(nrbond, bonds, param->AJ, aN);
    bNH = get_bond_length(nrbond, bonds, aN, param->AI);
    aCNH= get_angle      (nrang, angles, param->AJ, aN, param->AI);
    bError = bError || 
      (bMM==NOTSET) || (bCN==NOTSET) || (bNH==NOTSET) || (aCNH==NOTSET);
    
    /* calculate */
    dH  = bCN - bNH * cos(aCNH);
    rH  = bNH * sin(aCNH);
    /* we assume the H's are symmetrically distributed */
    rHx = rH*cos(DEG2RAD*30);
    rHy = rH*sin(DEG2RAD*30);
    rM  = 0.5*bMM;
    dM  = sqrt( sqr(bCM) - sqr(rM) );
    a   = 0.5*( (dH/dM) - (rHy/rM) );
    b   = 0.5*( (dH/dM) + (rHy/rM) );
    c   = rHx / (2*dM*rM);
    
  } else {
    /* this is the general construction */
    
    bij = get_bond_length(nrbond, bonds, param->AI, param->AJ);
    aijk= get_angle      (nrang, angles, param->AI, param->AJ, param->AK);
    aijl= get_angle      (nrang, angles, param->AI, param->AJ, param->AL);
    akjl= get_angle      (nrang, angles, param->AK, param->AJ, param->AL);
    bError = bError || 
      (bij==NOTSET) || (aijk==NOTSET) || (aijl==NOTSET) || (akjl==NOTSET);
  
    pijk = cos(aijk)*bij;
    pijl = cos(aijl)*bij;
    a = ( pijk + (pijk*cos(akjl)-pijl) * cos(akjl) / sqr(sin(akjl)) ) / bjk;
    b = ( pijl + (pijl*cos(akjl)-pijk) * cos(akjl) / sqr(sin(akjl)) ) / bjl;
    c = - sqrt( sqr(bij) - 
		( sqr(pijk) - 2*pijk*pijl*cos(akjl) + sqr(pijl) ) 
		/ sqr(sin(akjl)) )
      / ( bjk*bjl*sin(akjl) );
  }
  
  param->C0 = a;
  param->C1 = b;
  if (bSwapParity)
    param->C2 = -c;
  else
    param->C2 =  c;
  if (debug)
    fprintf(debug,"params for dummy3out %u: %g %g %g\n",
	    param->AI+1,param->C0,param->C1,param->C2);
  return bError;
}

static bool calc_dum4fd_param(t_param *param,
			      int nrbond, t_mybonded *bonds,
			      int nrang,  t_mybonded *angles)
{
  /* i = dummy atom            |    ,k
   * j = 1st bonded heavy atom | i-j-m
   * k,l,m = 2nd bonded atoms  |    `l
   */
  
  bool bError;
  real bij,bjk,bjl,bjm,aijk,aijl,aijm,akjm,akjl;
  real pk,pl,pm,cosakl,cosakm,sinakl,sinakm,cl,cm;
  
  bij = get_bond_length(nrbond, bonds, param->AI, param->AJ);
  bjk = get_bond_length(nrbond, bonds, param->AJ, param->AK);
  bjl = get_bond_length(nrbond, bonds, param->AJ, param->AL);
  bjm = get_bond_length(nrbond, bonds, param->AJ, param->AM);
  aijk= get_angle      (nrang, angles, param->AI, param->AJ, param->AK);
  aijl= get_angle      (nrang, angles, param->AI, param->AJ, param->AL);
  aijm= get_angle      (nrang, angles, param->AI, param->AJ, param->AM);
  akjm= get_angle      (nrang, angles, param->AK, param->AJ, param->AM);
  akjl= get_angle      (nrang, angles, param->AK, param->AJ, param->AL);
  bError = (bij==NOTSET) || (bjk==NOTSET) || (bjl==NOTSET) || (bjm==NOTSET) ||
    (aijk==NOTSET) || (aijl==NOTSET) || (aijm==NOTSET) || (akjm==NOTSET) || 
    (akjl==NOTSET);
  
  pk = bjk*sin(aijk);
  pl = bjl*sin(aijl);
  pm = bjm*sin(aijm);
  cosakl = (cos(akjl) - cos(aijk)*cos(aijl)) / (sin(aijk)*sin(aijl));
  cosakm = (cos(akjm) - cos(aijk)*cos(aijm)) / (sin(aijk)*sin(aijm));
  if ( cosakl < -1 || cosakl > 1 || cosakm < -1 || cosakm > 1 )
    fatal_error(0,"invalid construction in calc_dum4fd for atom %u: "
		"cosakl=%g cosakm=%g\n",param->AI+1,cosakl,cosakm);
  sinakl = sqrt(1-sqr(cosakl));
  sinakm = sqrt(1-sqr(cosakm));
  
  /* note: there is a '+' because of the way the sines are calculated */
  cl = -pk / ( pl*cosakl - pk + pl*sinakl*(pm*cosakm-pk)/(pm*sinakm) );
  cm = -pk / ( pm*cosakm - pk + pm*sinakm*(pl*cosakl-pk)/(pl*sinakl) );
  
  param->C0 = cl;
  param->C1 = cm;
  param->C2 = -bij;
  if (debug)
    fprintf(debug,"params for dummy4fd %u: %g %g %g\n",
	    param->AI+1,param->C0,param->C1,param->C2);
  
  return bError;
}

int set_dummies(bool bVerbose, t_atoms *atoms, t_atomtype atype,
		t_params plist[])
{
  int i,j,ftype;
  int ndum,nrbond,nrang,nridih,nrset;
  bool bFirst,bSet,bERROR;
  t_mybonded *bonds;
  t_mybonded *angles;
  t_mybonded *idihs;
  
  bFirst = TRUE;
  bERROR = TRUE;
  ndum=0;
  if (debug)
    fprintf(debug, "\nCalculating parameters for dummy atoms\n");  
  for(ftype=0; (ftype<F_NRE); ftype++)
    if (interaction_function[ftype].flags & IF_DUMMY) {
      nrset=0;
      ndum+=plist[ftype].nr;
      for(i=0; (i<plist[ftype].nr); i++) {
	/* check if all parameters are set */
	bSet=TRUE;
	for(j=0; j<NRFP(ftype) && bSet; j++)
	  bSet = plist[ftype].param[i].c[j]!=NOTSET;

	if (debug) {
	  fprintf(debug,"bSet=%s ",bool_names[bSet]);
	  print_param(debug,ftype,i,&plist[ftype].param[i]);
	}
	if (!bSet) {
	  if (bVerbose && bFirst) {
	    fprintf(stderr,"Calculating parameters for dummy atoms\n");
	    bFirst=FALSE;
	  }
	  
	  nrbond=nrang=nridih=0;
	  bonds = NULL;
	  angles= NULL;
	  idihs = NULL;
	  nrset++;
	  /* now set the dummy parameters: */
	  get_bondeds(NRAL(ftype), plist[ftype].param[i].a, plist, 
		      &nrbond, &bonds, &nrang,  &angles, &nridih, &idihs);
	  if (debug) {
	    fprintf(debug, "Found %d bonds, %d angles and %d idihs "
		    "for dummy atom %u (%s)\n",nrbond,nrang,nridih,
		    plist[ftype].param[i].AI+1,
		    interaction_function[ftype].longname);
	    print_bad(debug, nrbond, bonds, nrang, angles, nridih, idihs);
	  } /* debug */
	  switch(ftype) {
	  case F_DUMMY3: 
	    bERROR = 
	      calc_dum3_param(&atype, &(plist[ftype].param[i]), atoms,
			      nrbond, bonds, nrang, angles);
	    break;
	  case F_DUMMY3FD:
	    bERROR = 
	      calc_dum3fd_param(&(plist[ftype].param[i]),
				nrbond, bonds, nrang, angles);
	    break;
	  case F_DUMMY3FAD:
	    bERROR = 
	      calc_dum3fad_param(&(plist[ftype].param[i]),
				 nrbond, bonds, nrang, angles);
	    break;
	  case F_DUMMY3OUT:
	    bERROR = 
	      calc_dum3out_param(&atype, &(plist[ftype].param[i]), atoms,
				 nrbond, bonds, nrang, angles);
	    break;
	  case F_DUMMY4FD:
	    bERROR = 
	      calc_dum4fd_param(&(plist[ftype].param[i]), 
				nrbond, bonds, nrang, angles);
	    break;
	  default:
	    fatal_error(0,"Automatic parameter generation not supported "
			"for %s atom %d",
			interaction_function[ftype].longname,
			plist[ftype].param[i].AI+1);
	  } /* switch */
	  if (bERROR)
	    fatal_error(0,"Automatic parameter generation not supported "
			"for %s atom %d for this bonding configuration",
			interaction_function[ftype].longname,
			plist[ftype].param[i].AI+1);
	  sfree(bonds);
	  sfree(angles);
	  sfree(idihs);
	} /* if bSet */
      } /* for i */
      if (debug && plist[ftype].nr)
	fprintf(stderr,"Calculated parameters for %d out of %d %s atoms\n",
		nrset,plist[ftype].nr,interaction_function[ftype].longname);
    } /* if IF_DUMMY */
  
  return ndum;
}

void set_dummies_ptype(bool bVerbose, t_idef *idef, t_atoms *atoms)
{
  int i,ftype;
  int nra,nrd,tp;
  t_iatom *ia,adum;
  
  if (bVerbose)
    fprintf(stderr,"Setting particle type to Dummy for dummy atoms\n");
  if (debug)
    fprintf(stderr,"checking %d functypes\n",F_NRE);
  for(ftype=0; (ftype<F_NRE); ftype++) {
    if (interaction_function[ftype].flags & IF_DUMMY) {
      nra    = interaction_function[ftype].nratoms;
      nrd    = idef->il[ftype].nr;
      ia     = idef->il[ftype].iatoms;
      
      if (debug && nrd)
	fprintf(stderr,"doing %d %s dummies\n",
		(nrd / (nra+1)),interaction_function[ftype].longname);
      
      for(i=0; (i<nrd); ) {
	tp   = ia[0];
	assert(ftype == idef->functype[tp]);
	
	/* The dummy atom */
	adum = ia[1];
	atoms->atom[adum].ptype=eptDummy;
	
	i  += nra+1;
	ia += nra+1;
      }
    }
  }
  
}

typedef struct { 
  int ftype,parnr;
} t_pindex;

static void check_dum_constraints(t_params *plist, 
				  int cftype, int dummy_type[])
{
  int      i,k,n;
  atom_id  atom;
  t_params *ps;
  
  n=0;
  ps = &(plist[cftype]);
  for(i=0; (i<ps->nr); i++)
    for(k=0; k<2; k++) {
      atom = ps->param[i].a[k];
      if (dummy_type[atom]!=NOTSET) {
	fprintf(stderr,
		"ERROR: Cannot have constraint (%u-%u) with dummy atom (%u)\n",
		ps->param[i].AI+1, ps->param[i].AJ+1, atom+1);
	n++;
      }
    }
  if (n)
    fatal_error(0,"There were %d dummy atoms involved in constraints",n);
}

static void clean_dum_bonds(t_params *plist, t_pindex pindex[], 
			    int cftype, int dummy_type[])
{
  int      ftype,i,j,parnr,k,l,m,n,ndum,nOut,kept_i,dumnral,dumtype;
  int      nconverted,nremoved;
  atom_id  atom,oatom,constr,at1,at2;
  atom_id  dumatoms[MAXATOMLIST];
  bool     bKeep,bRemove,bUsed,bPresent,bThisFD,bThisOUT,bAllFD,bFirstTwo;
  t_params *ps;

  if (cftype == F_CONNBONDS)
    return;
  
  ps = &(plist[cftype]);
  dumnral=0;
  kept_i=0;
  nconverted=0;
  nremoved=0;
  nOut=0;
  for(i=0; (i<ps->nr); i++) { /* for all bonds in the plist */
    bKeep=FALSE;
    bRemove=FALSE;
    bAllFD=TRUE;
    /* check if all dummies are constructed from the same atoms */
    ndum=0;
    if(debug) 
      fprintf(debug,"constr %u %u:",ps->param[i].AI+1,ps->param[i].AJ+1);
    for(k=0; (k<2) && !bKeep && !bRemove; k++) { 
      /* for all atoms in the bond */
      atom = ps->param[i].a[k];
      if (dummy_type[atom]!=NOTSET) {
	if(debug) {
	  fprintf(debug," d%d[%d: %d %d %d]",k,atom+1,
		  plist[pindex[atom].ftype].param[pindex[atom].parnr].AJ+1,
		  plist[pindex[atom].ftype].param[pindex[atom].parnr].AK+1,
		  plist[pindex[atom].ftype].param[pindex[atom].parnr].AL+1);
	}
	ndum++;
	bThisFD = ( (pindex[atom].ftype == F_DUMMY3FD ) ||
		    (pindex[atom].ftype == F_DUMMY3FAD) ||
		    (pindex[atom].ftype == F_DUMMY4FD ) );
	bThisOUT= ( (pindex[atom].ftype == F_DUMMY3OUT) &&
		    (interaction_function[cftype].flags & IF_CONSTRAINT) );
	bAllFD = bAllFD && bThisFD;
	if (bThisFD || bThisOUT) {
	  if(debug)fprintf(debug," %s",bThisOUT?"out":"fd");
	  oatom = ps->param[i].a[1-k]; /* the other atom */
	  if ( dummy_type[oatom]==NOTSET &&
	       oatom==plist[pindex[atom].ftype].param[pindex[atom].parnr].AJ ){
	    /* if the other atom isn't a dummy, and it is AI */
	    bRemove=TRUE;
	    if (bThisOUT)
	      nOut++;
	    if(debug)fprintf(debug," D-AI");
	  }
	}
	if (!bRemove) {
	  if (ndum==1) {
	    /* if this is the first dummy we encounter then
	       store construction atoms */
	    dumnral=NRAL(pindex[atom].ftype)-1;
	    for(m=0; (m<dumnral); m++)
	      dumatoms[m]=
		plist[pindex[atom].ftype].param[pindex[atom].parnr].a[m+1];
	  } else {
	    /* if it is not the first then
	       check if this dummy is constructed from the same atoms */
	    if (dumnral == NRAL(pindex[atom].ftype)-1 )
	      for(m=0; (m<dumnral) && !bKeep; m++) {
		bPresent=FALSE;
		constr=
		  plist[pindex[atom].ftype].param[pindex[atom].parnr].a[m+1];
		for(n=0; (n<dumnral) && !bPresent; n++)
		  if (constr == dumatoms[n])
		    bPresent=TRUE;
		if (!bPresent) {
		  bKeep=TRUE;
		  if(debug)fprintf(debug," !present");
		}
	      }
	    else {
	      bKeep=TRUE;
	      if(debug)fprintf(debug," !same#at");
	    }
	  }
	}
      }
    }
    
    if (bRemove) 
      bKeep=FALSE;
    else {
      /* if we have no dummies in this bond, keep it */
      if (ndum==0) {
	if (debug)fprintf(debug," no dum");
	bKeep=TRUE;
      }
    
      /* check if all non-dummy atoms are used in construction: */
      bFirstTwo=TRUE;
      for(k=0; (k<2) && !bKeep; k++) { /* for all atoms in the bond */
	atom = ps->param[i].a[k];
	if (dummy_type[atom]==NOTSET) {
	  bUsed=FALSE;
	  for(m=0; (m<dumnral) && !bUsed; m++)
	    if (atom == dumatoms[m]) {
	      bUsed=TRUE;
	      bFirstTwo = bFirstTwo && m<2;
	    }
	  if (!bUsed) {
	    bKeep=TRUE;
	    if(debug)fprintf(debug," !used");
	  }
	}
      }
      
      if ( ! ( bAllFD && bFirstTwo ) )
	/* check if all constructing atoms are constrained together */
	for (m=0; m<dumnral && !bKeep; m++) { /* all constr. atoms */
	  at1 = dumatoms[m];
	  at2 = dumatoms[(m+1) % dumnral];
	  bPresent=FALSE;
	  for (ftype=0; ftype<F_NRE; ftype++)
	    if ( interaction_function[ftype].flags & IF_CONSTRAINT )
	      for (j=0; (j<plist[ftype].nr) && !bPresent; j++)
		/* all constraints until one matches */
		bPresent = ( ( (plist[ftype].param[j].AI == at1) &&
			       (plist[ftype].param[j].AJ == at2) ) || 
			     ( (plist[ftype].param[j].AI == at2) &&
			       (plist[ftype].param[j].AJ == at1) ) );
	  if (!bPresent) {
	    bKeep=TRUE;
	    if(debug)fprintf(debug," !bonded");
	  }
	}
    }
    
    if ( bKeep ) {
      if(debug)fprintf(debug," keeping");
      /* now copy the bond to the new array */
      memcpy(&(ps->param[kept_i]),
	     &(ps->param[i]),(size_t)sizeof(ps->param[0]));
      kept_i++;
    } else if (IS_CHEMBOND(cftype)) {
      srenew(plist[F_CONNBONDS].param,plist[F_CONNBONDS].nr+1);
      memcpy(&(plist[F_CONNBONDS].param[plist[F_CONNBONDS].nr]),
	     &(ps->param[i]),(size_t)sizeof(plist[F_CONNBONDS].param[0]));
      plist[F_CONNBONDS].nr++;
      nconverted++;
    } else
      nremoved++;
    if(debug)fprintf(debug,"\n");
  }
  
  if (nremoved)
    fprintf(stderr,"Removed   %4d %15ss with dummy atoms, %5d left\n",
	    nremoved, interaction_function[cftype].longname, kept_i);
  if (nconverted)
    fprintf(stderr,"Converted %4d %15ss with dummy atoms to connections, %5d left\n",
	    nconverted, interaction_function[cftype].longname, kept_i);
  if (nOut)
    fprintf(stderr,"Warning: removed %d %ss with dummy with %s construction\n"
	    "         This dummy construction does not guarantee constant "
	    "bond-length\n"
	    "         If the constructions were generated by pdb2gmx ignore "
	    "this warning\n",
	    nOut, interaction_function[cftype].longname, 
	    interaction_function[F_DUMMY3OUT].longname );
  ps->nr=kept_i;
}

static void clean_dum_angles(t_params *plist, t_pindex pindex[], 
			     int cftype, int dummy_type[])
{
  int      ftype,i,j,parnr,k,l,m,n,ndum,kept_i,dumnral,dumtype;
  atom_id  atom,constr,at1,at2;
  atom_id  dumatoms[MAXATOMLIST];
  bool     bKeep,bUsed,bPresent,bAll3FAD,bFirstTwo;
  t_params *ps;
  
  ps = &(plist[cftype]);
  dumnral=0;
  kept_i=0;
  for(i=0; (i<ps->nr); i++) { /* for all angles in the plist */
    bKeep=FALSE;
    bAll3FAD=TRUE;
    /* check if all dummies are constructed from the same atoms */
    ndum=0;
    for(k=0; (k<3) && !bKeep; k++) { /* for all atoms in the angle */
      atom = ps->param[i].a[k];
      if (dummy_type[atom]!=NOTSET) {
	ndum++;
	bAll3FAD = bAll3FAD && (pindex[atom].ftype == F_DUMMY3FAD);
	if (ndum==1) {
	  /* store construction atoms of first dummy */
	  dumnral=NRAL(pindex[atom].ftype)-1;
	  for(m=0; (m<dumnral); m++)
	    dumatoms[m]=
	      plist[pindex[atom].ftype].param[pindex[atom].parnr].a[m+1];
	} else 
	  /* check if this dummy is constructed from the same atoms */
	  if (dumnral == NRAL(pindex[atom].ftype)-1 )
	    for(m=0; (m<dumnral) && !bKeep; m++) {
	      bPresent=FALSE;
	      constr=
		plist[pindex[atom].ftype].param[pindex[atom].parnr].a[m+1];
	      for(n=0; (n<dumnral) && !bPresent; n++)
		if (constr == dumatoms[n])
		  bPresent=TRUE;
	      if (!bPresent)
		bKeep=TRUE;
	    }
	  else
	    bKeep=TRUE;
      }
    }
    
    /* keep all angles with no dummies in them or 
       with dummies with more than 3 constr. atoms */
    if ( ndum == 0 && dumnral > 3 )
      bKeep=TRUE;
    
    /* check if all non-dummy atoms are used in construction: */
    bFirstTwo=TRUE;
    for(k=0; (k<3) && !bKeep; k++) { /* for all atoms in the angle */
      atom = ps->param[i].a[k];
      if (dummy_type[atom]==NOTSET) {
	bUsed=FALSE;
	for(m=0; (m<dumnral) && !bUsed; m++)
	  if (atom == dumatoms[m]) {
	    bUsed=TRUE;
	    bFirstTwo = bFirstTwo && m<2;
	  }
	if (!bUsed)
	  bKeep=TRUE;
      }
    }
    
    if ( ! ( bAll3FAD && bFirstTwo ) )
      /* check if all constructing atoms are constrained together */
      for (m=0; m<dumnral && !bKeep; m++) { /* all constr. atoms */
	at1 = dumatoms[m];
	at2 = dumatoms[(m+1) % dumnral];
	bPresent=FALSE;
	for (ftype=0; ftype<F_NRE; ftype++)
	  if ( interaction_function[ftype].flags & IF_CONSTRAINT )
	    for (j=0; (j<plist[ftype].nr) && !bPresent; j++)
	      /* all constraints until one matches */
	      bPresent = ( ( (plist[ftype].param[j].AI == at1) &&
			     (plist[ftype].param[j].AJ == at2) ) || 
			   ( (plist[ftype].param[j].AI == at2) &&
			     (plist[ftype].param[j].AJ == at1) ) );
	if (!bPresent)
	  bKeep=TRUE;
      }
    
    if ( bKeep ) {
      /* now copy the angle to the new array */
      memcpy(&(ps->param[kept_i]),
	     &(ps->param[i]),(size_t)sizeof(ps->param[0]));
      kept_i++;
    }
  }
  
  if (ps->nr != kept_i)
    fprintf(stderr,"Removed   %4d %15ss with dummy atoms, %5d left\n",
	    ps->nr-kept_i, interaction_function[cftype].longname, kept_i);
  ps->nr=kept_i;
}

static void clean_dum_dihs(t_params *plist, t_pindex pindex[], 
			   int cftype, int dummy_type[])
{
  int      ftype,i,parnr,k,l,m,n,ndum,kept_i,dumnral;
  atom_id  atom,constr;
  atom_id  dumatoms[3];
  bool     bKeep,bUsed,bPresent;
  t_params *ps;
  
  ps = &(plist[cftype]);
  
  dumnral=0;
  kept_i=0;
  for(i=0; (i<ps->nr); i++) { /* for all dihedrals in the plist */
    bKeep=FALSE;
    /* check if all dummies are constructed from the same atoms */
    ndum=0;
    for(k=0; (k<4) && !bKeep; k++) { /* for all atoms in the dihedral */
      atom = ps->param[i].a[k];
      if (dummy_type[atom]!=NOTSET) {
	ndum++;
	if (ndum==1) {
	  /* store construction atoms of first dummy */
	  dumnral=NRAL(pindex[atom].ftype)-1;
	  for(m=0; (m<dumnral); m++)
	    dumatoms[m]=
	      plist[pindex[atom].ftype].param[pindex[atom].parnr].a[m+1];
	  if (debug) {
	    fprintf(debug,"dih w. dum: %u %u %u %u\n",
		    ps->param[i].AI+1,ps->param[i].AJ+1,
		    ps->param[i].AK+1,ps->param[i].AL+1);
	    fprintf(debug,"dum %u from: %u %u %u\n",
		    atom+1,dumatoms[0]+1,dumatoms[1]+1,dumatoms[2]+1);
	  }
	} else 
	  /* check if this dummy is constructed from the same atoms */
	  if (dumnral == NRAL(pindex[atom].ftype)-1 )
	    for(m=0; (m<dumnral) && !bKeep; m++) {
	      bPresent=FALSE;
	      constr=
		plist[pindex[atom].ftype].param[pindex[atom].parnr].a[m+1];
	      for(n=0; (n<dumnral) && !bPresent; n++)
		if (constr == dumatoms[n])
		  bPresent=TRUE;
	      if (!bPresent)
		bKeep=TRUE;
	    }
      }
    }
    
    /* keep all dihedrals with no dummies in them */
    if (ndum==0)
      bKeep=TRUE;
    
    /* check if all atoms in dihedral are either dummies, or used in 
       construction of dummies. If so, keep it, if not throw away: */
    for(k=0; (k<4) && !bKeep; k++) { /* for all atoms in the dihedral */
      atom = ps->param[i].a[k];
      if (dummy_type[atom]==NOTSET) {
	bUsed=FALSE;
	for(m=0; (m<dumnral) && !bUsed; m++)
	  if (atom == dumatoms[m])
	    bUsed=TRUE;
	if (!bUsed) {
	  bKeep=TRUE;
	  if (debug) fprintf(debug,"unused atom in dih: %u\n",atom+1);
	}
      }
    }
      
    if ( bKeep ) {
      memcpy(&(ps->param[kept_i]),
	     &(ps->param[i]),(size_t)sizeof(ps->param[0]));
      kept_i++;
    }
  }

  if (ps->nr != kept_i)
    fprintf(stderr,"Removed   %4d %15ss with dummy atoms, %5d left\n", 
	    ps->nr-kept_i, interaction_function[cftype].longname, kept_i);
  ps->nr=kept_i;
}

void clean_dum_bondeds(t_params *plist, int natoms, bool bRmDumBds)
{
  int i,k,ndum,ftype,parnr;
  int *dummy_type;
  t_pindex *pindex;

  pindex=0; /* avoid warnings */
  /* make dummy_type array */
  snew(dummy_type,natoms);
  for(i=0; i<natoms; i++)
    dummy_type[i]=NOTSET;
  ndum=0;
  for(ftype=0; ftype<F_NRE; ftype++)
    if (interaction_function[ftype].flags & IF_DUMMY) {
      ndum+=plist[ftype].nr;
      for(i=0; i<plist[ftype].nr; i++)
	if ( dummy_type[plist[ftype].param[i].AI] == NOTSET)
	  dummy_type[plist[ftype].param[i].AI]=ftype;
	else
	  fatal_error(0,"multiple dummy constructions for atom %d",
		      plist[ftype].param[i].AI+1);
    }
  
  /* the rest only if we have dummies: */
  if (ndum) {
    fprintf(stderr,"Cleaning up constraints %swith dummy particles\n",
	    bRmDumBds?"and constant bonded interactions ":"");
    snew(pindex,natoms);
    for(ftype=0; ftype<F_NRE; ftype++)
      if (interaction_function[ftype].flags & IF_DUMMY)
	for (parnr=0; (parnr<plist[ftype].nr); parnr++) {
	  k=plist[ftype].param[parnr].AI;
	  pindex[k].ftype=ftype;
	  pindex[k].parnr=parnr;
	}
    
    if (debug)
      for(i=0; i<natoms; i++)
	fprintf(debug,"atom %d dummy_type %s\n",i, 
		dummy_type[i]==NOTSET ? "NOTSET" : 
		interaction_function[dummy_type[i]].name);
    
    /* remove things with dummy atoms */
    for(ftype=0; ftype<F_NRE; ftype++)
      if ( ( ( interaction_function[ftype].flags & IF_BOND ) && bRmDumBds ) ||
	   ( interaction_function[ftype].flags & IF_CONSTRAINT ) ) {
	if (interaction_function[ftype].flags & (IF_BTYPE | IF_CONSTRAINT) )
	  clean_dum_bonds (plist, pindex, ftype, dummy_type);
	else if (interaction_function[ftype].flags & IF_ATYPE)
	  clean_dum_angles(plist, pindex, ftype, dummy_type);
	else if ( (ftype==F_PDIHS) || (ftype==F_IDIHS) )
	  clean_dum_dihs  (plist, pindex, ftype, dummy_type);
      }
    /* check if we have constraints left with dummy atoms in them */
    for(ftype=0; ftype<F_NRE; ftype++)
      if (interaction_function[ftype].flags & IF_CONSTRAINT)
	check_dum_constraints(plist, ftype, dummy_type);
    
  }
  sfree(pindex);
  sfree(dummy_type);
}
