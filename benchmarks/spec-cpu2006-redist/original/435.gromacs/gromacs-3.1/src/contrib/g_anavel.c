/*
 * $Id: g_anavel.c,v 1.4 2002/02/28 11:14:17 spoel Exp $
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
static char *SRCID_g_anavel_c = "$Id: g_anavel.c,v 1.4 2002/02/28 11:14:17 spoel Exp $";
#include "sysstuff.h"
#include "smalloc.h"
#include "macros.h"
#include "statutil.h"
#include "random.h"
#include "names.h"
#include "matio.h"
#include "physics.h"
#include "vec.h"
#include "futil.h"
#include "copyrite.h"
#include "xvgr.h"
#include "string2.h"
#include "rdgroup.h"
#include "tpxio.h"

int main(int argc,char *argv[])
{
  static char *desc[] = {
    "g_anavel computes temperature profiles in a sample. The sample",
    "can be analysed radial, i.e. the temperature as a function of",
    "distance from the center, cylindrical, i.e. as a function of distance",
    "from the vector (0,0,1) through the center of the box, or otherwise",
    "(will be specified later)"
  };
  t_filenm fnm[] = {
    { efTRN,  "-f",  NULL, ffREAD },
    { efTPX,  "-s",  NULL, ffREAD },
    { efXPM,  "-o", "xcm", ffWRITE }
  };
#define NFILE asize(fnm)

  static int  mode = 0,   nlevels = 10;
  static real tmax = 300, xmax    = -1;
  t_pargs pa[] = {
    { "-mode",    FALSE, etINT,  {&mode},    "mode" },
    { "-nlevels", FALSE, etINT,  {&nlevels}, "number of levels" },
    { "-tmax",    FALSE, etREAL, {&tmax},    "max temperature in output" },
    { "-xmax",    FALSE, etREAL, {&xmax},    "max distance from center" }
  };
  
  FILE       *fp;
  int        *npts,nmax;
  int        status;
  int        i,j,idum,step,nframe=0,natoms,index;
  real       t,temp,rdum,hboxx,hboxy,scale,xnorm;
  real       **profile=NULL;
  real       *t_x=NULL,*t_y,hi=0;
  rvec       *x,*v;
  t_topology *top;
  int        d,m,n;
  matrix     box;
  atom_id    *sysindex;
  bool       bHaveV,bReadV;
  t_rgb      rgblo = { 0, 0, 1 },rgbhi = { 1, 0, 0 };
  
  
  CopyRight(stderr,argv[0]);
  parse_common_args(&argc,argv,PCA_CAN_TIME | PCA_BE_NICE ,NFILE,fnm,
		    asize(pa),pa,asize(desc),desc,0,NULL);

  top    = read_top(ftp2fn(efTPX,NFILE,fnm));
		    
  natoms = read_first_x_v(&status,ftp2fn(efTRN,NFILE,fnm),&t,&x,&v,box);
  
  if (xmax > 0) {
    scale  = 5;
    nmax   = xmax*scale;
  }
  else {
    scale  = 5;
    nmax   = (0.5*sqrt(sqr(box[XX][XX])+sqr(box[YY][YY])))*scale; 
  }
  snew(npts,nmax+1);
  snew(t_y,nmax+1);
  for(i=0; (i<=nmax); i++) {
    npts[i] = 0;
    t_y[i]  = i/scale;
  }
  do {
    srenew(profile,++nframe);
    snew(profile[nframe-1],nmax+1);
    srenew(t_x,nframe);
    t_x[nframe-1] = t*1000;
    hboxx = box[XX][XX]/2;
    hboxy = box[YY][YY]/2;
    for(i=0; (i<natoms); i++) {
      /* determine position dependent on mode */
      switch (mode) {
      case 0:
	xnorm = sqrt(sqr(x[i][XX]-hboxx) + sqr(x[i][YY]-hboxy));
	break;
      default:
	fatal_error(0,"Unknown mode %d",mode);
      }
      index = xnorm*scale;
      if (index <= nmax) {
	temp = top->atoms.atom[i].m*iprod(v[i],v[i])/(2*BOLTZ);
	if (temp > hi)
	  hi = temp;
	npts[index]++;
	profile[nframe-1][index] += temp;
      }
    }
    for(i=0; (i<=nmax); i++) {
      if (npts[i] != 0) 
	profile[nframe-1][i] /= npts[i];
      npts[i] = 0;
    }
  } while (read_next_x_v(status,&t,natoms,x,v,box));
  close_trn(status);

  fp = ftp2FILE(efXPM,NFILE,fnm,"w");
  write_xpm(fp,"Temp. profile","T (a.u.)",
	    "t (fs)","R (nm)",
	    nframe,nmax+1,t_x,t_y,profile,0,tmax,
	    rgblo,rgbhi,&nlevels);
  
  thanx(stderr);
  
  return 0;
}

