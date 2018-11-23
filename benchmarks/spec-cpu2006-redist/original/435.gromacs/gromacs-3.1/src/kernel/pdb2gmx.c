/*
 * $Id: pdb2gmx.c,v 1.100 2002/02/28 10:54:43 spoel Exp $
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
static char *SRCID_pdb2gmx_c = "$Id: pdb2gmx.c,v 1.100 2002/02/28 10:54:43 spoel Exp $";
#include <time.h>
#include <ctype.h>
#include "assert.h"
#include "sysstuff.h"
#include "typedefs.h"
#include "smalloc.h"
#include "copyrite.h"
#include "string2.h"
#include "confio.h"
#include "symtab.h"
#include "vec.h"
#include "statutil.h"
#include "futil.h"
#include "fatal.h"
#include "pdbio.h"
#include "toputil.h"
#include "h_db.h"
#include "physics.h"
#include "pgutil.h"
#include "calch.h"
#include "resall.h"
#include "pdb2top.h"
#include "ter_db.h"
#include "strdb.h"
#include "gbutil.h"
#include "genhydro.h"
#include "readinp.h"
#include "xlate.h"
#include "specbond.h"
#include "index.h"
#include "hizzie.h"

#define NREXCL 3

static char *select_res(int nr,int resnr,char *name[],char *expl[],char *title)
{
  int sel=0;

  printf("Which %s type do you want for residue %d\n",title,resnr+1);
  for(sel=0; (sel < nr); sel++)
    printf("%d. %s (%s)\n",sel,expl[sel],name[sel]);
  printf("\nType a number:"); fflush(stdout);

  if (scanf("%d",&sel) != 1)
    fatal_error(0,"Answer me for res %s %d!",title,resnr+1);
  
  return name[sel];
}

static char *get_asptp(int resnr)
{
  enum { easp, easpH, easpNR };
  static char *lh[easpNR] = { "ASP", "ASPH" };
  static char *expl[easpNR] = {
    "Not protonated (charge -1)",
    "Protonated (charge 0)"
  };

  return select_res(easpNR,resnr,lh,expl,"ASPARTIC ACID");
}

static char *get_glutp(int resnr)
{
  enum { eglu, egluH, egluNR };
  static char *lh[egluNR] = { "GLU", "GLUH" };
  static char *expl[egluNR] = {
    "Not protonated (charge -1)",
    "Protonated (charge 0)"
  };

  return select_res(egluNR,resnr,lh,expl,"GLUTAMIC ACID");
}

static char *get_lystp(int resnr)
{
  enum { elys, elysH, elysNR };
  static char *lh[elysNR] = { "LYS", "LYSH" };
  static char *expl[elysNR] = {
    "Not protonated (charge 0)",
    "Protonated (charge +1)"
  };

  return select_res(elysNR,resnr,lh,expl,"LYSINE");
}

static char *get_cystp(int resnr)
{
  enum { ecys, ecysH, ecysNR };
  static char *lh[ecysNR] = { "CYS", "CYSH" };
  static char *expl[ecysNR] = {
    "Cysteine in disulfide bridge",
    "Protonated"
  };

  return select_res(ecysNR,resnr,lh,expl,"CYSTEINE");

}

static char *get_histp(int resnr)
{
  static char *expl[ehisNR] = {
    "H on ND1 only",
    "H on NE2 only",
    "H on ND1 and NE2",
    "Coupled to Heme"
  };
  
  return select_res(ehisNR,resnr,hh,expl,"HISTIDINE");
}

static void rename_pdbres(t_atoms *pdba,char *oldnm,char *newnm,
			  bool bFullCompare,t_symtab *symtab)
{
  char *resnm;
  int i;
  
  for(i=0; (i<pdba->nres); i++) {
    resnm=*pdba->resname[i];
    if ((bFullCompare && (strcasecmp(resnm,oldnm) == 0)) ||
	(!bFullCompare && strstr(resnm,oldnm) != NULL)) {
      pdba->resname[i] = put_symtab(symtab,newnm);
    }
  }
}

static void rename_pdbresint(t_atoms *pdba,char *oldnm,
			     char *gettp(int),bool bFullCompare,
			     t_symtab *symtab)
{
  int  i;
  char *ptr,*resnm;
  
  for(i=0; i<pdba->nres; i++) {
    resnm=*pdba->resname[i];
    if ((bFullCompare && (strcmp(resnm,oldnm) == 0)) ||
	(!bFullCompare && strstr(resnm,oldnm) != NULL)) {
      ptr=gettp(i);
      pdba->resname[i]=put_symtab(symtab,ptr);
    }
  }
}

static void check_occupancy(t_atoms *atoms,char *filename)
{
  int i,ftp;
  int nzero=0;
  int nnotone=0;
  
  ftp = fn2ftp(filename);
  if (!atoms->pdbinfo || ((ftp != efPDB) && (ftp != efBRK) && (ftp != efENT)))
    fprintf(stderr,"No occupancies in %s\n",filename);
  else {
    for(i=0; (i<atoms->nr); i++) {
      if (atoms->pdbinfo[i].occup == 0)
	nzero++;
      else if (atoms->pdbinfo[i].occup != 1)
	nnotone++;
    }
    if (nzero == atoms->nr)
      fprintf(stderr,"All occupancy fields zero. This is probably not an X-Ray structure\n");
    else if ((nzero > 0) || (nnotone > 0))
      fprintf(stderr,
	      "WARNING: there were %d atoms with zero occupancy and %d atoms"
	      " with\n         occupancy unequal to one (out of %d atoms)."
	      " Check your pdb file.\n",nzero,nnotone,atoms->nr);
    else
      fprintf(stderr,"All occupancies are one\n");
  }
}

void write_posres(char *fn,t_atoms *pdba,real fc)
{
  FILE *fp;
  int  i;
  
  fp=ffopen(fn,"w");
  fprintf(fp,
	  "; In this topology include file, you will find position restraint\n"
	  "; entries for all the heavy atoms in your original pdb file.\n"
	  "; This means that all the protons which were added by pdb2gmx are\n"
	  "; not restrained.\n"
	  "\n"
	  "[ position_restraints ]\n"
	  "; %4s%6s%8s%8s%8s\n","atom","type","fx","fy","fz"
	  );
  for(i=0; (i<pdba->nr); i++) {
    if (!is_hydrogen(*pdba->atomname[i]) && !is_dummymass(*pdba->atomname[i]))
      fprintf(fp,"%6d%6d  %g  %g  %g\n",i+1,1,fc,fc,fc);
  }
  ffclose(fp);
}

int read_pdball(char *inf, char *outf,char *title,
		t_atoms *atoms, rvec **x,matrix box, bool bRemoveH,
		t_symtab *symtab)
/* Read a pdb file. (containing proteins) */
{
  int       natom,new_natom,i;
  
  /* READ IT */
  printf("Reading %s...\n",inf);
  get_stx_coordnum(inf,&natom);
  init_t_atoms(atoms,natom,TRUE);
  snew(*x,natom);
  read_stx_conf(inf,title,atoms,*x,NULL,box);
  if (bRemoveH) {
    new_natom=0;
    for(i=0; i<atoms->nr; i++)
      if (!is_hydrogen(*atoms->atomname[i])) {
	atoms->atom[new_natom]=atoms->atom[i];
	atoms->atomname[new_natom]=atoms->atomname[i];
	atoms->pdbinfo[new_natom]=atoms->pdbinfo[i];
	copy_rvec((*x)[i],(*x)[new_natom]);
	new_natom++;
      }
    atoms->nr=new_natom;
    natom=new_natom;
  }
    
  printf("Read");
  if (title && title[0])
    printf(" '%s',",title);
  printf(" %d atoms\n",natom);
  
  /* Rename residues */
  rename_pdbres(atoms,"SOL","HOH",FALSE,symtab);
  rename_pdbres(atoms,"WAT","HOH",FALSE,symtab);
  rename_pdbres(atoms,"HEM","HEME",FALSE,symtab);

  rename_atoms(atoms,symtab);
  
  if (natom == 0)
    return 0;

  if (outf)
    write_sto_conf(outf,title,atoms,*x,NULL,box);
 
  return natom;
}

