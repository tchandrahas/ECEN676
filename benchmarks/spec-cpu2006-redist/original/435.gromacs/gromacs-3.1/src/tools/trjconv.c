/*
 * $Id: trjconv.c,v 1.115 2002/02/28 11:13:17 spoel Exp $
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
 * Green Red Orange Magenta Azure Cyan Skyblue
 */
static char *SRCID_trjconv_c = "$Id: trjconv.c,v 1.115 2002/02/28 11:13:17 spoel Exp $";
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <math.h>
#include <unistd.h>
#include "macros.h"
#include "sysstuff.h"
#include "smalloc.h"
#include "typedefs.h"
#include "copyrite.h"
#include "gmxfio.h"
#include "tpxio.h"
#include "trnio.h"
#include "statutil.h"
#include "futil.h"
#include "pdbio.h"
#include "confio.h"
#include "names.h"
#include "rdgroup.h"
#include "vec.h"
#include "xtcio.h"
#include "do_fit.h"
#include "rmpbc.h"
#include "wgms.h"
#include "magic.h"
#include "binio.h"
#include "pbc.h"
#include "viewit.h"

static void center_x(rvec x[],matrix box,int n,int nc,atom_id ci[])
{
  int i,m,ai;
  rvec cmin,cmax,box_center,dx;

  if (nc > 0) {
    copy_rvec(x[ci[0]],cmin);
    copy_rvec(x[ci[0]],cmax);
    for(i=0; i<nc; i++) {
      ai=ci[i];
      for(m=0; m<DIM; m++) {
	if (x[ai][m] < cmin[m])
	  cmin[m]=x[ai][m];
	else if (x[ai][m] > cmax[m])
	  cmax[m]=x[ai][m];
      }
    }
    calc_box_center(box,box_center);
    for(m=0; m<DIM; m++)
      dx[m] = box_center[m]-(cmin[m]+cmax[m])*0.5;
      
    for(i=0; i<n; i++)
      rvec_inc(x[i],dx);
  }
}


void check_trn(char *fn)
{
  if ((fn2ftp(fn) != efTRJ)  && (fn2ftp(fn) != efTRR))
    fatal_error(0,"%s is not a trj file, exiting\n",fn);
}

#ifndef _win_
void do_trunc(char *fn, real t0)
{
  int          in;
  FILE         *fp;
  bool         bStop,bOK;
  t_trnheader  sh;
  off_t        fpos;
  char         yesno[256];
  int          j;
  real         t=0;
  
  if (t0 == -1)
    fatal_error(0,"You forgot to set the truncation time");
  
  /* Check whether this is a .trj file */
  check_trn(fn);
  
  in   = open_trn(fn,"r");
  fp   = fio_getfp(in);
  if (fp == NULL) {
    fprintf(stderr,"Sorry, can not trunc %s, truncation of this filetype is not supported\n",fn);
    close_trn(in);
  } else {
    j    = 0;
    fpos = fio_ftell(in);
    bStop= FALSE;
    while (!bStop && fread_trnheader(in,&sh,&bOK)) {
      fread_htrn(in,&sh,NULL,NULL,NULL,NULL);
      fpos=ftell(fp);
      t=sh.t;
      if (t>=t0) {
#ifdef HAVE_FSEEKO	
	fseeko(fp,fpos,SEEK_SET);
#else
	fseek(fp,fpos,SEEK_SET);
#endif	
	bStop=TRUE;
      }
    }
    if (bStop) {
      fprintf(stderr,"Do you REALLY want to truncate this trajectory (%s) at:\n"
	      "frame %d, time %g, bytes %ld ??? (type YES if so)\n",
	      fn,j,t,(long int)fpos);
      scanf("%s",yesno);
      if (strcmp(yesno,"YES") == 0) {
	fprintf(stderr,"Once again, I'm gonna DO this...\n");
	close_trn(in);
	truncate(fn,fpos);
      }
      else {
	fprintf(stderr,"Ok, I'll forget about it\n");
      }
    }
    else {
      fprintf(stderr,"Already at end of file (t=%g)...\n",t);
      close_trn(in);
    }
  }
}
#endif

