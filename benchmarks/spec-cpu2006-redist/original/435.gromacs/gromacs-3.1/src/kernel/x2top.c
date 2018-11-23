/*
 * $Id: x2top.c,v 1.21 2002/02/28 10:54:45 spoel Exp $
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
static char *SRCID_x2top_c = "$Id: x2top.c,v 1.21 2002/02/28 10:54:45 spoel Exp $";
#include "maths.h"
#include "macros.h"
#include "copyrite.h"
#include "bondf.h"
#include "string2.h"
#include "smalloc.h"
#include "strdb.h"
#include "sysstuff.h"
#include "confio.h"
#include "physics.h"
#include "statutil.h"
#include "vec.h"
#include "random.h"
#include "3dview.h"
#include "txtdump.h"
#include "readinp.h"
#include "names.h"
#include "toppush.h"
#include "pdb2top.h"
#include "gen_ad.h"
#include "topexcl.h"
#include "vec.h"
#include "x2top.h"

char atp[6] = "HCNOSX";
#define NATP asize(atp)

real blen[NATP][NATP] = { 
  {  0.00,  0.108, 0.105, 0.10, 0.10, 0.10 },
  {  0.108, 0.15,  0.14,  0.14, 0.16, 0.14 },
  {  0.105, 0.14,  0.14,  0.14, 0.16, 0.14 },
  {  0.10,  0.14,  0.14,  0.14, 0.17, 0.14 },
  {  0.10,  0.16,  0.16,  0.17, 0.20, 0.17 },
  {  0.10,  0.14,  0.14,  0.14, 0.17, 0.17 }
};

#define MARGIN_FAC 1.1

static real set_x_blen(real scale)
{
  real maxlen;
  int  i,j;

  for(i=0; i<NATP-1; i++) {
    blen[NATP-1][i] *= scale;
    blen[i][NATP-1] *= scale;
  }
  blen[NATP-1][NATP-1] *= scale;
  
  maxlen = 0;
  for(i=0; i<NATP; i++)
    for(j=0; j<NATP; j++)
      if (blen[i][j] > maxlen)
	maxlen = blen[i][j];
  
  return maxlen*MARGIN_FAC;
}

static int get_atype(char *nm)
{
  int i,aai=NATP-1;

  for(i=0; (i<NATP-1); i++) {
    if (nm[0] == atp[i]) {
      aai=i;
      break;
    }
  }
  return aai;
}

static bool is_bond(int aai,int aaj,real len2)
{
  bool bIsBond;
  real bl;
    
  if (len2 == 0.0)
    bIsBond = FALSE;
  else {
    /* There is a bond when the deviation from ideal length is less than
     * 10 %
     */
    bl = blen[aai][aaj]*MARGIN_FAC;
    
    bIsBond = (len2 < bl*bl);
  }
#ifdef DEBUG
  if (debug)
    fprintf(debug,"ai: %5s  aj: %5s  len: %8.3f  bond: %s\n",
	    ai,aj,len,BOOL(bIsBond));
#endif
  return bIsBond;
}

real get_amass(char *aname,int nmass,char **nm2mass)
{
  int    i;
  char   nmbuf[32];
  double mass;
  real   m;
  
  m = 12;
  for(i=0; (i<nmass); i++) {
    sscanf(nm2mass[i],"%s%lf",nmbuf,&mass);
    trim(nmbuf);
    if (strcmp(aname,nmbuf) == 0) {
      m = mass;
      break;
    }
  }
  return m;
}

void mk_bonds(t_atoms *atoms,rvec x[],t_params *bond,int nbond[],char *ff,
	      real cutoff,bool bPBC,matrix box)
{
  t_param b;
  t_atom  *atom;
  char    **nm2mass=NULL,buf[128];
  int     i,j,aai,nmass;
  rvec    dx;
  real    dx2,c2;
  
  for(i=0; (i<MAXATOMLIST); i++)
    b.a[i] = -1;
  for(i=0; (i<MAXFORCEPARAM); i++)
    b.c[i] = 0.0;
    
  sprintf(buf,"%s.atp",ff);
  nmass = get_file(buf,&nm2mass);
  fprintf(stderr,"There are %d type to mass translations\n",nmass);
  atom  = atoms->atom;
  for(i=0; (i<atoms->nr); i++) {
    atom[i].type = get_atype(*atoms->atomname[i]);
    atom[i].m    = get_amass(*atoms->atomname[i],nmass,nm2mass);
  }
  c2 = sqr(cutoff);
  if (bPBC)
    init_pbc(box);
  for(i=0; (i<atoms->nr); i++) {
    if ((i % 10) == 0)
      fprintf(stderr,"\ratom %d",i);
    aai = atom[i].type;
    for(j=i+1; (j<atoms->nr); j++) {
      if (bPBC)
	pbc_dx(x[i],x[j],dx);
      else
	rvec_sub(x[i],x[j],dx);
      
      dx2 = iprod(dx,dx);
      if ((dx2 < c2) && (is_bond(aai,atom[j].type,dx2))) {
	b.AI = i;
	b.AJ = j;
	b.C0 = sqrt(dx2);
	push_bondnow (bond,&b);
	nbond[i]++;
	nbond[j]++;
      }
    }
  }
  fprintf(stderr,"\ratom %d\n",i);
}

