/*
 * $Id: g_covar.c,v 1.38 2002/02/28 11:00:25 spoel Exp $
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
static char *SRCID_g_covar_c = "$Id: g_covar.c,v 1.38 2002/02/28 11:00:25 spoel Exp $";
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "statutil.h"
#include "sysstuff.h"
#include "typedefs.h"
#include "smalloc.h"
#include "macros.h"
#include "vec.h"
#include "pbc.h"
#include "copyrite.h"
#include "futil.h"
#include "statutil.h"
#include "rdgroup.h"
#include "confio.h"
#include "trnio.h"
#include "mshift.h"
#include "xvgr.h"
#include "do_fit.h"
#include "rmpbc.h"
#include "txtdump.h"
#include "matio.h"
#include "eigio.h"
#include "ql77.h"

typedef double drvec[DIM];

int main(int argc,char *argv[])
{
  static char *desc[] = {
    "[TT]g_covar[tt] calculates and diagonalizes the (mass-weighted)",
    "covariance matrix.",
    "All structures are fitted to the structure in the structure file.",
    "When this is not a run input file periodicity will not be taken into",
    "account. When the fit and analysis groups are identical and the analysis",
    "is non mass-weighted, the fit will also be non mass-weighted.[PAR]",
    "The eigenvectors are written to a trajectory file ([TT]-v[tt]).",
    "When the same atoms are used for the fit and the covariance analysis,",
    "the reference structure for the fit is written first with t=-1.",
    "The average (or reference when [TT]-ref[tt] is used) structure is",
    "written with t=0, the eigenvectors",
    "are written as frames with the eigenvector number as timestamp.",
    "[PAR]",
    "The eigenvectors can be analyzed with [TT]g_anaeig[tt].",
    "[PAR]",
    "Option [TT]-xpm[tt] writes the whole covariance matrix to an xpm file.",
    "[PAR]",
    "Option [TT]-xpma[tt] writes the atomic covariance matrix to an xpm file,",
    "i.e. for each atom pair the sum of the xx, yy and zz covariances is",
    "written."
  };
  static bool bFit=TRUE,bRef=FALSE,bM=FALSE;
  static int  end=-1;
  t_pargs pa[] = {
    { "-fit",  FALSE, etBOOL, {&bFit},
      "Fit to a reference structure"},
    { "-ref",  FALSE, etBOOL, {&bRef},
      "Use the deviation from the conformation in the structure file instead of from the average" },
    { "-mwa",  FALSE, etBOOL, {&bM},
      "Mass-weighted covariance analysis"},
    { "-last",  FALSE, etINT, {&end}, 
      "Last eigenvector to write away (-1 is till the last)" }
  };
  FILE       *out;
  int        status,trjout;
  t_topology top;
  t_atoms    *atoms;  
  drvec      *xav;
  rvec       *x,*xread,*xref,*xproj;
  matrix     box,zerobox;
  double     *matd;
  real       t,tstart,tend,*mat,**mat2,dev,trace,sum,*eigval,inv_nframes;
  real       xj,*sqrtm,*w_rls=NULL;
  real       min,max,*axis;
  int        ntopatoms,step;
  int        natoms,nat,ndim,count,nframes,nlevels;
  int        WriteXref;
  char       *fitfile,*trxfile,*ndxfile;
  char       *eigvalfile,*eigvecfile,*averfile,*logfile,*xpmfile,*xpmafile;
  char       str[STRLEN],*fitname,*ananame;
  int        i,j,k,l,d,dj,nfit;
  atom_id    *index,*ifit;
  bool       bDiffMass1,bDiffMass2;
  time_t     now;
  t_rgb      rlo,rmi,rhi;

  t_filenm fnm[] = { 
    { efTRX, "-f",  NULL, ffREAD }, 
    { efTPS, NULL,  NULL, ffREAD },
    { efNDX, NULL,  NULL, ffOPTRD },
    { efXVG, NULL,  "eigenval", ffWRITE },
    { efTRN, "-v",  "eigenvec", ffWRITE },
    { efSTO, "-av", "average.pdb", ffWRITE },
    { efLOG, NULL,  "covar", ffWRITE },
    { efXPM, "-xpm","covar", ffOPTWR },
    { efXPM, "-xpma","covara", ffOPTWR }
  }; 
#define NFILE asize(fnm) 

  CopyRight(stderr,argv[0]); 
  parse_common_args(&argc,argv,PCA_CAN_TIME | PCA_TIME_UNIT | PCA_BE_NICE,
		    NFILE,fnm,asize(pa),pa,asize(desc),desc,0,NULL); 

  clear_mat(zerobox);

  fitfile    = ftp2fn(efTPS,NFILE,fnm);
  trxfile    = ftp2fn(efTRX,NFILE,fnm);
  ndxfile    = ftp2fn_null(efNDX,NFILE,fnm);
  eigvalfile = ftp2fn(efXVG,NFILE,fnm);
  eigvecfile = ftp2fn(efTRN,NFILE,fnm);
  averfile   = ftp2fn(efSTO,NFILE,fnm);
  logfile    = ftp2fn(efLOG,NFILE,fnm);
  xpmfile    = opt2fn_null("-xpm",NFILE,fnm);
  xpmafile   = opt2fn_null("-xpma",NFILE,fnm);

  read_tps_conf(fitfile,str,&top,&xref,NULL,box,TRUE);
  atoms=&top.atoms;

  if (bFit) {
    printf("\nChoose a group for the least squares fit\n"); 
    get_index(atoms,ndxfile,1,&nfit,&ifit,&fitname);
    if (nfit < 3) 
      fatal_error(0,"Need >= 3 points to fit!\n");
  } else
    nfit=0;
  printf("\nChoose a group for the covariance analysis\n"); 
  get_index(atoms,ndxfile,1,&natoms,&index,&ananame);

  bDiffMass1=FALSE;
  if (bFit) {
    snew(w_rls,atoms->nr);
    for(i=0; (i<nfit); i++) {
      w_rls[ifit[i]]=atoms->atom[ifit[i]].m;
      if (i)
        bDiffMass1 = bDiffMass1 || (w_rls[ifit[i]]!=w_rls[ifit[i-1]]);
    }
  }
  bDiffMass2=FALSE;
  snew(sqrtm,natoms);
  for(i=0; (i<natoms); i++)
    if (bM) {
      sqrtm[i]=sqrt(atoms->atom[index[i]].m);
      if (i)
	bDiffMass2 = bDiffMass2 || (sqrtm[i]!=sqrtm[i-1]);
    }
    else
      sqrtm[i]=1.0;
  
  if (bFit && bDiffMass1 && !bDiffMass2) {
    bDiffMass1 = natoms != nfit;
    i=0;
    for (i=0; (i<natoms) && !bDiffMass1; i++)
      bDiffMass1 = index[i] != ifit[i];
    if (!bDiffMass1) {
      fprintf(stderr,"\n"
	      "Note: the fit and analysis group are identical,\n"
	      "      while the fit is mass weighted and the analysis is not.\n"
	      "      Making the fit non mass weighted.\n\n");
      for(i=0; (i<nfit); i++)
	w_rls[ifit[i]]=1.0;
    }
  }

  /* Prepare reference frame */
  rm_pbc(&(top.idef),atoms->nr,box,xref,xref);
  if (bFit)
    reset_x(nfit,ifit,atoms->nr,NULL,xref,w_rls);

  snew(x,natoms);
  snew(xav,natoms);
  snew(xproj,natoms);
  ndim=natoms*DIM;
  snew(matd,ndim*ndim);
