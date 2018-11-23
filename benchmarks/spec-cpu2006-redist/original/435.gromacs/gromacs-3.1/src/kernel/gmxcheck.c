/*
 * $Id: gmxcheck.c,v 1.62 2002/02/28 10:54:42 spoel Exp $
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
static char *SRCID_gmxcheck_c = "$Id: gmxcheck.c,v 1.62 2002/02/28 10:54:42 spoel Exp $";
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "main.h"
#include "macros.h"
#include "math.h"
#include "futil.h"
#include "statutil.h"
#include "copyrite.h"
#include "sysstuff.h"
#include "txtdump.h"
#include "fatal.h"
#include "gmxfio.h"
#include "trnio.h"
#include "xtcio.h"
#include "tpbcmp.h"
#include "atomprop.h"
#include "vec.h"
#include "pbc.h"
#include "physics.h"
#include "smalloc.h"
#include "confio.h"
#include "enxio.h"

typedef struct {
  int bStep;
  int bTime;
  int bLambda;
  int bX;
  int bV;
  int bF;
  int bBox;
} t_count;

void chk_trj(char *fn)
{
  t_trxframe   fr;
  t_count      count;
  int          j=-1,new_natoms,natoms;
  off_t        fpos;
  real         rdum,t,tt,t0,old_t1,old_t2,prec;
  bool         bShowTimestep=TRUE,bOK,newline=FALSE;
  int          status;
  
  new_natoms = -1;
  natoms = -1;  
  t      = 0;
  t0     = NOTSET;
  
  printf("Checking file %s\n",fn);
  
  j      =  0;
  t      = -1;
  old_t2 = -2.0;
  old_t1 = -1.0;
  fpos   = 0;
  
  count.bStep = 0;
  count.bTime = 0;
  count.bLambda = 0;
  count.bX = 0;
  count.bV = 0;
  count.bF = 0;
  count.bBox = 0;

  read_first_frame(&status,fn,&fr,TRX_READ_X | TRX_READ_V | TRX_READ_F);

  do {
    if (j == 0) {
      fprintf(stderr,"\n# Atoms  %d\n",fr.natoms);
      if (fr.bPrec)
	fprintf(stderr,"Precision %g (nm)\n",1/fr.prec);
    }
    newline=TRUE;
    if ((natoms > 0) && (new_natoms != natoms)) {
      fprintf(stderr,"\nNumber of atoms at t=%g don't match (%d, %d)\n",
	      old_t1,natoms,new_natoms);
      newline=FALSE;
    }
    if (j>=2) {
      if ( fabs((fr.time-old_t1)-(old_t1-old_t2)) > 
	   0.1*(fabs(fr.time-old_t1)+fabs(old_t1-old_t2)) ) {
	bShowTimestep=FALSE;
	fprintf(stderr,"%sTimesteps at t=%g don't match (%g, %g)\n",
		newline?"\n":"",old_t1,old_t1-old_t2,fr.time-old_t1);
      }
    }
    natoms=new_natoms;
    old_t2=old_t1;
    old_t1=fr.time;
    if (t0 == NOTSET) t0=fr.time;
    if (fpos && (j<10 || j%10==0))
      fprintf(stderr," byte: %10lu",(unsigned long)fpos);
    j++;
    t=fr.time;
    new_natoms=fr.natoms;
#define INC(s,n,item) if (s.item != 0) n.item++
    INC(fr,count,bStep);
    INC(fr,count,bTime);
    INC(fr,count,bLambda);
    INC(fr,count,bX);
    INC(fr,count,bV);
    INC(fr,count,bF);
    INC(fr,count,bBox);
#undef INC
    fpos = fio_ftell(status);
  } while (read_next_frame(status,&fr));
  
  fprintf(stderr,"\n");

  close_trj(status);

  fprintf(stderr,"\nItem        #frames");
  if (bShowTimestep)
    fprintf(stderr," Timestep (ps)");
  fprintf(stderr,"\n");
#define PRINTITEM(label,item) fprintf(stderr,"%-10s  %6d",label,count.item); if ((bShowTimestep) && (count.item > 1)) fprintf(stderr,"    %g\n",(t-t0)/(count.item-1)); else fprintf(stderr,"\n")
  PRINTITEM ( "Step",       bStep );
  PRINTITEM ( "Time",       bTime );
  PRINTITEM ( "Lambda",     bLambda );
  PRINTITEM ( "Coords",     bX );
  PRINTITEM ( "Velocities", bV );
  PRINTITEM ( "Forces",     bF );
  PRINTITEM ( "Box",        bBox );
}  

void chk_tps(char *fn, real vdw_fac, real bon_lo, real bon_hi)
{
  int       natom,i,j,k;
  char      title[STRLEN];
  t_topology top;
  t_atoms   *atoms;
  rvec      *x,*v;
  rvec      dx;
  matrix    box;
  bool      bV,bX,bB,bFirst,bOut;
  real      r2,ekin,temp1,temp2,dist2,vdwfac2,bonlo2,bonhi2;
  real      *atom_vdw;
  
  fprintf(stderr,"Checking coordinate file %s\n",fn);
  read_tps_conf(fn,title,&top,&x,&v,box,TRUE);
  atoms=&top.atoms;
  natom=atoms->nr;
  fprintf(stderr,"%d atoms in file\n",atoms->nr);
  
  /* check coordinates and box */
  bV=FALSE;
  bX=FALSE;
  for (i=0; (i<natom) && !(bV && bX); i++)
    for (j=0; (j<DIM) && !(bV && bX); j++) {
      bV=bV || (v[i][j]!=0);
      bX=bX || (x[i][j]!=0);
    }
  bB=FALSE;
  for (i=0; (i<DIM) && !bB; i++)
    for (j=0; (j<DIM) && !bB; j++)
      bB=bB || (box[i][j]!=0);
  
  fprintf(stderr,"coordinates %s\n",bX?"found":"absent");
  fprintf(stderr,"box         %s\n",bB?"found":"absent");
  fprintf(stderr,"velocities  %s\n",bV?"found":"absent");
  fprintf(stderr,"\n");
  
  /* check velocities */
  if (bV) {
    ekin=0.0;
    for (i=0; (i<natom); i++)
      for (j=0; (j<DIM); j++)
	ekin+=0.5*atoms->atom[i].m*v[i][j]*v[i][j];
    temp1=(2.0*ekin)/(natom*DIM*BOLTZ); 
    temp2=(2.0*ekin)/(natom*(DIM-1)*BOLTZ); 
    fprintf(stderr,"Kinetic energy: %g (kJ/mol)\n",ekin);
    fprintf(stderr,"Assuming the number of degrees of freedom to be "
	    "Natoms * %d or Natoms * %d,\n"
	    "the velocities correspond to a temperature of the system\n"
	    "of %g K or %g K respectively.\n\n",DIM,DIM-1,temp1,temp2);
  }
  
  /* check coordinates */
  if (bX) {
    vdwfac2=sqr(vdw_fac);
    bonlo2=sqr(bon_lo);
    bonhi2=sqr(bon_hi);
   
    fprintf(stderr,
	    "Checking for atoms closer than %g and not between %g and %g,\n"
	    "relative to sum of Van der Waals distance:\n",
	    vdw_fac,bon_lo,bon_hi);
    snew(atom_vdw,natom);
    for (i=0; (i<natom); i++) {
      atom_vdw[i]=get_vdw(*(atoms->resname[atoms->atom[i].resnr]),
			  *(atoms->atomname[i]),0.1);
      if (debug) fprintf(debug,"%5d %4s %4s %7g\n",i+1,
			 *(atoms->resname[atoms->atom[i].resnr]),
			 *(atoms->atomname[i]),
			 atom_vdw[i]);
    }
    
    if (bB) 
      init_pbc(box);
      
    bFirst=TRUE;
    for (i=0; (i<natom); i++) {
      if (((i+1)%10)==0)
	fprintf(stderr,"\r%5d",i+1);
      for (j=i+1; (j<natom); j++) {
	if (bB)
	  pbc_dx(x[i],x[j],dx);
	else
	  rvec_sub(x[i],x[j],dx);
	r2=iprod(dx,dx);
	dist2=sqr(atom_vdw[i]+atom_vdw[j]);
	if ( (r2<=dist2*bonlo2) || 
	     ( (r2>=dist2*bonhi2) && (r2<=dist2*vdwfac2) ) ) {
	  if (bFirst) {
	    fprintf(stderr,"\r%5s %4s %8s %5s  %5s %4s %8s %5s  %6s\n",
		    "atom#","name","residue","r_vdw",
		    "atom#","name","residue","r_vdw","distance");
	    bFirst=FALSE;
	  }
	  fprintf(stderr,
		  "\r%5d %4s %4s%4d %-5.3g  %5d %4s %4s%4d %-5.3g  %-6.4g\n",
		  i+1,*(atoms->atomname[i]),
		  *(atoms->resname[atoms->atom[i].resnr]),atoms->atom[i].resnr+1,
		  atom_vdw[i],
		  j+1,*(atoms->atomname[j]),
		  *(atoms->resname[atoms->atom[j].resnr]),atoms->atom[j].resnr+1,
		  atom_vdw[j],
		  sqrt(r2) );
	}
      }
    }
    if (bFirst) 
      fprintf(stderr,"\rno close atoms found\n");
    fprintf(stderr,"\r      \n");
    
    if (bB) {
      /* check box */
      bFirst=TRUE;
      k=0;
      for (i=0; (i<natom) && (k<10); i++) {
	bOut=FALSE;
	for(j=0; (j<DIM) && !bOut; j++)
	  bOut = bOut || (x[i][j]<0) || (x[i][j]>box[j][j]);
	if (bOut) {
	  k++;
	  if (bFirst) {
	    fprintf(stderr,"Atoms outside box ( ");
	    for (j=0; (j<DIM); j++)
	      fprintf(stderr,"%g ",box[j][j]);
	    fprintf(stderr,"):\n"
		    "(These may occur often and are normally not a problem)\n"
		    "%5s %4s %8s %5s  %s\n",
		    "atom#","name","residue","r_vdw","coordinate");
	    bFirst=FALSE;
	  }
	  fprintf(stderr,
		  "%5d %4s %4s%4d %-5.3g",
		  i,*(atoms->atomname[i]),
		  *(atoms->resname[atoms->atom[i].resnr]),
		  atoms->atom[i].resnr,atom_vdw[i]);
	  for (j=0; (j<DIM); j++)
	    fprintf(stderr," %6.3g",x[i][j]);
	  fprintf(stderr,"\n");
	}
      }
      if (k==10)
	fprintf(stderr,"(maybe more)\n");
      if (bFirst) 
	fprintf(stderr,"no atoms found outside box\n");
      fprintf(stderr,"\n");
    }
  }
}

