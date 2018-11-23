/*
UPPERMET_guts.h

Macro to calculate the components of the upper physical metric,
and as an offspin the determinant of the conformal(?) metric. 

Gabrielle Allen, 11th June 1998

*/

#ifndef UPPERMET_GUTS
#define UPPERMET_GUTS

#include "CactusEinstein/Einstein/src/macro/DETG_guts.h"

#ifdef FCODE

      UPPERMET_PSI4DET = DETG_PSI4*DETG_DETCG

      UPPERMET_UXX = DETG_TEMPXX/UPPERMET_PSI4DET
      UPPERMET_UXY = DETG_TEMPXY/UPPERMET_PSI4DET
      UPPERMET_UXZ = DETG_TEMPXZ/UPPERMET_PSI4DET
      UPPERMET_UYY = DETG_TEMPYY/UPPERMET_PSI4DET
      UPPERMET_UYZ = DETG_TEMPYZ/UPPERMET_PSI4DET
      UPPERMET_UZZ = DETG_TEMPZZ/UPPERMET_PSI4DET

#endif

#ifdef CCODE

UPPERMET_PSI4DET = DETG_PSI4*DETG_DETCG;

UPPERMET_UXX = DETG_TEMPXX/UPPERMET_PSI4DET;
UPPERMET_UXY = DETG_TEMPXY/UPPERMET_PSI4DET;
UPPERMET_UXZ = DETG_TEMPXZ/UPPERMET_PSI4DET;
UPPERMET_UYY = DETG_TEMPYY/UPPERMET_PSI4DET;
UPPERMET_UYZ = DETG_TEMPYZ/UPPERMET_PSI4DET;
UPPERMET_UZZ = DETG_TEMPZZ/UPPERMET_PSI4DET;

#endif

#endif