#ifndef DOUBLE
  snew(mat,ndim*ndim);
#else
  mat = matd;
#endif

  fprintf(stderr,"Constructing covariance matrix (%dx%d)...\n",ndim,ndim);

  nframes=0;
  nat=read_first_x(&status,trxfile,&t,&xread,box);
  tstart = t;
  do {
    nframes++;
    tend = t;
    /* calculate x: a fitted struture of the selected atoms */
    rm_pbc(&(top.idef),nat,box,xread,xread);
    if (bFit) {
      reset_x(nfit,ifit,nat,NULL,xread,w_rls);
      do_fit(nat,w_rls,xref,xread);
    }
    if (bRef)
      for (i=0; i<natoms; i++)
	rvec_sub(xread[index[i]],xref[index[i]],x[i]);
    else
      for (i=0; i<natoms; i++)
	copy_rvec(xread[index[i]],x[i]);
    
    for (j=0; j<natoms; j++) {
      /* calculate average structure */
      for(d=0; d<DIM; d++)
	xav[j][d] += x[j][d];
      /* calculate cross product matrix */
      for (dj=0; dj<DIM; dj++) {
	k=ndim*(DIM*j+dj);
	xj=x[j][dj];
	for (i=j; i<natoms; i++) {
	  l=k+DIM*i;
	  for(d=0; d<DIM; d++)
	    matd[l+d] += x[i][d]*xj;
	}
      }
    }
  } while (read_next_x(status,&t,nat,xread,box));
  close_trj(status);

  /* calculate and write the average structure */
  inv_nframes=1.0/nframes;
  for (i=0; i<natoms; i++)
    for(d=0; d<DIM; d++) {
      xav[i][d] *= inv_nframes;
      xread[index[i]][d] = xav[i][d];
    }
  write_sto_conf_indexed(opt2fn("-av",NFILE,fnm),"Average structure",
			 atoms,xread,NULL,zerobox,natoms,index);
  
  if (bRef) {
    /* copy the reference structure to the ouput array x */
    for (i=0; i<natoms; i++)
      copy_rvec(xref[index[i]],xproj[i]);
    /* correct the covariance matrix for the mass */
    for (j=0; j<natoms; j++) 
      for (dj=0; dj<DIM; dj++) 
	for (i=j; i<natoms; i++) { 
	  k = ndim*(DIM*j+dj)+DIM*i;
	  for (d=0; d<DIM; d++)
	    mat[k+d] = matd[k+d]*inv_nframes*sqrtm[i]*sqrtm[j];
	}
  } else {
    /* copy the average structure to the ouput array x */
    for (i=0; i<natoms; i++)
      for(d=0; d<DIM; d++)
	xproj[i][d] = xav[i][d];
    /* correct the covariance matrix for the mass and the average */
    for (j=0; j<natoms; j++) 
      for (dj=0; dj<DIM; dj++) 
	for (i=j; i<natoms; i++) { 
	  k = ndim*(DIM*j+dj)+DIM*i;
	  for (d=0; d<DIM; d++)
	    mat[k+d] = (matd[k+d]*inv_nframes-xav[i][d]*xav[j][dj])
	      *sqrtm[i]*sqrtm[j];
	}
  }

