/*
 * $Id: g_sorient.c,v 1.8 2002/02/28 11:00:27 spoel Exp $
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
 * GROtesk MACabre and Sinister
 */
static char *SRCID_g_sorient_c = "$Id: g_sorient.c,v 1.8 2002/02/28 11:00:27 spoel Exp $";
#include "macros.h"
#include "statutil.h"
#include "smalloc.h"
#include "copyrite.h"
#include "gstat.h"
#include "vec.h"
#include "xvgr.h"
#include "pbc.h"
#include "rdgroup.h"
#include "tpxio.h"

int main(int argc,char *argv[])
{
  t_topology top;
  char     title[STRLEN];
  int      status;
  int      natoms;
  real     t;
  rvec     *xtop,*x;
  matrix   box;
  
  FILE    *fp;
  int     i,j,p,sa0,sa1,sa2,n,ntot,nf,m,*hist1,*hist2,*histn,nrbin;
  real    *histi1,*histi2,rbinw,invbw;
  int     *isize,nrefgrp,nrefat;
  atom_id **index;
  char    **grpname;
  real    inp,outp,two_pi,nav,normfac,rmin2,rmax2,rcut,rcut2,r2,r,mass,mtot;
  real    c1,c2;
  char    str[STRLEN];
  rvec    xref,dx,dxh1,dxh2,outer;
  char    *legr[] = { "<cos(\\8q\\4\\s1\\N)>", 
		      "<3cos\\S2\\N(\\8q\\4\\s2\\N)-1>" };
  char    *legc[] = { "cos(\\8q\\4\\s1\\N)", 
		      "3cos\\S2\\N(\\8q\\4\\s2\\N)-1" };
  
  static char *desc[] = {
    "g_sorient analyzes solvent orientation around solutes.", 
    "It calculates two angles between the vector from one or more",
    "reference positions to the first atom of each solvent molecule:[BR]"
    "theta1: the angle with the vector from the first atom of the solvent",
    "molecule to the midpoint between atoms 2 and 3.[BR]",
    "theta2: the angle with the normal of the solvent plane, defined by the",
    "same three atoms.[BR]",
    "The reference can be a set of atoms or",
    "the center of mass of a set of atoms. The group of solvent atoms should",
    "consist of 3 atoms per solvent molecule.",
    "Only solvent molecules between [TT]-rmin[tt] and [TT]-rmax[tt] are",
    "considered each frame.[PAR]",
    "[TT]-o[tt]: angle distribution of theta1.[PAR]",
    "[TT]-no[tt]: angle distribution of theta2.[PAR]",
    "[TT]-ro[tt]: <cos(theta1)> and <3cos^2(theta2)-1> as a function of the",
    "distance.[PAR]",
    "[TT]-ro[tt]: the sum over all solvent molecules within distance r",
    "of cos(theta1) and 3cos^2(theta2)-1 as a function of r.[PAR]"
  };
  
  static bool bCom = FALSE;
  static int nbin=20;
  static real rmin=0.0,rmax=0.5;
  t_pargs pa[] = {
    { "-com",  FALSE, etBOOL,  {&bCom},
      "Use the center of mass as the reference postion" },
    { "-rmin",  FALSE, etREAL, {&rmin}, "Minimum distance" },
    { "-rmax",  FALSE, etREAL, {&rmax}, "Maximum distance" },
    { "-nbin",  FALSE, etINT,  {&nbin}, "Number of bins" }
  };
  
  t_filenm fnm[] = {
    { efTRX, NULL,  NULL,  ffREAD },
    { efTPS, NULL,  NULL,  ffREAD },
    { efNDX, NULL,  NULL,  ffOPTRD },
    { efXVG, NULL,  "sori.xvg",  ffWRITE },
    { efXVG, "-no", "snor.xvg",  ffWRITE },
    { efXVG, "-ro", "sord.xvg",  ffWRITE },
    { efXVG, "-co", "scum.xvg",  ffWRITE }
  };
#define NFILE asize(fnm)

  CopyRight(stderr,argv[0]);
  parse_common_args(&argc,argv,PCA_CAN_TIME | PCA_CAN_VIEW | PCA_BE_NICE,
		    NFILE,fnm,asize(pa),pa,asize(desc),desc,0,NULL);
  
  two_pi = 2/M_PI;

  read_tps_conf(ftp2fn(efTPS,NFILE,fnm),title,&top,&xtop,NULL,box,bCom);

  /* get index groups */
  printf("Select a group of reference particles and a solvent group:\n"); 
  snew(grpname,2);
  snew(index,2);
  snew(isize,2);
  get_index(&top.atoms,ftp2fn_null(efNDX,NFILE,fnm),2,isize,index,grpname);

  if (bCom) {
    nrefgrp = 1;
    nrefat  = isize[0];
  } else {
    nrefgrp = isize[0];
    nrefat  = 1;
  }

  if (isize[1] % 3)
    fatal_error(0,"The number of solvent atoms (%d) is not a multiple of 3",
		isize[1]);

  /* initialize reading trajectory:                         */
  natoms=read_first_x(&status,ftp2fn(efTRX,NFILE,fnm),&t,&x,box);

  rmin2 = sqr(rmin);
  rmax2 = sqr(rmax);
  rcut  = 0.45*min(box[XX][XX],min(box[YY][YY],box[ZZ][ZZ]));
  rcut2 = sqr(rcut);
  rbinw = 0.02;
  invbw = 1/rbinw;
  
  ntot = 0;
  nf = 0;
  
  snew(hist1,nbin);
  snew(hist2,nbin);
  nrbin = rcut/rbinw+1;
  snew(histi1,nrbin);
  snew(histi2,nrbin);
  snew(histn,nrbin);

  /* start analysis of trajectory */
  do {
    /* make molecules whole again */
    rm_pbc(&top.idef,natoms,box,x,x);
    
    init_pbc(box);
    n = 0;
    inp = 0;
    outp = 0;
    for(p=0; p<nrefgrp; p++) {
      clear_rvec(xref);
      mtot = 0;
      for(m=0; m<nrefat; m++) {
	mass = top.atoms.atom[index[0][p]].m;
	for(j=0; j<DIM; j++)
	  xref[j] += mass*x[index[0][p]][j];
	mtot += mass;
      }
      svmul(1/mtot,xref,xref);
      for(m=0; m<isize[1]; m+=3) {
	sa0 = index[1][m];
	sa1 = index[1][m+1];
	sa2 = index[1][m+2];
	pbc_dx(x[sa0],xref,dx);
	r2  = norm2(dx);
	if (r2 < rcut2) {
	  r = sqrt(r2);
	  rvec_sub(x[sa1],x[sa0],dxh1);
	  rvec_sub(x[sa2],x[sa0],dxh2);
	  rvec_inc(dxh1,dxh2);
	  svmul(1/r,dx,dx);
	  unitv(dxh1,dxh1);
	  inp = iprod(dx,dxh1);
	  oprod(dxh1,dxh2,outer);
	  unitv(outer,outer);
	  outp = iprod(dx,outer);
	  (histi1[(int)(invbw*r)])+=inp;
	  (histi2[(int)(invbw*r)])+=3*sqr(outp)-1;
	  (histn[(int)(invbw*r)])++;
	  if (r2>=rmin2 && r2<rmax2) {
	    n++;
	    (hist1[(int)(nbin*0.5*two_pi*acos(inp)+1)])++;
	    (hist2[(int)(nbin*two_pi*acos(fabs(outp)))])++;
	  }
	}
      }
    }
    ntot += n;
    nf++;

  }  while (read_next_x(status,&t,natoms,x,box));

  /* clean up */
  sfree(x);
  close_trj(status);
  
  nav     = (real)ntot/(nrefgrp*nf);
  normfac = nbin/(ntot*M_PI/2);
  
  fprintf(stderr,"Average number of molecules between %g and %g nm is %.1f\n",
	  rmin,rmax,nav);
  
  sprintf(str,"Solvent orientation between %g and %g nm",rmin,rmax);
  fp=xvgropen(opt2fn("-o",NFILE,fnm), 
	      str,"Angle","Number"); 
  fprintf(fp,"@ subtitle \"average shell size %.1f molecules\"\n",nav);
  for(i=0; i<nbin; i++) {
    r = 90*2*(i+0.5)/nbin;
    fprintf(fp,"%g %g %g\n",r,normfac*hist1[i],sin(M_PI*r/180));
  }
  fclose(fp);
  
  sprintf(str,"Solvent normal orientation between %g and %g nm",rmin,rmax);
  fp=xvgropen(opt2fn("-no",NFILE,fnm), 
	      str,"Angle","Number");
  fprintf(fp,"@ subtitle \"average shell size %.1f molecules\"\n",nav);
  for(i=0; i<nbin; i++) {
    r = 90*(i+0.5)/nbin;
    fprintf(fp,"%g %g %g\n",r,normfac*hist2[i],sin(M_PI*r/180));
  }
  fclose(fp);

  
  sprintf(str,"Solvent orientation");
  fp=xvgropen(opt2fn("-ro",NFILE,fnm),str,"r (nm)","");
  fprintf(fp,"@ subtitle \"as a function of distance\"\n");
  xvgr_legend(fp,2,legr);
  for(i=0; i<nrbin; i++)
    fprintf(fp,"%g %g %g\n",(i+0.5)*rbinw,
	    histn[i] ? histi1[i]/histn[i] : 0,
	    histn[i] ? histi2[i]/histn[i] : 0);
  fclose(fp);
  
  sprintf(str,"Cumulative solvent orientation");
  fp=xvgropen(opt2fn("-co",NFILE,fnm),str,"r (nm)","");
  fprintf(fp,"@ subtitle \"as a function of distance\"\n");
  xvgr_legend(fp,2,legc);
  normfac = 1.0/(nrefgrp*nf);
  c1 = 0;
  c2 = 0;
  for(i=0; i<nrbin; i++) {
    fprintf(fp,"%g %g %g\n",i*rbinw,c1,c2);
    c1 += histi1[i]*normfac;
    c2 += histi2[i]*normfac;
  }
  fclose(fp);

  do_view(opt2fn("-o",NFILE,fnm),NULL);
  do_view(opt2fn("-no",NFILE,fnm),NULL);
  do_view(opt2fn("-ro",NFILE,fnm),NULL);
  do_view(opt2fn("-co",NFILE,fnm),NULL);

  thanx(stderr);
  
  return 0;
}