void process_chain(t_atoms *pdba, rvec *x, 
		   bool bTrpU,bool bPheU,bool bTyrU,
		   bool bLysMan,bool bAspMan,bool bGluMan,
		   bool bHisMan,bool bCysMan,
		   int *nssbonds,t_ssbond **ssbonds,
		   real angle,real distance,t_symtab *symtab)
{
  /* Rename aromatics, lys, asp and histidine */
  if (bTyrU) rename_pdbres(pdba,"TYR","TYRU",FALSE,symtab);
  if (bTrpU) rename_pdbres(pdba,"TRP","TRPU",FALSE,symtab);
  if (bPheU) rename_pdbres(pdba,"PHE","PHEU",FALSE,symtab);
  if (bLysMan) 
    rename_pdbresint(pdba,"LYS",get_lystp,FALSE,symtab);
  else
    rename_pdbres(pdba,"LYS","LYSH",FALSE,symtab);
  if (bAspMan) 
    rename_pdbresint(pdba,"ASP",get_asptp,FALSE,symtab);
  else
    rename_pdbres(pdba,"ASPH","ASP",FALSE,symtab);
  if (bGluMan) 
    rename_pdbresint(pdba,"GLU",get_glutp,FALSE,symtab);
  else
    rename_pdbres(pdba,"GLUH","GLU",FALSE,symtab);

  /* Make sure we don't have things like CYS? */ 
  rename_pdbres(pdba,"CYS","CYS",FALSE,symtab);
  *nssbonds=mk_specbonds(pdba,x,bCysMan,ssbonds);
  rename_pdbres(pdba,"CYS","CYSH",TRUE,symtab);

  if (!bHisMan)
    set_histp(pdba,x,angle,distance);
  else
    rename_pdbresint(pdba,"HIS",get_histp,TRUE,symtab);
}

/* struct for sorting the atoms from the pdb file */
typedef struct {
  int  resnr;  /* residue number               */
  int  j;      /* database order index         */
  int  index;  /* original atom number         */
  char anm1;   /* second letter of atom name   */
  char altloc; /* alternate location indicator */
} t_pdbindex;
  
int pdbicomp(const void *a,const void *b)
{
  t_pdbindex *pa,*pb;
  int d;

  pa=(t_pdbindex *)a;
  pb=(t_pdbindex *)b;

  d = (pa->resnr - pb->resnr);
  if (d==0) {
    d = (pa->j - pb->j);
    if (d==0) {
      d = (pa->anm1 - pb->anm1);
      if (d==0)
	d = (pa->altloc - pb->altloc);
    }
  }

  return d;
}