int *set_cgnr(t_atoms *atoms)
{
  int    i,n=0;
  int    *cgnr;
  double qt = 0;
  
  snew(cgnr,atoms->nr);
  for(i=0; (i<atoms->nr); i++) {
    qt += atoms->atom[i].q;
    cgnr[i] = n;
    if (is_int(qt)) {
      n++;
      qt=0;
    }
  }
  return cgnr;
}

t_atomtype *set_atom_type(t_atoms *atoms,int nbonds[],
			  int nnm,t_nm2type nm2t[])
{
  static t_symtab symtab;
  t_atomtype *atype;
  char *type;
  int  i,k;
  
  open_symtab(&symtab);
  snew(atype,1);
  atype->nr       = 0;
  atype->atom     = NULL;
  atype->atomname = NULL;
  k=0;
  for(i=0; (i<atoms->nr); i++) {
    if ((type = nm2type(nnm,nm2t,*atoms->atomname[i],nbonds[i])) == NULL)
      fatal_error(0,"No forcefield type for atom %s (%d) with %d bonds",
		  *atoms->atomname[i],i+1,nbonds[i]);
    else if (debug)
      fprintf(debug,"Selected atomtype %s for atom %s\n",
	      type,*atoms->atomname[i]);
    for(k=0; (k<atype->nr); k++) {
      if (strcmp(type,*atype->atomname[k]) == 0) {
	atoms->atom[i].type  = k;
	atoms->atom[i].typeB = k;
	break;
      }
    }
    if (k==atype->nr) {
      /* New atomtype */
      atype->nr++;
      srenew(atype->atomname,atype->nr);
      srenew(atype->atom,atype->nr);
      atype->atomname[k]   = put_symtab(&symtab,type);
      atype->atom[k].type  = k;
      atoms->atom[i].type  = k;
      atype->atom[k].typeB = k;
      atoms->atom[i].typeB = k;
    }
  } 
  /* MORE CODE */

  close_symtab(&symtab);
    
  fprintf(stderr,"There are %d different atom types in your sample\n",
	  atype->nr);
    
  return atype;
}

void lo_set_force_const(t_params *plist,real c[],int nrfp,bool bRound,
			bool bDih,bool bParam)
{
  int    i,j;
  double cc;
  char   buf[32];
  
  for(i=0; (i<plist->nr); i++) {
    if (!bParam)
      for(j=0; j<nrfp; j++)
	c[j] = NOTSET;
    else {
      if (bRound) {
	sprintf(buf,"%.2e",plist->param[i].c[0]);
	sscanf(buf,"%lf",&cc);
	c[0] = cc;
      }
      else 
	c[0] = plist->param[i].c[0];
      if (bDih) {
	c[0] *= c[2];
	c[0] = ((int)(c[0] + 3600)) % 360;
	if (c[0] > 180)
	  c[0] -= 360;
	/* To put the minimum at the current angle rather than the maximum */
	c[0] += 180; 
      }
    }
    for(j=0; (j<nrfp); j++) {
      plist->param[i].c[j]      = c[j];
      plist->param[i].c[nrfp+j] = c[j];
    }
    set_p_string(&(plist->param[i]),"");
  }
}

void set_force_const(t_params plist[],real kb,real kt,real kp,bool bRound,
		     bool bParam)
{
  int i;
  real c[MAXFORCEPARAM];
  
  c[0] = 0;
  c[1] = kb;
  lo_set_force_const(&plist[F_BONDS],c,2,bRound,FALSE,bParam);
  c[1] = kt;
  lo_set_force_const(&plist[F_ANGLES],c,2,bRound,FALSE,bParam);
  c[1] = kp;
  c[2] = 3;
  lo_set_force_const(&plist[F_PDIHS],c,3,bRound,TRUE,bParam);
}