void chk_enx(char *fn)
{
  int        in,nre,fnr,ndr;
  char       **enm=NULL;
  t_enxframe *fr;
  bool       bShowTStep;
  real       t0,old_t1,old_t2;
  
  fprintf(stderr,"Checking energy file %s\n\n",fn);

  in = open_enx(fn,"r");
  do_enxnms(in,&nre,&enm);
  fprintf(stderr,"%d groups in energy file",nre);
  snew(fr,1);
  old_t2=-2.0;
  old_t1=-1.0;
  fnr=0;
  t0=NOTSET;
  bShowTStep=TRUE;

  while (do_enx(in,fr)) {
    if (fnr>=2) {
      if ( fabs((fr->t-old_t1)-(old_t1-old_t2)) > 
	   0.1*(fabs(fr->t-old_t1)+fabs(old_t1-old_t2)) ) {
	bShowTStep=FALSE;
	fprintf(stderr,"\nTimesteps at t=%g don't match (%g, %g)\n",
		old_t1,old_t1-old_t2,fr->t-old_t1);
      }
    }
    old_t2=old_t1;
    old_t1=fr->t;
    if (t0 == NOTSET) t0=fr->t;
    if (fnr == 0)
      fprintf(stderr,"\rframe: %6d (index %6d), t: %10.3f\n",
	      fr->step,fnr,fr->t);
    fnr++;
  }
  fprintf(stderr,"\n\nFound %d frames",fnr);
  if (bShowTStep && fnr>1)
    fprintf(stderr," with a timestep of %g ps",(old_t1-t0)/(fnr-1));
  fprintf(stderr,".\n");

  free_enxframe(fr);
  sfree(fr);
}


