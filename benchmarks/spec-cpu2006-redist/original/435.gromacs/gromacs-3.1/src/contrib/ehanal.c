/*
 * $Id: ehanal.c,v 1.4 2002/02/28 11:14:16 spoel Exp $
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
 * Great Red Owns Many ACres of Sand 
 */
static char *SRCID_ehanal_c = "$Id: ehanal.c,v 1.4 2002/02/28 11:14:16 spoel Exp $";
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "typedefs.h"
#include "smalloc.h"
#include "macros.h"
#include "copyrite.h"
#include "statutil.h"
#include "fatal.h"
#include "random.h"
#include "pdbio.h"
#include "futil.h"
#include "physics.h"
#include "xvgr.h"
#include "vec.h"
#include "names.h"
#include "ehdata.h"

t_histo *init_histo(int np,real minx,real maxx)
{
  t_histo *h;
  
  snew(h,1);
  snew(h->y,np+1);
  snew(h->nh,np+1);
  h->np   = np;
  if (maxx <= minx)
    fatal_error(0,"minx (%f) should be less than maxx (%f) in init_histo",minx,maxx);
  h->minx = minx;
  h->maxx = maxx;
  h->dx_1 = np/(maxx-minx);
  h->dx   = (maxx-minx)/np;
  
  return h;
}

void done_histo(t_histo *h)
{
  sfree(h->y);
  sfree(h->nh);
  h->np = 0;
}

void add_histo(t_histo *h,real x,real y)
{
  int n;
  
  n = (x-h->minx)*h->dx_1;
  if ((n < 0) || (n > h->np)) 
    fatal_error(0,"Invalid x (%f) in add_histo. SHould be in %f - %f",x,h->minx,h->maxx);
  h->y[n] += y;
  h->nh[n]++;
}

void dump_histo(t_histo *h,char *fn,char *title,char *xaxis,char *yaxis,
		int enorm,real norm_fac)
{
  FILE *fp;
  int  i,nn;
  
  for(nn=h->np; (nn > 0); nn--)
    if (h->nh[nn] != 0) 
      break;
  for(i=0; (i<nn); i++)
    if (h->nh[i] > 0)
      break;
  fp = xvgropen(fn,title,xaxis,yaxis);
  for(  ; (i<nn); i++) {
    switch (enorm) {
    case enormNO:
      fprintf(fp,"%12f  %12f  %d\n",h->minx+h->dx*i,h->y[i],h->nh[i]);
      break;
    case enormFAC:
      fprintf(fp,"%12f  %12f  %d\n",h->minx+h->dx*i,h->y[i]*norm_fac,h->nh[i]);
      break;
    case enormNP:
      if (h->nh[i] > 0)
	fprintf(fp,"%12f  %12f  %d\n",h->minx+h->dx*i,h->y[i]*norm_fac/h->nh[i],h->nh[i]);
      break;
    default:
      fatal_error(0,"Wrong value for enorm (%d)",enorm);
    }
  }
  fclose(fp);
}

/*******************************************************************
 *
 * Functions to analyse and monitor scattering
 *
 *******************************************************************/	

void add_scatter_event(t_ana_scat *scatter,rvec pos,bool bInel,
		       real t,real ekin)
{
  int np = scatter->np;
  
  if (np == scatter->maxp) {
    scatter->maxp += 32;
    srenew(scatter->time,scatter->maxp);
    srenew(scatter->ekin,scatter->maxp);
    srenew(scatter->bInel,scatter->maxp);
    srenew(scatter->pos,scatter->maxp);
  }
  scatter->time[np]  = t;
  scatter->bInel[np] = np;
  scatter->ekin[np]  = ekin;
  copy_rvec(pos,scatter->pos[np]);
  scatter->np++;
}

void reset_ana_scat(t_ana_scat *scatter)
{
  scatter->np = 0;
}

void done_scatter(t_ana_scat *scatter)
{
  scatter->np = 0;
  sfree(scatter->time);
  sfree(scatter->ekin);
  sfree(scatter->bInel);
  sfree(scatter->pos);
}

void analyse_scatter(t_ana_scat *scatter,t_histo *hmfp)
{
  int   i,n;
  rvec  dx;
  
  if (scatter->np > 1) {
    for(i=1; (i<scatter->np); i++) {
      rvec_sub(scatter->pos[i],scatter->pos[i-1],dx);
      add_histo(hmfp,scatter->ekin[i],norm(dx));
    }
  }
}

/*******************************************************************
 *
 * Functions to analyse structural changes
 *
 *******************************************************************/	

t_ana_struct *init_ana_struct(int nstep,int nsave,real timestep)
{
  t_ana_struct *anal;
  
  snew(anal,1);
  anal->nanal = (nstep / nsave)+1;
  anal->index = 0;
  anal->dt    = nsave*timestep;
  snew(anal->t,anal->nanal);
  snew(anal->maxdist,anal->nanal);
  snew(anal->averdist,anal->nanal);
  snew(anal->ad2,anal->nanal);
  snew(anal->nion,anal->nanal);
  
  return anal;
}