static void sort_pdbatoms(int nrtp,t_restp restp[],
			  int natoms,t_atoms **pdbaptr,rvec **x,
			  t_block *block,char ***gnames)
{
  t_atoms *pdba,*pdbnew;
  rvec **xnew;
  int     i,j;
  t_restp *rptr;
  t_pdbindex *pdbi;
  atom_id *a;
  char *atomnm,*resnm;
  
  pdba=*pdbaptr;
  natoms=pdba->nr;
  pdbnew=NULL;
  snew(xnew,1);
  snew(pdbi, natoms);
  
  for(i=0; i<natoms; i++) {
    atomnm=*pdba->atomname[i];
    resnm=*pdba->resname[pdba->atom[i].resnr];
    if ((rptr=search_rtp(resnm,nrtp,restp)) == NULL)
      fatal_error(0,"Residue type %s not found",resnm);
    for(j=0; (j<rptr->natom); j++)
      if (strcasecmp(atomnm,*(rptr->atomname[j])) == 0)
	break;
    if (j==rptr->natom) {
      if ( ( ( pdba->atom[i].resnr == 0) && (atomnm[0] == 'H') &&
	     ( (atomnm[1] == '1') || (atomnm[1] == '2') || 
	       (atomnm[1] == '3') ) ) )
	j=1;
      else {
	char buf[STRLEN];
	
	sprintf(buf,"Atom %s in residue %s %d not found in rtp database\n"
		"             while sorting atoms%s",atomnm,
		rptr->resname,pdba->atom[i].resnr+1,
		is_hydrogen(atomnm) ? ". Maybe different protonation state.\n"
		"             Remove this hydrogen or choose a different "
		"protonation state.\n"
		"             Option -ignh will ignore all hydrogens "
		"in the input." : "");
	fatal_error(0,buf);
      }
    }
    /* make shadow array to be sorted into indexgroup */
    pdbi[i].resnr  = pdba->atom[i].resnr;
    pdbi[i].j      = j;
    pdbi[i].index  = i;
    pdbi[i].anm1   = atomnm[1];
    pdbi[i].altloc = pdba->pdbinfo[i].altloc;
  }
  qsort(pdbi,natoms,(size_t)sizeof(pdbi[0]),pdbicomp);
  
  /* pdba is sorted in pdbnew using the pdbi index */ 
  snew(a,natoms);
  snew(pdbnew,1);
  init_t_atoms(pdbnew,natoms,TRUE);
  snew(*xnew,natoms);
  pdbnew->nr=pdba->nr;
  pdbnew->nres=pdba->nres;
  sfree(pdbnew->resname);
  pdbnew->resname=pdba->resname;
  for (i=0; i<natoms; i++) {
    pdbnew->atom[i]     = pdba->atom[pdbi[i].index];
    pdbnew->atomname[i] = pdba->atomname[pdbi[i].index];
    pdbnew->pdbinfo[i]  = pdba->pdbinfo[pdbi[i].index];
    copy_rvec((*x)[pdbi[i].index],(*xnew)[i]);
     /* make indexgroup in block */
    a[i]=pdbi[i].index;
  }
  /* clean up */
  sfree(pdba->atomname);
  sfree(pdba->atom);
  sfree(pdba->pdbinfo);
  done_block(&pdba->excl);
  sfree(pdba);
  sfree(*x);
  /* copy the sorted pdbnew back to pdba */
  *pdbaptr=pdbnew;
  *x=*xnew;
  add_grp(block, gnames, natoms, a, "prot_sort");
  sfree(xnew);
  sfree(a);
  sfree(pdbi);
}

static int remove_duplicate_atoms(t_atoms *pdba,rvec x[])
{
  int     i,j,oldnatoms;
  
  printf("Checking for duplicate atoms....\n");
  oldnatoms    = pdba->nr;
  
  /* NOTE: pdba->nr is modified inside the loop */
  for(i=1; (i < pdba->nr); i++) {
    /* compare 'i' and 'i-1', throw away 'i' if they are identical 
       this is a 'while' because multiple alternate locations can be present */
    while ( (i < pdba->nr) &&
	    (pdba->atom[i-1].resnr == pdba->atom[i].resnr) &&
	    (strcmp(*pdba->atomname[i-1],*pdba->atomname[i])==0) ) {
      printf("deleting duplicate atom %4s  %s%4d",
	     *pdba->atomname[i], *pdba->resname[pdba->atom[i].resnr], 
	     pdba->atom[i].resnr+1);
      if (pdba->atom[i].chain && (pdba->atom[i].chain!=' '))
	printf(" ch %c", pdba->atom[i].chain);
      if (pdba->pdbinfo) {
	if (pdba->pdbinfo[i].atomnr)
	  printf("  pdb nr %4d",pdba->pdbinfo[i].atomnr);
	if (pdba->pdbinfo[i].altloc && (pdba->pdbinfo[i].altloc!=' '))
	  printf("  altloc %c",pdba->pdbinfo[i].altloc);
      }
      printf("\n");
      pdba->nr--;
      sfree(pdba->atomname[i]);
      for (j=i; j < pdba->nr; j++) {
	pdba->atom[j]     = pdba->atom[j+1];
	pdba->atomname[j] = pdba->atomname[j+1];
	pdba->pdbinfo[j]  = pdba->pdbinfo[j+1];
	copy_rvec(x[j+1],x[j]);
      }
      srenew(pdba->atom,     pdba->nr);
      srenew(pdba->atomname, pdba->nr);
      srenew(pdba->pdbinfo,  pdba->nr);
    }
  }
  if (pdba->nr != oldnatoms)
    printf("Now there are %d atoms\n",pdba->nr);
  
  return pdba->nr;
}

void find_nc_ter(t_atoms *pdba,int r0,int r1,int *rn,int *rc)
{
  int rnr;
  
  *rn=-1;
  *rc=-1;
  for(rnr=r0; rnr<r1; rnr++) {
    if ((*rn == -1) && (is_protein(*pdba->resname[rnr])))
	*rn=rnr;
    if ((*rc != rnr) && (is_protein(*pdba->resname[rnr])))
      *rc=rnr;
  }
  if (debug) fprintf(debug,"nres: %d, rN: %d, rC: %d\n",pdba->nres,*rn,*rc);
}