void calc_angles_dihs(t_params *ang,t_params *dih,rvec x[],bool bPBC,
		      matrix box)
{
  int    i,ai,aj,ak,al;
  rvec   r_ij,r_kj,r_kl,m,n;
  real   sign,th,costh,ph,cosph;

  if (!bPBC)
    box[XX][XX] = box[YY][YY] = box[ZZ][ZZ] = 1000;
  if (debug)
    pr_rvecs(debug,0,"X2TOP",box,DIM);
  for(i=0; (i<ang->nr); i++) {
    ai = ang->param[i].AI;
    aj = ang->param[i].AJ;
    ak = ang->param[i].AK;
    th = RAD2DEG*bond_angle(box,x[ai],x[aj],x[ak],r_ij,r_kj,&costh);
    if (debug)
      fprintf(debug,"X2TOP: ai=%3d aj=%3d ak=%3d r_ij=%8.3f r_kj=%8.3f th=%8.3f\n",
	      ai,aj,ak,norm(r_ij),norm(r_kj),th);
    ang->param[i].C0 = th;
  }
  for(i=0; (i<dih->nr); i++) {
    ai = dih->param[i].AI;
    aj = dih->param[i].AJ;
    ak = dih->param[i].AK;
    al = dih->param[i].AL;
    ph = RAD2DEG*dih_angle(box,x[ai],x[aj],x[ak],x[al],
			   r_ij,r_kj,r_kl,m,n,&cosph,&sign);
    if (debug)
      fprintf(debug,"X2TOP: ai=%3d aj=%3d ak=%3d al=%3d r_ij=%8.3f r_kj=%8.3f r_kl=%8.3f ph=%8.3f\n",
	      ai,aj,ak,al,norm(r_ij),norm(r_kj),norm(r_kl),ph);
    dih->param[i].C0 = ph;
  }
}

static void dump_hybridization(FILE *fp,t_atoms *atoms,int nbonds[])
{
  int i;
  
  for(i=0; (i<atoms->nr); i++) {
    fprintf(fp,"Atom %5s has %1d bonds\n",*atoms->atomname[i],nbonds[i]);
  }
}

static void print_rtp(char *filenm,char *title,char *name,t_atoms *atoms,
		      t_params plist[],t_atomtype *atype,int cgnr[])
{
  FILE *fp;
}