void done_ana_struct(t_ana_struct *anal)
{
  sfree(anal->t);
  sfree(anal->maxdist);
  sfree(anal->averdist);
  sfree(anal->ad2);
  sfree(anal->nion);
}

void reset_ana_struct(t_ana_struct *anal)
{
  int i;
  
  for(i=0; (i<anal->nanal); i++) {
    anal->t[i] = 0;
    anal->maxdist[i] = 0;
    anal->averdist[i] = 0;
    anal->ad2[i] = 0;
    anal->nion[i] = 0;
  }
  anal->index = 0;
}

void add_ana_struct(t_ana_struct *total,t_ana_struct *add)
{
  int i;
  
  if (total->index == 0)
    total->index = add->index;
  else if (total->index != add->index)
    fatal_error(0,"Analysis incompatible %s, %d",__FILE__,__LINE__);
  for(i=0; (i<total->index); i++) {
    if (total->t[i] == 0)
      total->t[i] = add->t[i];
    else if (total->t[i] != add->t[i])
      fatal_error(0,"Inconsistent times in analysis (%f-%f) %s, %d",
		  total->t[i],add->t[i],__FILE__,__LINE__);
    total->maxdist[i]  += add->maxdist[i];
    total->averdist[i] += add->averdist[i];
    total->ad2[i]      += add->ad2[i];
    total->nion[i]     += add->nion[i];
  }
}

void analyse_structure(t_ana_struct *anal,real t,rvec center,
		       rvec x[],int nparticle,real charge[])
{
  int  i,j,n=0;
  rvec dx;
  real dx2,dx1;
  
  j = anal->index;
  anal->t[j]       = t;
  anal->maxdist[j] = 0;
  for(i=0; (i<nparticle); i++) {
    if (charge[i] < 0) {
      rvec_sub(x[i],center,dx);
      dx2 = iprod(dx,dx);
      dx1 = sqrt(dx2);
      anal->ad2[j] += dx2;
      anal->averdist[j]  += dx1;
      if (dx1 > anal->maxdist[j])
	anal->maxdist[j] = dx1;
      n++;
    }
  }
  anal->nion[j] = n;
  anal->index++;
}

void dump_ana_struct(char *rmax,char *nion,char *gyr,
		     t_ana_struct *anal,int nsim)
{
  FILE *fp,*gp,*hp;
  int  i;
  real t;
  
  fp = xvgropen(rmax,"rmax","Time (fs)","r (nm)");
  gp = xvgropen(nion,"N ion","Time (fs)","N ions");
  hp = xvgropen(gyr,"Radius of gyration","Time (fs)","Rg (nm)");
  for(i=0; (i<anal->index); i++) {
    t = 1000*anal->t[i];
    fprintf(fp,"%12g  %12.3f\n",t,anal->maxdist[i]/nsim);
    fprintf(gp,"%12g  %12.3f\n",t,(1.0*anal->nion[i])/nsim-1);
    if (anal->nion[i] > 0)
      fprintf(hp,"%12g  %12.3f\n",t,sqrt(anal->ad2[i]/anal->nion[i]));
  }
  fclose(hp);
  fclose(gp);
  fclose(fp);
}

char *enms[eNR] = {
  "Coulomb", "Repulsion", "Potential",
  "EkHole",  "EkElectron", "EkLattice", "Kinetic",
  "Total"
};

void add_ana_ener(t_ana_ener *ae,int nn,real e[])
{
  int i;
 
  /* First time around we are constantly increasing the array size */ 
  if (nn >= ae->nx) {
    if (ae->nx == ae->maxx) {
      ae->maxx += 1024;
      srenew(ae->e,ae->maxx);
    }
    for(i=0; (i<eNR); i++)
      ae->e[ae->nx][i] = e[i];
    ae->nx++;
  }
  else {
    for(i=0; (i<eNR); i++)
      ae->e[nn][i] += e[i];
  }
}

void dump_ana_ener(t_ana_ener *ae,int nsim,real dt,char *edump,
		   t_ana_struct *total)
{
  FILE *fp;
  int  i,j;
  real fac;
  
  fac = 1.0/(nsim*ELECTRONVOLT);
  fp=xvgropen(edump,"Energies","Time (fs)","E (eV)");
  xvgr_legend(fp,eNR,enms);
  fprintf(fp,"@ s%d legend \"Ek/Nelec\"\n",eNR);
  fprintf(fp,"@ type nxy\n");
  for(i=0; (i<ae->nx); i++) {
    fprintf(fp,"%10f",1000.0*dt*i);
    for(j=0; (j<eNR); j++)
      fprintf(fp,"  %8.3f",ae->e[i][j]*fac);
    fprintf(fp,"  %8.3f\n",ae->e[i][eELECTRON]/(ELECTRONVOLT*total->nion[i]));
  }    
  fprintf(fp,"&\n");
  fclose(fp);
}