#ifndef DOUBLE
  sfree(matd);
#endif

  /* symmetrize the matrix */
  for (j=0; j<ndim; j++) 
    for (i=j; i<ndim; i++)
      mat[ndim*i+j]=mat[ndim*j+i];
  
  trace=0;
  for(i=0; i<ndim; i++)
    trace+=mat[i*ndim+i];
  fprintf(stderr,"\nTrace of the covariance matrix: %g (%snm^2)\n",
	  trace,bM ? "u " : "");
  
  if (debug) {
    fprintf(stderr,"Dumping the covariance matrix to covmat.dat\n"); 
    out = ffopen("covmat.dat","w");
    for (j=0; j<ndim; j++) {
      for (i=0; i<ndim; i++)
	fprintf(out," %g",mat[ndim*j+i]);
      fprintf(out,"\n");
    }
  }
  
  if (xpmfile) {
    min = 0;
    max = 0;
    snew(mat2,ndim);
    for (j=0; j<ndim; j++) {
      mat2[j] = &(mat[ndim*j]);
      for (i=0; i<=j; i++) {
	if (mat2[j][i] < min)
	  min = mat2[j][i];
	if (mat2[j][j] > max)
	  max = mat2[j][i];
      }
    }
    snew(axis,ndim);
    for(i=0; i<ndim; i++)
      axis[i] = i+1;
    rlo.r = 0; rlo.g = 0; rlo.b = 1;
    rmi.r = 1; rmi.g = 1; rmi.b = 1;
    rhi.r = 1; rhi.g = 0; rhi.b = 0;
    out = ffopen(xpmfile,"w");
    nlevels = 80;
    write_xpm3(out,"Covariance",bM ? "u nm^2" : "nm^2",
	       "dim","dim",ndim,ndim,axis,axis,
	       mat2,min,0.0,max,rlo,rmi,rhi,&nlevels);
    fclose(out);
    sfree(axis);
    sfree(mat2);
  }

  if (xpmafile) {
    min = 0;
    max = 0;
    snew(mat2,ndim/DIM);
    for (i=0; i<ndim/DIM; i++)
      snew(mat2[i],ndim/DIM);
    for (j=0; j<ndim/DIM; j++) {
      for (i=0; i<=j; i++) {
	mat2[j][i] = 0;
	for(d=0; d<DIM; d++)
	  mat2[j][i] += mat[ndim*(DIM*j+d)+DIM*i+d];
	if (mat2[j][i] < min)
	  min = mat2[j][i];
	if (mat2[j][j] > max)
	  max = mat2[j][i];
	mat2[i][j] = mat2[j][i];
      }
    }
    snew(axis,ndim/DIM);
    for(i=0; i<ndim/DIM; i++)
      axis[i] = i+1;
    rlo.r = 0; rlo.g = 0; rlo.b = 1;
    rmi.r = 1; rmi.g = 1; rmi.b = 1;
    rhi.r = 1; rhi.g = 0; rhi.b = 0;
    out = ffopen(xpmafile,"w");
    nlevels = 80;
    write_xpm3(out,"Covariance",bM ? "u nm^2" : "nm^2",
	       "atom","atom",ndim/DIM,ndim/DIM,axis,axis,
	       mat2,min,0.0,max,rlo,rmi,rhi,&nlevels);
    fclose(out);
    sfree(axis);
    for (i=0; i<ndim/DIM; i++)
      sfree(mat2[i]);
    sfree(mat2);
  }


  /* call diagonalization routine */
  
  fprintf(stderr,"\nDiagonalizing...\n");
  fflush(stderr);

  snew(eigval,ndim);
  ql77 (ndim,mat,eigval);
  
  /* now write the output */

  sum=0;
  for(i=0; i<ndim; i++)
    sum+=eigval[i];
  fprintf(stderr,"\nSum of the eigenvalues: %g (%snm^2)\n",
	  sum,bM ? "u " : "");
  if (fabs(trace-sum)>0.01*trace)
    fprintf(stderr,"\nWARNING: eigenvalue sum deviates from the trace of the covariance matrix\n");
  
  fprintf(stderr,"\nWriting eigenvalues to %s\n",eigvalfile);

  sprintf(str,"(%snm\\S2\\N)",bM ? "u " : "");
  out=xvgropen(eigvalfile, 
	       "Eigenvalues of the covariance matrix",
	       "Eigenvector index",str);  
  for (i=0; (i<ndim); i++)
    fprintf (out,"%10d %g\n",i+1,eigval[ndim-1-i]);
  fclose(out);  

  if (end==-1) {
    if (nframes-1 < ndim)
      end=nframes-1;
    else
      end=ndim;
  }
  if (bFit) {
    /* misuse lambda: 0/1 mass weighted analysis no/yes */
    if (nfit==natoms) {
      WriteXref = eWXR_YES;
      for(i=0; i<nfit; i++)
	copy_rvec(xref[ifit[i]],x[i]);
    } else
      WriteXref = eWXR_NO;
  } else {
    /* misuse lambda: -1 for no fit */
    WriteXref = eWXR_NOFIT;
  }

  write_eigenvectors(eigvecfile,natoms,mat,TRUE,1,end,
		     WriteXref,x,bDiffMass1,xproj,bM);

  out = ffopen(logfile,"w");

  now = time(NULL);
  fprintf(out,"Covariance analysis log, written %s\n",
	  ctime(&now));
  fprintf(out,"Program: %s\n",argv[0]);
  getcwd(str,STRLEN);
  fprintf(out,"Working directory: %s\n\n",str);

  fprintf(out,"Read %d frames from %s (time %g to %g %s)\n",nframes,trxfile,
	  convert_time(tstart),convert_time(tend),time_unit());
  if (bFit)
    fprintf(out,"Read reference structure for fit from %s\n",fitfile);
  if (ndxfile)
    fprintf(out,"Read index groups from %s\n",ndxfile);
  fprintf(out,"\n");

  fprintf(out,"Analysis group is '%s' (%d atoms)\n",ananame,natoms);
  if (bFit)
    fprintf(out,"Fit group is '%s' (%d atoms)\n",fitname,nfit);
  else
    fprintf(out,"No fit was used\n");
  fprintf(out,"Analysis is %smass weighted\n", bDiffMass2 ? "":"non-");
  if (bFit)
    fprintf(out,"Fit is %smass weighted\n", bDiffMass1 ? "":"non-");
  fprintf(out,"Diagonalized the %dx%d covariance matrix\n",ndim,ndim);
  fprintf(out,"Trace of the covariance matrix before diagonalizing: %g\n",
	  trace);
  fprintf(out,"Trace of the covariance matrix after diagonalizing: %g\n\n",
	  sum);

  fprintf(out,"Wrote %d eigenvalues to %s\n",ndim,eigvalfile);
  if (WriteXref == eWXR_YES)
    fprintf(out,"Wrote reference structure to %s\n",eigvecfile);
  fprintf(out,"Wrote average structure to %s and %s\n",averfile,eigvecfile);
  fprintf(out,"Wrote eigenvectors %d to %d to %s\n",1,end,eigvecfile);

  fclose(out);

  fprintf(stderr,"Wrote the log to %s\n",logfile);

  thanx(stderr);
  
  return 0;
}
