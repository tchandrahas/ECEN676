/*
 * $Id: dlg.c,v 1.6 2002/02/28 11:07:08 spoel Exp $
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
 * Gromacs Runs On Most of All Computer Systems
 */
static char *SRCID_dlg_c = "$Id: dlg.c,v 1.6 2002/02/28 11:07:08 spoel Exp $";
#include <stdio.h>
#include <stdlib.h>
#include <xdlghi.h>

int main(int argc, char *argv[])
{
  t_x11 *x11;
  t_dlg *dlg;

  if ((x11=GetX11(&argc,argv))==NULL) {
    fprintf(stderr,"No X!\n");
    exit(1);
  }
  if (argc > 1) {
    dlg=ReadDlg(x11,0,x11->title,x11->fg,x11->bg,argv[1],100,100,TRUE,
		TRUE,NULL,NULL);
    ShowDlg(dlg);
    x11->MainLoop(x11);
  }
  else 
    fprintf(stderr,"Usage: %s [ X options ] infile\n",argv[0]);

  x11->CleanUp(x11);

  return 0;
}
