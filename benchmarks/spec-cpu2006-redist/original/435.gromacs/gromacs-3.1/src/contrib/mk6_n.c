/*
 * $Id: mk6_n.c,v 1.4 2002/02/28 11:14:17 spoel Exp $
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
static char *SRCID_mk6_n_c = "$Id: mk6_n.c,v 1.4 2002/02/28 11:14:17 spoel Exp $";
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void doit(char *fn,double myexp[],int n,double tabscale)
{
  int    i,k;
  double myfac[3] = { 1, -1, 1 };
  double x,v,v2;
  FILE   *fp;
  
  fp = fopen(fn,"w");
  for(i=0; (i<=n); i++) {
    x   =  i/tabscale;
    
    fprintf(fp,"%10g",x);
    
    for(k=0; (k<3); k++) {
      if (x < 0.04) {
	/* Avoid very high numbers */
	v = v2 = 0;
      }
      else {
	v  =  myfac[k]*pow(x,-myexp[k]);
	v2 = (myexp[k]+1)*(myexp[k])*v/(x*x); 
      }
      fprintf(fp,"  %10g  %10g",v,v2);
    }
    fprintf(fp,"\n");
  }
  fclose(fp);
}

int main(int argc,char *argv[])
{
  double my8[3]  = { 1, 6, 8 };
  double my9[3]  = { 1, 6, 9 };
  double my10[3] = { 1, 6, 10 };
  double my11[3] = { 1, 6, 11 };
  double my12[3] = { 1, 6, 12 };
#ifdef DOUBLE
  double tabscale = 2000;
#else
  double tabscale = 500;
#endif
  int n = (int) (2.0*tabscale);
    
  doit("table6-8.xvg",my8,n,tabscale);
  doit("table6-9.xvg",my9,n,tabscale);
  doit("table6-10.xvg",my10,n,tabscale);
  doit("table6-11.xvg",my11,n,tabscale);
  doit("table6-12.xvg",my12,n,tabscale);
  
  return 0;
}

