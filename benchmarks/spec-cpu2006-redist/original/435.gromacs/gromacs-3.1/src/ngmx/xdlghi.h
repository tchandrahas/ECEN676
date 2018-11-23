/*
 * $Id: xdlghi.h,v 1.7 2002/02/28 11:07:10 spoel Exp $
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
 * Glycine aRginine prOline Methionine Alanine Cystine Serine
 */

#ifndef _xdlghi_h
#define _xdlghi_h

static char *SRCID_xdlghi_h = "$Id: xdlghi.h,v 1.7 2002/02/28 11:07:10 spoel Exp $";
#ifdef HAVE_IDENT
#ident	"@(#) xdlghi.h 1.2 9/29/92"
#endif /* HAVE_IDENT */

#include <stdarg.h>
#include "Xstuff.h"
#include "x11.h"
#include "xdlg.h"

typedef struct {
  int       nitem;
  int       w,h;
  t_dlgitem **list;
} t_dlgitemlist;

extern t_dlgitem **CreateRadioButtonGroup(t_x11 *x11, char *szTitle, 
					  t_id GroupID, int nrb, t_id rb[],
					  int nSelect,
					  char *szRB[], int x0,int y0);
/* This routine creates a radio button group at the
 * specified position. The return values is a pointer to an
 * array of dlgitems, the array has length (nrb+1) with the +1
 * because of the groupbox.
 * nSelect is the ordinal of the selected button.
 */

extern t_dlgitem **CreateDlgitemGroup(t_x11 *x11, char *szTitle, 
				      t_id GroupID, int x0, int y0,
				      int nitem, ...);
/* This routine creates a dlgitem group at the
 * specified position. The return values is a pointer to an
 * array of dlgitems, the array has length (nitem+1) with the +1
 * because of the groupbox.
 */

extern t_dlg *ReadDlg(t_x11 *x11,Window Parent, char *title,
		      unsigned long fg, unsigned long bg, char *infile, 
		      int x0, int y0, bool bAutoPosition,bool bUseMon,
		      DlgCallback *cb,void *data);
/* Read a dialog box from a template file */

#endif	/* _xdlghi_h */