int main(int argc,char *argv[])
{
  static char *desc[] = {
    "gmxcheck reads a trajectory ([TT].trj[tt], [TT].trr[tt] or ",
    "[TT].xtc[tt]) or an energy file ([TT].ene[tt] or [TT].edr[tt])",
    "and prints out useful information about them.[PAR]",
    "Option [TT]-c[tt] checks for presence of coordinates,",
    "velocities and box in the file, for close contacts (smaller than",
    "[TT]-vdwfac[tt] and not bonded, i.e. not between [TT]-bonlo[tt]",
    "and [TT]-bonhi[tt], all relative to the sum of both Van der Waals",
    "radii) and atoms outside the box (these may occur often and are",
    "no problem). If velocities are present, an estimated temperature",
    "will be calculated from them.[PAR]",
    "The program will compare run input ([TT].tpr[tt], [TT].tpb[tt] or",
    "[TT].tpa[tt]) files",
    "when both [TT]-s1[tt] and [TT]-s2[tt] are supplied."
  };
  t_filenm fnm[] = {
    { efTRX, "-f",  NULL, ffOPTRD },
    { efTRX, "-f2",  NULL, ffOPTRD },
    { efTPX, "-s1", "top1", ffOPTRD },
    { efTPX, "-s2", "top2", ffOPTRD },
    { efTPS, "-c",  NULL, ffOPTRD },
    { efENX, "-e",  NULL, ffOPTRD },
    { efENX, "-e2", "ener2", ffOPTRD }
  };
#define NFILE asize(fnm)
  char *fn1=NULL,*fn2=NULL;
  
  static real vdw_fac=0.8;
  static real bon_lo=0.4;
  static real bon_hi=0.7;
  static real ftol=0;
  static char *lastener=NULL;
  static t_pargs pa[] = {
    { "-vdwfac", FALSE, etREAL, {&vdw_fac},
      "Fraction of sum of VdW radii used as warning cutoff" },
    { "-bonlo",  FALSE, etREAL, {&bon_lo},
      "Min. fract. of sum of VdW radii for bonded atoms" },
    { "-bonhi",  FALSE, etREAL, {&bon_hi},
      "Max. fract. of sum of VdW radii for bonded atoms" },
    { "-tol",    FALSE, etREAL, {&ftol},
      "Relative tolerance for comparing real values defined as 2*(a-b)/(|a|+|b|)" },
    { "-lastener",FALSE, etSTR,  {&lastener},
      "Last energy term to compare (if not given all are tested). It makes sense to go up until the Pressure." }
  };

  CopyRight(stdout,argv[0]);
  parse_common_args(&argc,argv,0,NFILE,fnm,asize(pa),pa,
		    asize(desc),desc,0,NULL);

  fn1 = opt2fn_null("-f",NFILE,fnm);
  fn2 = opt2fn_null("-f2",NFILE,fnm);
  if (fn1 && fn2)
    comp_trx(fn1,fn2,ftol);
  else if (fn1)
    chk_trj(fn1);
  else if (fn2)
    fprintf(stderr,"Please give me TWO trajectory (.xtc/.trr/.trj) files!\n");
  
  fn1 = opt2fn_null("-s1",NFILE,fnm);
  fn2 = opt2fn_null("-s2",NFILE,fnm);
  if (fn1 && fn2)
    comp_tpx(fn1,fn2,ftol);
  else if (fn1 || fn2)
    fprintf(stderr,"Please give me TWO run input (.tpr/.tpa/.tpb) files!\n");

  fn1 = opt2fn_null("-e",NFILE,fnm);
  fn2 = opt2fn_null("-e2",NFILE,fnm);
  if (fn1 && fn2)
    comp_enx(fn1,fn2,ftol,lastener);
  else if (fn1)
    chk_enx(ftp2fn(efENX,NFILE,fnm));
  else if (fn2)
    fprintf(stderr,"Please give me TWO energy (.edr/.ene) files!\n");
  
  if (ftp2bSet(efTPS,NFILE,fnm))
    chk_tps(ftp2fn(efTPS,NFILE,fnm), vdw_fac, bon_lo, bon_hi);
  
  if (ftp2bSet(efENX,NFILE,fnm))
  
  thanx(stderr);
  
  return 0;
}