int main(int argc, char *argv[])
{
  static char *desc[] = {
    "x2top generates a primitive topology from a coordinate file.",
    "The program assumes all hydrogens are present when defining",
    "the hybridization from the atom name and the number of bonds.",
    "The program can also make an rtp entry, which you can then add",
    "to the rtp database.[PAR]",
    "When [TT]-param[tt] is set, equilibrium distances and angles",
    "and force constants will be printed in the topology for all",
    "interactions. The equilibrium distances and angles are taken",
    "from the input coordinates, the force constant are set with",
    "command line options."
  };
  static char *bugs[] = {
    "The atom type selection is primitive. Virtually no chemical knowledge is used",
    "Periodic boundary conditions screw up the bonding",
    "No improper dihedrals are generated",
    "The atoms to atomtype translation table is incomplete (ffG43a1.n2t file in the $GMXLIB directory). Please extend it and send the results back to the GROMACS crew."
  };
  FILE       *fp;
  t_params   plist[F_NRE];
  t_excls    *excls;
  t_atoms    *atoms;       /* list with all atoms */
  t_atomtype *atype;
  t_nextnb   nnb;
  t_nm2type  *nm2t;
  t_mols     mymol;
  char       *ff;
  int        nnm;
  char       title[STRLEN];
  rvec       *x;        /* coordinates? */
  int        *nbonds,*cgnr;
  int        bts[] = { 1,1,1,2 };
  matrix     box;          /* box length matrix */
  int        natoms;       /* number of atoms in one molecule  */
  int        nres;         /* number of molecules? */
  int        i,j,k,l,m;
  bool       bRTP,bTOP;
  real       cutoff;
  
  t_filenm fnm[] = {
    { efSTX, "-f", "conf", ffREAD  },
    { efTOP, "-o", "out",  ffOPTWR },
    { efRTP, "-r", "out",  ffOPTWR }
  };
#define NFILE asize(fnm)
  static real scale = 1.1, kb = 4e5,kt = 400,kp = 5;
  static int  nexcl = 3;
  static bool bParam = FALSE,bH14 = FALSE,bAllDih = FALSE,bRound = TRUE;
  static bool bPairs = TRUE, bPBC = TRUE;
  static char *molnm = "ICE";
  t_pargs pa[] = {
    { "-scale", FALSE, etREAL, {&scale},
      "Scaling factor for bonds with unknown atom types relative to atom type O" },
    { "-nexcl", FALSE, etINT,  {&nexcl},
      "Number of exclusions" },
    { "-H14",    FALSE, etBOOL, {&bH14}, 
      "Use 3rd neighbour interactions for hydrogen atoms" },
    { "-alldih", FALSE, etBOOL, {&bAllDih}, 
      "Generate all proper dihedrals" },
    { "-pairs",  FALSE, etBOOL, {&bPairs},
      "Output 1-4 interactions (pairs) in topology file" },
    { "-name",   FALSE, etSTR,  {&molnm},
      "Name of your molecule" },
    { "-pbc",    FALSE, etBOOL, {&bPBC},
      "Use periodic boundary conditions. Please set the GMXFULLPBC environment variable as well." },
    { "-param", FALSE, etBOOL, {&bParam},
      "Print parameters in the output" },
    { "-round",  FALSE, etBOOL, {&bRound},
      "Round off measured values" },
    { "-kb",    FALSE, etREAL, {&kb},
      "Bonded force constant (kJ/mol/nm^2)" },
    { "-kt",    FALSE, etREAL, {&kt},
      "Angle force constant (kJ/mol/rad^2)" },
    { "-kp",    FALSE, etREAL, {&kp},
      "Dihedral angle force constant (kJ/mol/rad^2)" }
  };
  
  CopyRight(stdout,argv[0]);

  parse_common_args(&argc,argv,0,NFILE,fnm,asize(pa),pa,
		    asize(desc),desc,asize(bugs),bugs);
  bRTP = opt2bSet("-r",NFILE,fnm);
  bTOP = opt2bSet("-o",NFILE,fnm);
  if (!bRTP && !bTOP)
    fatal_error(0,"Specify at least one output file");

  cutoff = set_x_blen(scale);

  if (bPBC)
    set_gmx_full_pbc(stdout);
    		    
  mymol.name = strdup(molnm);
  mymol.nr   = 1;
	
  /* Init parameter lists */
  init_plist(plist);
  
  /* Read coordinates */
  get_stx_coordnum(opt2fn("-f",NFILE,fnm),&natoms); 
  snew(atoms,1);
  
  /* make space for all the atoms */
  init_t_atoms(atoms,natoms,FALSE);
  snew(x,natoms);              

  read_stx_conf(opt2fn("-f",NFILE,fnm),title,atoms,x,NULL,box);

  ff = choose_ff();
  
  snew(nbonds,atoms->nr);
  
  printf("Generating bonds from distances...\n");
  mk_bonds(atoms,x,&(plist[F_BONDS]),nbonds,ff,cutoff,bPBC,box);

  nm2t = rd_nm2type(ff,&nnm);
  printf("There are %d name to type translations\n",nnm);
  if (debug)
    dump_nm2type(debug,nnm,nm2t);
  atype = set_atom_type(atoms,nbonds,nnm,nm2t);
  
  /* Make Angles and Dihedrals */
  snew(excls,atoms->nr);
  printf("Generating angles and dihedrals from bonds...\n");
  init_nnb(&nnb,atoms->nr,4);
  gen_nnb(&nnb,plist);
  print_nnb(&nnb,"NNB");
  gen_pad(&nnb,atoms,bH14,nexcl,plist,excls,NULL,bAllDih);
  done_nnb(&nnb);

  if (!bPairs)
    plist[F_LJ14].nr = 0;
  fprintf(stderr,
	  "There are %4d dihedrals, %4d impropers, %4d angles\n"
	  "          %4d pairs,     %4d bonds and  %4d atoms\n",
	  plist[F_PDIHS].nr, plist[F_IDIHS].nr, plist[F_ANGLES].nr,
	  plist[F_LJ14].nr, plist[F_BONDS].nr,atoms->nr);

  calc_angles_dihs(&plist[F_ANGLES],&plist[F_PDIHS],x,bPBC,box);
  
  set_force_const(plist,kb,kt,kp,bRound,bParam);

  cgnr = set_cgnr(atoms);

  if (bTOP) {    
    fp = ftp2FILE(efTOP,NFILE,fnm,"w");
    print_top_header(fp,ftp2fn(efTOP,NFILE,fnm),
		     "Generated by x2top",TRUE, ff,1.0);
    
    write_top(fp,NULL,mymol.name,atoms,bts,plist,excls,atype,cgnr,nexcl);
    print_top_mols(fp,mymol.name,0,NULL,1,&mymol);
    
    fclose(fp);
  }
  if (bRTP)
    print_rtp(ftp2fn(efRTP,NFILE,fnm),"Generated by x2top",
	      mymol.name,atoms,plist,atype,cgnr);
  
  if (debug) {
    dump_hybridization(debug,atoms,nbonds);
  }
  
  thanx(stderr);
  
  return 0;
}