int main(int argc,char *argv[])
{
  static char *desc[] = {
    "trjconv can convert trajectory files in many ways:[BR]",
    "[BB]1.[bb] from one format to another[BR]",
    "[BB]2.[bb] select a subset of atoms[BR]",
    "[BB]3.[bb] remove periodicity from molecules[BR]",
    "[BB]4.[bb] keep multimeric molecules together[BR]",
    "[BB]5.[bb] center atoms in the box[BR]",
    "[BB]6.[bb] fit atoms to reference structure[BR]",
    "[BB]7.[bb] reduce the number of frames[BR]",
    "[BB]8.[bb] change the timestamps of the frames ",
    "([TT]-t0[tt] and [TT]-timestep[tt])",
    "[PAR]",
    "The program [TT]trjcat[tt] can concatenate multiple trajectory files.",
    "[PAR]",
    "Currently seven formats are supported for input and output:",
    "[TT].xtc[tt], [TT].trr[tt], [TT].trj[tt], [TT].gro[tt], [TT].g96[tt],",
    "[TT].pdb[tt] and [TT].g87[tt].",
    "The file formats are detected from the file extension.",
    "The precision of [TT].xtc[tt] and [TT].gro[tt] output is taken from the",
    "input file for [TT].xtc[tt], [TT].gro[tt] and [TT].pdb[tt],",
    "and from the [TT]-ndec[tt] option for other input formats. The precision",
    "is always taken from [TT]-ndec[tt], when this option is set.",
    "All other formats have fixed precision. [TT].trr[tt] and [TT].trj[tt]",
    "output can be single or double precision, depending on the precision",
    "of the trjconv binary.",
    "Note that velocities are only supported in",
    "[TT].trr[tt], [TT].trj[tt], [TT].gro[tt] and [TT].g96[tt] files.[PAR]",
    "Option [TT]-app[tt] can be used to",
    "append output to an existing trajectory file.",
    "No checks are performed to ensure integrity",
    "of the resulting combined trajectory file.[PAR]",
    "Option [TT]-sep[tt] can be used to write every frame to a seperate",
    ".gro, .g96 or .pdb file, default all frames all written to one file.",
    "[TT].pdb[tt] files with all frames concatenated can be viewed with",
    "[TT]rasmol -nmrpdb[tt].[PAR]",
    "It is possible to select part of your trajectory and write it out",
    "to a new trajectory file in order to save disk space, e.g. for leaving",
    "out the water from a trajectory of a protein in water.",
    "[BB]ALWAYS[bb] put the original trajectory on tape!",
    "We recommend to use the portable [TT].xtc[tt] format for your analysis",
    "to save disk space and to have portable files.[PAR]",
    "There are two options for fitting the trajectory to a reference",
    "either for essential dynamics analysis or for whatever.",
    "The first option is just plain fitting to a reference structure",
    "in the structure file, the second option is a progressive fit",
    "in which the first timeframe is fitted to the reference structure ",
    "in the structure file to obtain and each subsequent timeframe is ",
    "fitted to the previously fitted structure. This way a continuous",
    "trajectory is generated, which might not be the case when using the",
    "regular fit method, e.g. when your protein undergoes large",
    "conformational transitions.[PAR]",
    "Option [TT]-pbc[tt] sets the type of periodic boundary condition",
    "treatment. [TT]whole[tt] puts the atoms in the box and then makes",
    "broken molecules whole (a run input file is required).",
    "[TT]inbox[tt] puts all the atoms in the box.",
    "[TT]nojump[tt] checks if atoms jump across the box and then puts",
    "them back. This has the effect that all molecules",
    "will remain whole (provided they were whole in the initial",
    "conformation), note that this ensures a continuous trajectory but",
    "molecules may diffuse out of the box. The starting configuration",
    "for this procedure is taken from the structure file, if one is",
    "supplied, otherwise it is the first frame.",
    "[TT]-pbc[tt] is ignored when [TT]-fit[tt] of [TT]-pfit[tt] is set,",
    "in that case molecules will be made whole.[PAR]",
    "Option [TT]-ur[tt] sets the unit cell representation for options",
    "[TT]whole[tt] and [TT]inbox[tt] of [TT]-pbc[tt].",
    "All three options give different results for triclinc boxes and",
    "identical results for rectangular boxes.",
    "[TT]rect[tt] is the ordinary brick shape.",
    "[TT]tric[tt] is the triclinic unit cell.", 
    "[TT]compact[tt] puts all atoms at the closest distance from the center",
    "of the box. This can be useful for visualizing e.g. truncated",
    "octahedrons.[PAR]",
    "Option [TT]-center[tt] centers the system in the box. The user can",
    "select the group which is used to determine the geometrical center.",
    "Use option [TT]-pbc whole[tt] in addition to [TT]-center[tt] when you",
    "want all molecules in the box after the centering.[PAR]",
    "With [TT]-dt[tt] it is possible to reduce the number of ",
    "frames in the output. This option relies on the accuracy of the times",
    "in your input trajectory, so if these are inaccurate use the",
    "[TT]-timestep[tt]",
    "option to modify the time (this can be done simultaneously).[PAR]",
    "Using [TT]-trunc[tt] trjconv can truncate [TT].trj[tt] in place, i.e.",
    "without copying the file. This is useful when a run has crashed",
    "during disk I/O (one more disk full), or when two contiguous",
    "trajectories must be concatenated without have double frames.[PAR]",
    "[TT]trjcat[tt] is more suitable for concatenating trajectory files.[PAR]",
    "Option [TT]-dump[tt] can be used to extract a frame at or near",
    "one specific time from your trajectory."
  };
  
  static char *pbc_opt[] = { NULL, "none", "whole", "inbox", "nojump", NULL };
  static char *unitcell_opt[] = { NULL, "rect", "tric", "compact",
				  NULL };

  static bool  bAppend=FALSE,bSeparate=FALSE,bVels=TRUE,bForce=FALSE;
  static bool  bCenter=FALSE,bFit=FALSE,bPFit=FALSE;
  static int   skip_nr=1,ndec=3;
  static real  tzero=0,delta_t=0,timestep=0,ttrunc=-1,tdump=-1,split_t=0;
  static rvec  newbox = {0,0,0}, shift = {0,0,0};
  static char  *exec_command=NULL;

  t_pargs pa[] = {
    { "-skip", FALSE,  etINT, {&skip_nr},
      "Only write every nr-th frame" },
    { "-dt", FALSE,  etTIME, {&delta_t},
      "Only write frame when t MOD dt = first time (%t)" },
    { "-dump", FALSE, etTIME, {&tdump},
      "Dump frame nearest specified time (%t)" },
    { "-t0", FALSE,  etTIME, {&tzero},
      "Starting time (%t) (default: don't change)"},
    { "-timestep", FALSE,  etTIME, {&timestep},
      "Change time step between input frames (%t)" },
    { "-pbc", FALSE,  etENUM, {pbc_opt},
      "PBC treatment" },
    { "-ur", FALSE,  etENUM, {unitcell_opt},
      "Unit-cell representation" },
    { "-center", FALSE,  etBOOL, {&bCenter},
      "Center atoms in box" },
    { "-box", FALSE, etRVEC, {newbox},
      "Size for new cubic box (default: read from input)" },
    { "-shift", FALSE, etRVEC, {shift},
      "All coordinates will be shifted by framenr*shift" },
    { "-fit", FALSE,  etBOOL, {&bFit},
      "Fit molecule to ref structure in the structure file" },
    { "-pfit", FALSE,  etBOOL, {&bPFit},
      "Progressive fit, to the previous fitted structure" },
    { "-ndec", FALSE,  etINT,  {&ndec},
      "Precision for .xtc and .gro writing in number of decimal places" },
    { "-vel", FALSE, etBOOL, {&bVels},
      "Read and write velocities if possible" },
    { "-force", FALSE, etBOOL, {&bForce},
      "Read and write forces if possible" },
#ifndef _win_
    { "-trunc", FALSE, etTIME, {&ttrunc},
      "Truncate input trj file after this time (%t)" },
#endif
    { "-exec", FALSE,  etSTR, {&exec_command},
      "Execute command for every output frame with the frame number "
      "as argument" },
    { "-app", FALSE,  etBOOL, {&bAppend},
      "Append output"},
    { "-split",FALSE, etTIME, {&split_t},
      "Start writing new file when t MOD split = first time (%t)" },
    { "-sep", FALSE,  etBOOL, {&bSeparate},
      "Write each frame to a separate .gro, .g96 or .pdb file"}
  };
      
  FILE         *out=NULL;
  int          trxout=NOTSET;
  int          status,ftp,ftpin=0,file_nr;
  t_trxframe   fr,frout;
  int          flags;
  rvec         *xmem=NULL,*vmem=NULL;
  rvec         *xp=NULL,x_shift,hbox,box_center,dx;
  real         xtcpr, lambda,*w_rls=NULL;
  int          m,i,d,frame,outframe,natoms,nout,ncent,nre,newstep=0;
#define SKIP 10
  t_topology   top;
  t_atoms      *atoms=NULL,useatoms;
  matrix       top_box;
  atom_id      *index,*cindex;
  char         *grpnm;
  int          *frindex,nrfri;
  char         *frname;
  int          ifit,irms;
  atom_id      *ind_fit,*ind_rms;
  char         *gn_fit,*gn_rms;
  real         tshift=0,t0=-1,dt=0.001,prec;
  bool         bPBC,bInBox,bNoJump,bRect,bTric,bComp;
  bool         bCopy,bDoIt,bIndex,bTDump,bSetTime,bTPS=FALSE,bDTset=FALSE;
  bool         bExec,bTimeStep=FALSE,bDumpFrame=FALSE,bSetPrec,bNeedPrec;
  bool         bHaveFirstFrame,bHaveNextFrame,bSetBox,bSplit;
  char         *top_file,*in_file,*out_file=NULL,out_file2[256],*charpt;
  char         top_title[256],title[256],command[256],filemode[5];
  int          xdr=0;

  t_filenm fnm[] = {
    { efTRX, "-f",  NULL, ffREAD },
    { efTRX, "-o", "trajout", ffWRITE },
    { efTPS, NULL,  NULL, ffOPTRD },
    { efNDX, NULL,  NULL, ffOPTRD },
    { efNDX, "-fr", "frames", ffOPTRD }
  };
#define NFILE asize(fnm)
  
  CopyRight(stderr,argv[0]);
  parse_common_args(&argc,argv,
		    PCA_CAN_BEGIN | PCA_CAN_END | PCA_CAN_VIEW | PCA_TIME_UNIT | PCA_BE_NICE,
		    NFILE,fnm,asize(pa),pa,asize(desc),desc,
		    0,NULL);

  top_file=ftp2fn(efTPS,NFILE,fnm);

  /* Check command line */
  in_file=opt2fn("-f",NFILE,fnm);
  if (ttrunc != -1) {
#ifndef _win_
    do_trunc(in_file,ttrunc);
#endif
  }
  else {
    if (bPFit)
      bFit = TRUE;
    bSetBox   = opt2parg_bSet("-box", asize(pa), pa);
    bSetTime  = opt2parg_bSet("-t0", asize(pa), pa);
    bSetPrec  = opt2parg_bSet("-ndec", asize(pa), pa);
    bExec     = opt2parg_bSet("-exec", asize(pa), pa);
    bTimeStep = opt2parg_bSet("-timestep", asize(pa), pa);
    bTDump    = opt2parg_bSet("-dump", asize(pa), pa);
    bPBC      = (strcmp(pbc_opt[0],"whole") == 0);
    bInBox    = (strcmp(pbc_opt[0],"inbox") == 0);
    bNoJump   = (strcmp(pbc_opt[0],"nojump") == 0);
    bRect     = (strcmp(unitcell_opt[0],"rect") == 0);
    bTric     = (strcmp(unitcell_opt[0],"tric") == 0);
    bComp     = (strcmp(unitcell_opt[0],"compact") == 0);
    if (bFit && (strcmp(pbc_opt[0],"none") == 0))
      bPBC = TRUE;
    if (bPBC && !bFit)
      bInBox = TRUE;

    /* ndec is in nr of decimal places, prec is a multiplication factor: */
    prec = 1;
    for (i=0; i<ndec; i++)
      prec *= 10;
    
    bIndex=ftp2bSet(efNDX,NFILE,fnm);
    
    /* Determine output type */ 
    out_file=opt2fn("-o",NFILE,fnm);
    ftp=fn2ftp(out_file);
    fprintf(stderr,"Will write %s: %s\n",ftp2ext(ftp),ftp2desc(ftp));
    bNeedPrec = (ftp==efXTC || ftp==efGRO);
    if (bVels) {
      /* check if velocities are possible in input and output files */
      ftpin=fn2ftp(in_file);
      bVels= (ftp==efTRR || ftp==efTRJ || ftp==efGRO || ftp==efG96) 
	&& (ftpin==efTRR || ftpin==efTRJ || ftpin==efGRO || ftp==efG96);
    }
     
    /* skipping */  
    if (skip_nr <= 0) {
      fprintf(stderr,"The number of frames to skip is <= 0. "
	      "Writing out all frames.\n\n");
      skip_nr = 1;
    } 
    
    /* Determine whether to read a topology */
    bTPS = (ftp2bSet(efTPS,NFILE,fnm) || 
	    bPBC || bFit || (ftp == efGRO) || (ftp == efPDB));

    /* Determine if when can read index groups */
    bIndex = (bIndex || bTPS);
    
    if (bTPS) {
      read_tps_conf(top_file,top_title,&top,&xp,NULL,top_box,bFit);
      atoms=&top.atoms;
      /* top_title is only used for gro and pdb,
       * the header in such a file is top_title t= ...
       * to prevent a double t=, remove it from top_title
       */
      if ((charpt=strstr(top_title," t= ")))
	charpt[0]='\0';
    }
    
    /* get frame number index */
    frindex=NULL;
    if (opt2bSet("-fr",NFILE,fnm)) {
      printf("Select groups of frame number indices:\n");
      rd_index(opt2fn("-fr",NFILE,fnm),1,&nrfri,(atom_id **)&frindex,&frname);
      if (debug)
	for(i=0; i<nrfri; i++)
	  fprintf(debug,"frindex[%4d]=%4d\n",i,frindex[i]);
    }
    
      /* get index groups etc. */
    if (bFit) {
      printf("Select group for root least squares fit\n");
      get_index(atoms,ftp2fn_null(efNDX,NFILE,fnm),
		1,&ifit,&ind_fit,&gn_fit);

      if (ifit < 3) 
	fatal_error(0,"Need at least 3 points to fit!\n");
    }
    
    if (bIndex) {
      if (bCenter) {
	printf("Select group for centering\n");
	get_index(atoms,ftp2fn_null(efNDX,NFILE,fnm),
		  1,&ncent,&cindex,&grpnm);
      }
      printf("Select group for output\n");
      get_index(atoms,ftp2fn_null(efNDX,NFILE,fnm),
		1,&nout,&index,&grpnm);
    } else {
      /* no index file, so read natoms from TRX */
      read_first_frame(&status,in_file,&fr,TRX_DONT_SKIP);
      natoms = fr.natoms;
      close_trj(status);
      sfree(fr.x);
      snew(index,natoms);
      for(i=0;i<natoms;i++)
	index[i]=i;
      nout=natoms; 
      if (bCenter) {
	ncent = nout;
	cindex = index;
      }
    }
    
    if (bFit) {
      snew(w_rls,atoms->nr);
      for(i=0; (i<ifit); i++)
	w_rls[ind_fit[i]]=atoms->atom[ind_fit[i]].m;
      
      /* Restore reference structure and set to origin, 
         store original location (to put structure back) */
      rm_pbc(&(top.idef),atoms->nr,top_box,xp,xp);
      copy_rvec(xp[index[0]],x_shift);
      reset_x(ifit,ind_fit,atoms->nr,NULL,xp,w_rls);
      rvec_dec(x_shift,xp[index[0]]);
    } else
      clear_rvec(x_shift);
    
    /* Make atoms struct for output in GRO or PDB files */
    if ((ftp == efGRO) || ((ftp == efG96) && bTPS) || (ftp == efPDB)) {
      /* get memory for stuff to go in pdb file */
      init_t_atoms(&useatoms,atoms->nr,FALSE);
      sfree(useatoms.resname);
      useatoms.resname=atoms->resname;
      for(i=0;(i<nout);i++) {
	useatoms.atomname[i]=atoms->atomname[index[i]];
	useatoms.atom[i]=atoms->atom[index[i]];
	useatoms.nres=max(useatoms.nres,useatoms.atom[i].resnr+1);
      }
      useatoms.nr=nout;
    }
    /* select what to read */
    if (ftp==efTRR || ftp==efTRJ)
      flags = TRX_READ_X; 
    else
      flags = TRX_NEED_X;
    if (bVels)
      flags = flags | TRX_READ_V;
    if (bForce)
      flags = flags | TRX_READ_F;

    /* open trx file for reading */
    bHaveFirstFrame = read_first_frame(&status,in_file,&fr,flags);
    if (fr.bPrec)
      fprintf(stderr,"\nPrecision of %s is %g (nm)\n",in_file,1/fr.prec);
    if (bNeedPrec) {
      if (bSetPrec || !fr.bPrec)
	fprintf(stderr,"\nSetting output precision to %g (nm)\n",1/prec);
      else
	fprintf(stderr,"Using output precision of %g (nm)\n",1/prec);
    }

    if (bHaveFirstFrame) {
      natoms = fr.natoms;
      
      if (bSetTime)
	tshift=tzero-fr.time;
      else
	tzero=fr.time;
      
      /* open output for writing */
      if ((bAppend) && (fexist(out_file))) {
	strcpy(filemode,"a");
	fprintf(stderr,"APPENDING to existing file %s\n",out_file);
      } else
	strcpy(filemode,"w");
      switch (ftp) {
      case efXTC:
      case efG87:
      case efTRR:
      case efTRJ:
	out=NULL;
	if ( split_t == 0 )
	  trxout = open_trx(out_file,filemode);
	break;
      case efGRO:
      case efG96:
      case efPDB:
	if ( !bSeparate && split_t == 0 )
	  out=ffopen(out_file,filemode);
	break;
      }
      
      bCopy=FALSE;
      if (bIndex)
	/* check if index is meaningful */
	for(i=0; i<nout; i++) {
	  if (index[i] >= natoms)
	  fatal_error(0,"Index[%d] %d is larger than the number of atoms in the trajectory file (%d)",i,index[i]+1,natoms);
	  bCopy = bCopy || (i != index[i]);
	}
      if (bCopy) {
	snew(xmem,nout);
	snew(vmem,nout);
      }
      
      if (ftp == efG87)
	fprintf(fio_getfp(trxout),"Generated by %s. #atoms=%d, a BOX is"
		" stored in this file.\n",Program(),nout);
    
      /* Start the big loop over frames */
      file_nr  =  0;  
      frame    =  0;
      outframe =  0;
      bDTset   = FALSE;
    
      do {
	if (!fr.bStep) {
	  /* set the step */
	  fr.step = newstep;
	  newstep++;
	}
	  
	if (bSetBox) {
	  /* generate new box */
	  clear_mat(fr.box);
	  for (m=0; m<DIM; m++)
	    fr.box[m][m] = newbox[m];
	}
	
	if (bTDump) {
	  /* determine timestep */
	  if (t0 == -1) {
	    t0 = fr.time;
	  } else {
	    if (!bDTset) {
	      dt = fr.time-t0;
	      bDTset = TRUE;
	    }
	  }
	  bDumpFrame = (fr.time >= tdump-0.5*dt) && (fr.time <= tdump+0.5*dt);
	} else
	  bDumpFrame = FALSE;
	
	/* determine if an atom jumped across the box and reset it if so */
	if (bNoJump && (bTPS || frame!=0)) {
	  for(d=0; d<DIM; d++)
	    hbox[d] = 0.5*fr.box[d][d];
	  for(i=0; i<natoms; i++)
	    for(m=DIM-1; m>=0; m--)
	      if (hbox[m] > 0) {
		while (fr.x[i][m]-xp[i][m] <= -hbox[m])
		  for(d=0; d<=m; d++)
		    fr.x[i][d] += fr.box[m][d];
		while (fr.x[i][m]-xp[i][m] > hbox[m])
		  for(d=0; d<=m; d++)
		    fr.x[i][d] -= fr.box[m][d];
	      }
	}
      
	if (bPFit) {
	  /* Now modify the coords according to the flags,
	     for normal fit, this is only done for output frames */
	  rm_pbc(&(top.idef),natoms,fr.box,fr.x,fr.x);
	
	  reset_x(ifit,ind_fit,natoms,NULL,fr.x,w_rls);
	  do_fit(natoms,w_rls,xp,fr.x);
	}
      
	/* store this set of coordinates for future use */
	if (bPFit || bNoJump) {
	  if (xp == NULL)
	    snew(xp,natoms);
	  for(i=0; (i<natoms); i++) {
	    copy_rvec(fr.x[i],xp[i]);
	    rvec_inc(fr.x[i],x_shift);
	  }
	}
	
	if ( frindex )
	  /* see if we have a frame from the frame index group */
	  for(i=0; i<nrfri && !bDumpFrame; i++)
	    bDumpFrame = frame == frindex[i];
	if (debug && bDumpFrame)
	  fprintf(debug,"dumping %d\n",frame);
	
	if ( ( !bTDump && !frindex && frame % skip_nr == 0 ) || bDumpFrame ) {
	  
	  /* calc new time */
	  if (bTimeStep) 
	    fr.time = tzero+frame*timestep;
	  else
	    if (bSetTime)
	      fr.time += tshift;

	  if (bTDump)
	    fprintf(stderr,"\nDumping frame at t= %g %s\n",
		    convert_time(fr.time),time_unit());

	/* check for writing at each delta_t */
	  bDoIt=(delta_t == 0);
	  if (!bDoIt)
	    bDoIt=bRmod(fr.time-tzero, delta_t);
	
	  if (bDoIt || bTDump) {
	    /* print sometimes */
	    if ( ((outframe % SKIP) == 0) || (outframe < SKIP) )
	      fprintf(stderr," ->  frame %6d time %8.3f      \r",
		      outframe,convert_time(fr.time));
	  
	    if (!bPFit) {
	      /* Now modify the coords according to the flags,
		 for PFit we did this already! */
	    
	      if (bCenter)
		center_x(fr.x,fr.box,natoms,ncent,cindex);

	      if (bInBox) {
		if (bRect)
		  put_atoms_in_box(fr.box,natoms,fr.x);
		else if (bTric)
		  put_atoms_in_triclinic_unitcell(fr.box,natoms,fr.x);
		else if (bComp)
		  put_atoms_in_compact_unitcell(fr.box,natoms,fr.x);
	      }
	    
	      if (bPBC)
		rm_pbc(&(top.idef),natoms,fr.box,fr.x,fr.x);
	  
	      if (bFit) {
		reset_x(ifit,ind_fit,natoms,NULL,fr.x,w_rls);
		do_fit(natoms,w_rls,xp,fr.x);
		for(i=0; i<natoms; i++)
		  rvec_inc(fr.x[i],x_shift);
	      }
	    }
	    /* Copy the input trxframe struct to the output trxframe struct */
	    frout = fr;
	    frout.natoms = nout;
	    if (bNeedPrec && (bSetPrec || !fr.bPrec)) {
	      frout.bPrec = TRUE;
	      frout.prec  = prec;
	    }
	    if (bCopy) {
	      frout.x = xmem;
	      frout.v = vmem;
	      for(i=0; i<nout; i++) {
		copy_rvec(fr.x[index[i]],frout.x[i]);
		if (bVels && fr.bV) copy_rvec(fr.v[index[i]],frout.v[i]);
	      }
	    }
	  
	    if (opt2parg_bSet("-shift",asize(pa),pa))
	      for(i=0; i<nout; i++)
		for (d=0; d<DIM; d++)
		  frout.x[i][d] += outframe*shift[d];
	  
	    bSplit = split_t!=0 && bRmod(fr.time-tzero, split_t);
	    if (bSeparate || bSplit)
	      sprintf(out_file2,"%d_%s",file_nr,out_file);
	    switch(ftp) {
	    case efTRJ:
	    case efTRR:
	    case efG87:
	    case efXTC:
	      if ( bSplit ) {
		if ( trxout >= 0 )
		  close_trx(trxout);
		trxout = open_trx(out_file2,filemode);
	      }
	      write_trxframe(trxout,&frout);
	      break;
	    case efGRO:
	    case efG96:
	    case efPDB:
	      sprintf(title,"Generated by trjconv : %s t= %9.5f",
		      top_title,fr.time);
	      if (bSeparate || bSplit )
		out=ffopen(out_file2,"w");
	      switch(ftp) {
	      case efGRO: 
		write_hconf_p(out,title,&useatoms,prec2ndec(frout.prec),
			      frout.x,fr.bV?frout.v:NULL,frout.box);
		break;
	      case efPDB:
		fprintf(out,"REMARK    GENERATED BY TRJCONV\n");
		sprintf(title,"%s t= %9.5f",top_title,frout.time);
		/* if reading from pdb, we want to keep the original 
		   model numbering else we write the output frame
		   number plus one, because model 0 is not allowed in pdb */
		write_pdbfile(out,title,&useatoms,frout.x,frout.box,0,
			      (ftpin==efPDB) ? fr.step : (outframe+1) );
		break;
	      case efG96:
		frout.title = title;
		if (bSeparate || bTDump) {
		  fr.bTitle = TRUE;
		  fr.bAtoms = TRUE;
		  fr.bStep = FALSE;
		  fr.bTime = FALSE;
		} else {
		  frout.bTitle = (outframe == 0);
		  frout.bAtoms = FALSE;
		  frout.bStep = TRUE;
		  frout.bTime = TRUE;
		}
		write_g96_conf(out,&frout,-1,NULL);
	      }
	      if (bSeparate) {
		ffclose(out);
		out = NULL;
	      }
	      break;
	    default:
	      fatal_error(0,"DHE, ftp=%d\n",ftp);
	    }
	    if (bSeparate || bSplit)
	      file_nr++;
	  
	    /* execute command */
	    if (bExec) {
	      char c[255];
	      sprintf(c,"%s  %d",exec_command,file_nr-1);
	      /*fprintf(stderr,"Executing '%s'\n",c);*/
	      system(c);
	    }
	    outframe++;
	  }
	}
	frame++;
	bHaveNextFrame=read_next_frame(status,&fr);
      } while (!(bTDump && bDumpFrame) && bHaveNextFrame);
    }
    
    if (!bHaveFirstFrame || (bTDump && !bDumpFrame))
      fprintf(stderr,"\nWARNING no output, "
	      "trajectory ended at %g\n",fr.time);
    fprintf(stderr,"\n");
    
    close_trj(status);
    
    if (trxout >= 0)
      close_trx(trxout);
    else if (out != NULL)
      fclose(out);
  }
  
  do_view(out_file,NULL);
  
  thanx(stderr);
  
  return 0;
}