int main(int argc, char *argv[])
{
  static char *desc[] = {
    "This program reads a pdb file, lets you choose a forcefield, reads",
    "some database files, adds hydrogens to the molecules and generates",
    "coordinates in Gromacs (Gromos) format and a topology in Gromacs format.",
    "These files can subsequently be processed to generate a run input file.",
    "[PAR]",
    
    "Note that a pdb file is nothing more than a file format, and it",
    "need not necessarily contain a protein structure. Every kind of",
    "molecule for which there is support in the database can be converted.",
    "If there is no support in the database, you can add it yourself.[PAR]",
    
    "The program has limited intelligence, it reads a number of database",
    "files, that allow it to make special bonds (Cys-Cys, Heme-His, etc.),",
    "if necessary this can be done manually. The program can prompt the",
    "user to select which kind of LYS, ASP, GLU, CYS or HIS residue she",
    "wants. For LYS the choice is between LYS (two protons on NZ) or LYSH",
    "(three protons, default), for ASP and GLU unprotonated (default) or",
    "protonated, for HIS the proton can be either on ND1 (HISA), on NE2",
    "(HISB) or on both (HISH). By default these selections are done",
    "automatically. For His, this is based on an optimal hydrogen bonding",
    "conformation. Hydrogen bonds are defined based on a simple geometric",
    "criterium, specified by the maximum hydrogen-donor-acceptor angle",
    "and donor-acceptor distance, which are set by [TT]-angle[tt] and",
    "[TT]-dist[tt] respectively.[PAR]",

    "Option [TT]-merge[tt] will ask if you want to merge consecutive chains",
    "into one molecule, this can be useful for connecting chains with a",
    "disulfide brigde.[PAR]",
    
    "pdb2gmx will also check the occupancy field of the pdb file.",
    "If any of the occupanccies are not one, indicating that the atom is",
    "not resolved well in the structure, a warning message is issued.",
    "When a pdb file does not originate from an X-Ray structure determination",
    "all occupancy fields may be zero. Either way, it is up to the user",
    "to verify the correctness of the input data (read the article!).[PAR]", 
    
    "During processing the atoms will be reordered according to Gromacs",
    "conventions. With [TT]-n[tt] an index file can be generated that",
    "contains one group reordered in the same way. This allows you to",
    "convert a Gromos trajectory and coordinate file to Gromos. There is",
    "one limitation: reordering is done after the hydrogens are stripped",
    "from the input and before new hydrogens are added. This means that",
    "you should not use [TT]-ignh[tt].[PAR]",

    "The [TT].gro[tt] and [TT].g96[tt] file formats do not support chain",
    "identifiers. Therefore it is useful to enter a pdb file name at",
    "the [TT]-o[tt] option when you want to convert a multichain pdb file.",
    "[PAR]",
    
    "[TT]-sort[tt] will sort all residues according to the order in the",
    "database, sometimes this is necessary to get charge groups",
    "together.[PAR]",
    
    "[TT]-alldih[tt] will generate all proper dihedrals instead of only",
    "those with as few hydrogens as possible, this is useful for use with",
    "the Charmm forcefield.[PAR]",
    
    "The option [TT]-dummy[tt] removes hydrogen and fast improper dihedral",
    "motions. Angular and out-of-plane motions can be removed by changing",
    "hydrogens into dummy atoms and fixing angles, which fixes their",
    "position relative to neighboring atoms. Additionally, all atoms in the",
    "aromatic rings of the standard amino acids (i.e. PHE, TRP, TYR and HIS)",
    "can be converted into dummy atoms, elminating the fast improper dihedral",
    "fluctuations in these rings. Note that in this case all other hydrogen",
    "atoms are also converted to dummy atoms. The mass of all atoms that are",
    "converted into dummy atoms, is added to the heavy atoms.[PAR]",
    "Also slowing down of dihedral motion can be done with [TT]-heavyh[tt]",
    "done by increasing the hydrogen-mass by a factor of 4. This is also",
    "done for water hydrogens to slow down the rotational motion of water.",
    "The increase in mass of the hydrogens is subtracted from the bonded",
    "(heavy) atom so that the total mass of the system remains the same.",
    "Reference Feenstra et al., J. Comput. Chem. 20, 786 (1999)."
  };

  typedef struct {
    char chain;
    int  start;
    int  natom;
    bool bAllWat;
    int  nterpairs;
    int  *chainstart;
  } t_pdbchain;

  typedef struct {
    char chain;
    bool bAllWat;
    int nterpairs;
    int *chainstart;
    t_hackblock **ntdb;
    t_hackblock **ctdb;
    int *rN;
    int *rC;
    t_atoms *pdba;
    rvec *x;
  } t_chain;
  
  FILE       *fp,*top_file,*top_file2,*itp_file=NULL;
  int        natom,nres;
  t_atoms    pdba_all,*pdba;
  t_atoms    *atoms;
  t_block    *block;
  int        chain,nch,maxch,nwaterchain;
  t_pdbchain *pdb_ch;
  t_chain    *chains,*cc;
  char       pchain,select[STRLEN];
  int        nincl,nmol;
  char       **incls;
  t_mols     *mols;
  char       **gnames;
  matrix     box;
  rvec       box_space;
  char       *ff;
  int        i,j,k,l,nrtp;
  int        *swap_index,si;
  int        bts[ebtsNR];
  t_restp    *restp;
  t_hackblock *ah;
  t_symtab   symtab;
  t_atomtype *atype;
  char       fn[256],*top_fn,itp_fn[STRLEN],posre_fn[STRLEN],buf_fn[STRLEN];
  char       molname[STRLEN],title[STRLEN];
  char       *c;
  int        nah,nNtdb,nCtdb;
  t_hackblock *ntdb,*ctdb;
  int        nssbonds;
  t_ssbond   *ssbonds;
  rvec       *pdbx,*x;
  bool       bUsed,bDummies=FALSE,bWat,bPrevWat=FALSE,bITP,bDummyAromatics=FALSE;
  real       mHmult=0;
  
  t_filenm   fnm[] = { 
    { efSTX, "-f", "eiwit.pdb", ffREAD  },
    { efSTO, "-o", "conf",      ffWRITE },
    { efTOP, NULL, NULL,        ffWRITE },
    { efITP, "-i", "posre",     ffWRITE },
    { efNDX, "-n", "clean",     ffOPTWR },
    { efSTO, "-q", "clean.pdb", ffOPTWR }
  };
#define NFILE asize(fnm)
  
  /* Command line arguments must be static */
  static bool bNewRTP=FALSE,bMerge=FALSE;
  static bool bInter=FALSE, bCysMan=FALSE; 
  static bool bLysMan=FALSE, bAspMan=FALSE, bGluMan=FALSE, bHisMan=FALSE;
  static bool bTerMan=FALSE, bUnA=FALSE, bHeavyH;
  static bool bH14=FALSE, bSort=TRUE, bRemoveH=FALSE;
  static bool bAlldih=FALSE;
  static bool bDeuterate=FALSE;
  static real angle=135.0, distance=0.3,posre_fc=1000;
  static real long_bond_dist=0.25, short_bond_dist=0.05;
  static char *dumstr[] = { NULL, "none", "hydrogens", "aromatics", NULL };
  t_pargs pa[] = {
    { "-newrtp", FALSE, etBOOL, {&bNewRTP},
      "HIDDENWrite the residue database in new format to 'new.rtp'"},
    { "-lb",     FALSE, etREAL, {&long_bond_dist},
      "HIDDENLong bond warning distance" },
    { "-sb",     FALSE, etREAL, {&short_bond_dist},
      "HIDDENShort bond warning distance" },
    { "-merge", FALSE, etBOOL, {&bMerge},
      "Merge multiple chains into one molecule"},
    { "-inter",  FALSE, etBOOL, {&bInter},
      "Set the next 6 options to interactive"},
    { "-ss",     FALSE, etBOOL, {&bCysMan}, 
      "Interactive SS bridge selection" },
    { "-ter",    FALSE, etBOOL, {&bTerMan}, 
      "Interactive termini selection, iso charged" },
    { "-lys",    FALSE, etBOOL, {&bLysMan}, 
      "Interactive Lysine selection, iso charged" },
    { "-asp",    FALSE, etBOOL, {&bAspMan}, 
      "Interactive Aspartic Acid selection, iso charged" },
    { "-glu",    FALSE, etBOOL, {&bGluMan}, 
      "Interactive Glutamic Acid selection, iso charged" },
    { "-his",    FALSE, etBOOL, {&bHisMan},
      "Interactive Histidine selection, iso checking H-bonds" },
    { "-angle",  FALSE, etREAL, {&angle}, 
      "Minimum hydrogen-donor-acceptor angle for a H-bond (degrees)" },
    { "-dist",   FALSE, etREAL, {&distance},
      "Maximum donor-acceptor distance for a H-bond (nm)" },
    { "-posrefc",FALSE, etREAL, {&posre_fc},
      "Force constant for position restraints" },
    { "-una",    FALSE, etBOOL, {&bUnA}, 
      "Select aromatic rings with united CH atoms on Phenylalanine, "
      "Tryptophane and Tyrosine" },
    { "-sort",   FALSE, etBOOL, {&bSort}, 
      "Sort the residues according to database" },
    { "-H14",    FALSE, etBOOL, {&bH14}, 
      "Use 1-4 interactions between hydrogen atoms" },
    { "-ignh",   FALSE, etBOOL, {&bRemoveH}, 
      "Ignore hydrogen atoms that are in the pdb file" },
    { "-alldih", FALSE, etBOOL, {&bAlldih}, 
      "Generate all proper dihedrals" },
    { "-dummy",  FALSE, etENUM, {dumstr}, 
      "Convert atoms to dummy atoms" },
    { "-heavyh", FALSE, etBOOL, {&bHeavyH},
      "Make hydrogen atoms heavy" },
    { "-deuterate", FALSE, etBOOL, {&bDeuterate},
      "Change the mass of hydrogens to 2 amu" }
  };
#define NPARGS asize(pa)
  
  CopyRight(stderr,argv[0]);
  parse_common_args(&argc,argv,0,NFILE,fnm,asize(pa),pa,asize(desc),desc,
		    0,NULL);
		    
  if (bInter) {
    /* if anything changes here, also change description of -inter */
    bCysMan = TRUE;
    bTerMan = TRUE;
    bLysMan = TRUE;
    bAspMan = TRUE;
    bGluMan = TRUE;
    bHisMan = TRUE;
  }
  
  if (bHeavyH)
    mHmult=4.0;
  else if (bDeuterate)
    mHmult=2.0;
  else
    mHmult=1.0;
  
  switch(dumstr[0][0]) {
  case 'n': /* none */
    bDummies=FALSE;
    bDummyAromatics=FALSE;
    break;
  case 'h': /* hydrogens */
    bDummies=TRUE;
    bDummyAromatics=FALSE;
    break;
  case 'a': /* aromatics */
    bDummies=TRUE;
    bDummyAromatics=TRUE;
    break;
  default:
    fatal_error(0,"DEATH HORROR in $s (%d): dumstr[0]='%s'",
		__FILE__,__LINE__,dumstr[0]);
  }/* end switch */
  if (bDummies) please_cite(stdout,"Feenstra99");
  
  /* Open the symbol table */
  open_symtab(&symtab);
  
  clear_mat(box);
  natom=read_pdball(opt2fn("-f",NFILE,fnm),opt2fn_null("-q",NFILE,fnm),title,
		    &pdba_all,&pdbx,box,bRemoveH,&symtab);
  
  if (natom==0)
    fatal_error(0,"No atoms found in pdb file %s\n",opt2fn("-f",NFILE,fnm));

  printf("Analyzing pdb file\n");
  nch=0;
  maxch=0;
  nwaterchain=0;
  /* keep the compiler happy */
  pchain='?';
  pdb_ch=NULL;
  for (i=0; (i<natom); i++) {
    bWat = strcasecmp(*pdba_all.resname[pdba_all.atom[i].resnr],"HOH") == 0;
    if ((i==0) || (pdba_all.atom[i].chain!=pchain) || (bWat != bPrevWat)) {
      if (bMerge && i>0 && !bWat) {
	printf("Merge chain '%c' and '%c'? (n/y) ",
	       pchain,pdba_all.atom[i].chain);
	fgets(select,STRLEN-1,stdin);
      } 
      else
	select[0] = 'n';
      pchain=pdba_all.atom[i].chain;
      if (select[0] == 'y') {
	pdb_ch[nch-1].chainstart[pdb_ch[nch-1].nterpairs] = 
	  pdba_all.atom[i].resnr;
	pdb_ch[nch-1].nterpairs++;
	srenew(pdb_ch[nch-1].chainstart,pdb_ch[nch-1].nterpairs+1);
      } else {
	/* set natom for previous chain */
	if (nch > 0)
	  pdb_ch[nch-1].natom=i-pdb_ch[nch-1].start;
	if (bWat) {
	  nwaterchain++;
	  pdba_all.atom[i].chain='\0';
	}
	/* check if chain identifier was used before */
	for (j=0; (j<nch); j++)
	  if ((pdb_ch[j].chain != '\0') && (pdb_ch[j].chain != ' ') &&
	      (pdb_ch[j].chain == pdba_all.atom[i].chain))
	    fatal_error(0,"Chain identifier '%c' was used "
			"in two non-sequential blocks (residue %d, atom %d)",
			pdba_all.atom[i].chain,pdba_all.atom[i].resnr+1,i+1);
	if (nch == maxch) {
	  maxch += 16;
	  srenew(pdb_ch,maxch);
	}
	pdb_ch[nch].chain=pdba_all.atom[i].chain;
	pdb_ch[nch].start=i;
	pdb_ch[nch].bAllWat=bWat;
	if (bWat)
	  pdb_ch[nch].nterpairs=0;
	else
	  pdb_ch[nch].nterpairs=1;
	snew(pdb_ch[nch].chainstart,pdb_ch[nch].nterpairs+1);
	/* modified [nch] to [0] below */
	pdb_ch[nch].chainstart[0]=0;
	nch++;
      }
    }
    bPrevWat=bWat;
  }
  pdb_ch[nch-1].natom=natom-pdb_ch[nch-1].start;
  
  /* set all the water blocks at the end of the chain */
  snew(swap_index,nch);
  j=0;
  for(i=0; i<nch; i++)
    if (!pdb_ch[i].bAllWat) {
      swap_index[j]=i;
      j++;
    }
  for(i=0; i<nch; i++)
    if (pdb_ch[i].bAllWat) {
      swap_index[j]=i;
      j++;
    }
  if (nwaterchain>1)
    printf("Moved all the water blocks to the end\n");

  snew(chains,nch);
  /* copy pdb data and x for all chains */
  for (i=0; (i<nch); i++) {
    si=swap_index[i];
    chains[i].chain   = pdb_ch[si].chain;
    chains[i].bAllWat = pdb_ch[si].bAllWat;
    chains[i].nterpairs = pdb_ch[si].nterpairs;
    chains[i].chainstart = pdb_ch[si].chainstart;
    snew(chains[i].ntdb,pdb_ch[si].nterpairs);
    snew(chains[i].ctdb,pdb_ch[si].nterpairs);
    snew(chains[i].rN,pdb_ch[si].nterpairs);
    snew(chains[i].rC,pdb_ch[si].nterpairs);
    /* check for empty chain identifiers */
    if ((nch-nwaterchain>1) && !pdb_ch[si].bAllWat && 
	((chains[i].chain=='\0') || (chains[i].chain==' '))) {
      bUsed=TRUE;
      for(k='A'; (k<='Z') && bUsed; k++) {
	bUsed=FALSE;
	for(j=0; j<nch; j++)
	  bUsed = bUsed || pdb_ch[j].chain==k || chains[j].chain==k;
	if (!bUsed) {
	  printf("Gave chain %d chain identifier '%c'\n",i+1,k);
	  chains[i].chain=k;
	}
      }
    }
    snew(chains[i].pdba,1);
    init_t_atoms(chains[i].pdba,pdb_ch[si].natom,TRUE);
    snew(chains[i].x,chains[i].pdba->nr);
    for (j=0; j<chains[i].pdba->nr; j++) {
      chains[i].pdba->atom[j]=pdba_all.atom[pdb_ch[si].start+j];
      snew(chains[i].pdba->atomname[j],1);
      *chains[i].pdba->atomname[j] = 
	strdup(*pdba_all.atomname[pdb_ch[si].start+j]);
      /* make all chain identifiers equal to that off the chain */
      chains[i].pdba->atom[j].chain=pdb_ch[si].chain;
      chains[i].pdba->pdbinfo[j]=pdba_all.pdbinfo[pdb_ch[si].start+j];
      copy_rvec(pdbx[pdb_ch[si].start+j],chains[i].x[j]);
    }
    /* Renumber the residues assuming that the numbers are continuous */
    k    = chains[i].pdba->atom[0].resnr;
    nres = chains[i].pdba->atom[chains[i].pdba->nr-1].resnr - k + 1;
    chains[i].pdba->nres = nres;
    for(j=0; j < chains[i].pdba->nr; j++)
      chains[i].pdba->atom[j].resnr -= k;
    srenew(chains[i].pdba->resname,nres);
    for(j=0; j<nres; j++) {
      snew(chains[i].pdba->resname[j],1);
      *chains[i].pdba->resname[j] = strdup(*pdba_all.resname[k+j]);
    }
  }

  if (bMerge)
    printf("\nMerged %d chains into one molecule\n\n",
	   pdb_ch[0].nterpairs);

  printf("There are %d chains and %d blocks of water and "
	 "%d residues with %d atoms\n",
	 nch-nwaterchain,nwaterchain,
	 pdba_all.atom[natom-1].resnr+1,natom);
	  
  printf("\n  %5s  %4s %6s\n","chain","#res","#atoms");
  for (i=0; (i<nch); i++)
    printf("  %d '%c' %5d %6d  %s\n",
	   i+1, chains[i].chain ? chains[i].chain:'-',
	   chains[i].pdba->nres, chains[i].pdba->nr,
	   chains[i].bAllWat ? "(only water)":"");
  printf("\n");
  
  check_occupancy(&pdba_all,opt2fn("-f",NFILE,fnm));
  
  ff=choose_ff();
  printf("Using %s force field\n",ff);
  
  /* Read atomtypes... */
  atype=read_atype(ff,&symtab);
    
  /* read residue database */
  printf("Reading residue database... (%s)\n",ff);
  nrtp=read_resall(ff,bts,&restp,atype,&symtab);
  if (bNewRTP) {
    fp=ffopen("new.rtp","w");
    print_resall(fp,bts,nrtp,restp,atype);
    fclose(fp);
  }
    
  /* read hydrogen database */
  nah=read_h_db(ff,&ah);
  
  /* Read Termini database... */
  nNtdb=read_ter_db(ff,'n',&ntdb,atype);
  nCtdb=read_ter_db(ff,'c',&ctdb,atype);
  
  top_fn=ftp2fn(efTOP,NFILE,fnm);
  top_file=ffopen(top_fn,"w");
  print_top_header(top_file,top_fn,title,FALSE,ff,mHmult);

  nincl=0;
  nmol=0;
  incls=NULL;
  mols=NULL;
  nres=0;
  for(chain=0; (chain<nch); chain++) {
    cc = &(chains[chain]);

    /* set pdba, natom and nres to the current chain */
    pdba =cc->pdba;
    x    =cc->x;
    natom=cc->pdba->nr;
    nres =cc->pdba->nres;
    
    if (cc->chain && ( cc->chain != ' ' ) )
      printf("Processing chain %d '%c' (%d atoms, %d residues)\n",
	      chain+1,cc->chain,natom,nres);
    else
      printf("Processing chain %d (%d atoms, %d residues)\n",
	      chain+1,natom,nres);

    process_chain(pdba,x,bUnA,bUnA,bUnA,bLysMan,bAspMan,bGluMan,
		  bHisMan,bCysMan,&nssbonds,&ssbonds,angle,distance,&symtab);
		  
    if (bSort) {
      block = new_block();
      snew(gnames,1);
      sort_pdbatoms(nrtp,restp,natom,&pdba,&x,block,&gnames);
      natom = remove_duplicate_atoms(pdba,x);
      if (ftp2bSet(efNDX,NFILE,fnm)) {
	if (bRemoveH)
	  fprintf(stderr,"WARNING: with the -remh option the generated "
		  "index file (%s) might be useless\n"
		  "(the index file is generated before hydrogens are added)",
		  ftp2fn(efNDX,NFILE,fnm));
	write_index(ftp2fn(efNDX,NFILE,fnm),block,gnames);
      }
      for(i=0; i < block->nr; i++)
	sfree(gnames[i]);
      sfree(gnames);
      done_block(block);
    } else 
      fprintf(stderr,"WARNING: "
	      "without sorting no check for duplicate atoms can be done\n");
    
    if (debug) {
      if ( cc->chain == '\0' || cc->chain == ' ')
	sprintf(fn,"chain.pdb");
      else
	sprintf(fn,"chain_%c.pdb",cc->chain);
      write_sto_conf(fn,title,pdba,x,NULL,box);
    }

    for(i=0; i<cc->nterpairs; i++) {
      cc->chainstart[cc->nterpairs] = pdba->nres;
      find_nc_ter(pdba,cc->chainstart[i],cc->chainstart[i+1],
		  &(cc->rN[i]),&(cc->rC[i]));    
      
      if ( (cc->rN[i]<0) || (cc->rC[i]<0) ) {
	printf("No N- or C-terminus found: "
	       "this chain appears to contain no protein\n");
	cc->nterpairs = 0;
      } else {
	/* set termini */
	if ( (cc->rN[i]>=0) && (bTerMan || (nNtdb<4)) )
	  cc->ntdb[i]=choose_ter(nNtdb,ntdb,"Select N-terminus type (start)");
	  else
	    if (strncmp(*pdba->resname[pdba->atom[cc->rN[i]].resnr],"PRO",3))
	      cc->ntdb[i] = &(ntdb[1]);
	    else
	      cc->ntdb[i] = &(ntdb[3]);
	printf("N-terminus: %s\n",(cc->ntdb[i])->name);
	
	if ( (cc->rC[i]>=0) && (bTerMan || (nCtdb<2)) )
	   cc->ctdb[i] = choose_ter(nCtdb,ctdb,"Select C-terminus type (end)");
	else
	  cc->ctdb[i] = &(ctdb[1]);
	printf("C-terminus: %s\n",(cc->ctdb[i])->name);
      }
    }

    /* Generate Hydrogen atoms (and termini) in the sequence */
    natom=add_h(&pdba,&x,nah,ah,
		cc->nterpairs,cc->ntdb,cc->ctdb,cc->rN,cc->rC,
		NULL,NULL,TRUE,FALSE);
    printf("Now there are %d residues with %d atoms\n",
	   pdba->nres,pdba->nr);
    if (debug) write_pdbfile(debug,title,pdba,x,box,0,0);

    if (debug)
      for(i=0; (i<natom); i++)
	fprintf(debug,"Res %s%d atom %d %s\n",
		*(pdba->resname[pdba->atom[i].resnr]),
		pdba->atom[i].resnr+1,i+1,*pdba->atomname[i]);
    
    strcpy(posre_fn,ftp2fn(efITP,NFILE,fnm));
    
    /* make up molecule name(s) */
    if (cc->bAllWat) 
      sprintf(molname,"Water");
    else if ( cc->chain == '\0' || cc->chain == ' ' )
      sprintf(molname,"Protein");
    else
      sprintf(molname,"Protein_%c",cc->chain);
    
    if ((nch-nwaterchain>1) && !cc->bAllWat) {
      bITP=TRUE;
      strcpy(itp_fn,top_fn);
      printf("Chain time...\n");
      c=strrchr(itp_fn,'.');
      if ( cc->chain == '\0' || cc->chain == ' ' )
	sprintf(c,".itp");
      else
	sprintf(c,"_%c.itp",cc->chain);
      c=strrchr(posre_fn,'.');
      if ( cc->chain == '\0' || cc->chain == ' ' )
	sprintf(c,".itp");
      else
	sprintf(c,"_%c.itp",cc->chain);
      if (strcmp(itp_fn,posre_fn) == 0) {
	strcpy(buf_fn,posre_fn);
	c  = strrchr(buf_fn,'.');
	*c = '\0';
	sprintf(posre_fn,"%s_pr.itp",buf_fn);
      }
      
      nincl++;
      srenew(incls,nincl);
      incls[nincl-1]=strdup(itp_fn);
      itp_file=ffopen(itp_fn,"w");
    } else
      bITP=FALSE;

    srenew(mols,nmol+1);
    if (cc->bAllWat) {
      mols[nmol].name = strdup("SOL");
      mols[nmol].nr   = pdba->nres;
    } else {
      mols[nmol].name = strdup(molname);
      mols[nmol].nr   = 1;
    }
    nmol++;

    if (bITP)
      print_top_comment(itp_file,itp_fn,title,TRUE);

    if (cc->bAllWat)
      top_file2=NULL;
    else
      if (bITP)
	top_file2=itp_file;
      else
	top_file2=top_file;
    
    pdb2top(top_file2,posre_fn,molname,pdba,&x,atype,&symtab,bts,nrtp,restp,
	    cc->nterpairs,cc->ntdb,cc->ctdb,cc->rN,cc->rC,bH14,bAlldih,
	    bDummies,bDummyAromatics,mHmult,nssbonds,ssbonds,NREXCL, 
	    long_bond_dist, short_bond_dist,bDeuterate);
    
    if (!cc->bAllWat)
      write_posres(posre_fn,pdba,posre_fc);

    if (bITP)
      fclose(itp_file);

    /* pdba and natom have been reassigned somewhere so: */
    cc->pdba = pdba;
    cc->x = x;
    
    if (debug) {
      if ( cc->chain == '\0' || cc->chain == ' ' )
	sprintf(fn,"chain.pdb");
      else
	sprintf(fn,"chain_%c.pdb",cc->chain);
      write_sto_conf(fn,cool_quote(),pdba,x,NULL,box);
    }
  }
  
  print_top_mols(top_file,title,nincl,incls,nmol,mols);
  fclose(top_file);
  
  /* now merge all chains back together */
  natom=0;
  nres=0;
  for (i=0; (i<nch); i++) {
    natom+=chains[i].pdba->nr;
    nres+=chains[i].pdba->nres;
  }
  snew(atoms,1);
  init_t_atoms(atoms,natom,FALSE);
  for(i=0; i < atoms->nres; i++)
    sfree(atoms->resname[i]);
  sfree(atoms->resname);
  atoms->nres=nres;
  snew(atoms->resname,nres);
  snew(x,natom);
  k=0;
  l=0;
  for (i=0; (i<nch); i++) {
    if (nch>1)
      printf("Including chain %d in system: %d atoms %d residues\n",
	     i+1,chains[i].pdba->nr,chains[i].pdba->nres);
    for (j=0; (j<chains[i].pdba->nr); j++) {
      atoms->atom[k]=chains[i].pdba->atom[j];
      atoms->atom[k].resnr+=l; /* l is processed nr of residues */
      atoms->atomname[k]=chains[i].pdba->atomname[j];
      atoms->atom[k].chain=chains[i].chain;
      copy_rvec(chains[i].x[j],x[k]);
      k++;
    }
    for (j=0; (j<chains[i].pdba->nres); j++) {
      atoms->resname[l]=chains[i].pdba->resname[j];
      l++;
    }
  }
  
  if (nch>1) {
    fprintf(stderr,"Now there are %d atoms and %d residues\n",k,l);
    print_sums(atoms, TRUE);
  }
  
  fprintf(stderr,"\nWriting coordinate file...\n");
  clear_rvec(box_space);
  if (box[0][0] == 0) 
    gen_box(0,atoms->nr,x,box,box_space,FALSE);
  write_sto_conf(ftp2fn(efSTO,NFILE,fnm),title,atoms,x,NULL,box);

  thanx(stderr);
  
  return 0;
}
