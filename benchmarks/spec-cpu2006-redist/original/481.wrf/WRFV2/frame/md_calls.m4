!
! WRF io macro file
!
! This file is used to generate the series of 40 meta-data get and 
! put calls in the WRF I/O API.  It contains an M4 macro and then
! a series of invocations of the macro to generate the subroutine
! definitions, which are then included by the file module_io.F
!

! $1 = get|put $2=dom|var $3=type $4=[char] $5=td|ti

define( md_call_2,
`!--- $1_$2_$6_$3$4

SUBROUTINE wrf_$1_$2_$6_$3$4_$5 ( DataHandle,Element, ifelse($6,td,`DateStr,') ifelse($2,var,`Varname,') Data, ifelse($4,char,,`Count, ifelse($1,get,`Outcount,')') Status )
USE module_state_description
IMPLICIT NONE
INTEGER ,       INTENT(IN)  :: DataHandle
CHARACTER*(*) , INTENT(IN)  :: Element
ifelse($6,td,`CHARACTER*(*) , INTENT(IN)  :: DateStr')
ifelse($2,var,`CHARACTER*(*) , INTENT(IN)  :: VarName') 

! ifelse($4,char,`CHARACTER*(*)  :: Data', `ifelse($3,double,real*8,$3)  :: Data ifelse($5,arr,(*),)')
 ifelse($4,char,`CHARACTER*(*)  :: Data', `ifelse($3,double,real*8,$3)  :: Data ifelse($5,arr,(*),)')

ifelse($4,char,,`INTEGER ,       INTENT(IN)  :: Count')
ifelse($4,char,,`ifelse($1,get,`INTEGER ,       INTENT(OUT)  :: OutCount')')
INTEGER ,       INTENT(OUT) :: Status

#include <wrf_status_codes.h>
INTEGER                     :: len_of_str
LOGICAL                     :: for_out
INTEGER, EXTERNAL           :: use_package
LOGICAL, EXTERNAL           :: wrf_dm_on_monitor, multi_files, use_output_servers
INTEGER                     :: locCount

INTEGER io_form , Hndl

CALL wrf_debug( DEBUG_LVL, "module_io.F (md_calls.m4) : in wrf_$1_$2_$6_$3$4_$5 " )

ifelse($3,integer,`locCount = Count')
ifelse($3,real,`locCount = Count')
ifelse($3,logical,`locCount = Count')

Status = 0
CALL get_handle ( Hndl, io_form , for_out, DataHandle )
IF ( Hndl .GT. -1 ) THEN
  IF ( multi_files( io_form ) .OR. .NOT. (for_out .AND. use_output_servers()) ) THEN
    SELECT CASE ( use_package( io_form ) )
#ifdef INTERNAL_IO
      CASE ( IO_INTERNAL )
        CALL int_$1_$2_$6_$3$4 ( Hndl, Element, ifelse($6,td,`DateStr,') ifelse($2,var,`Varname,') Data, &
                              ifelse($4,char,,`locCount, ifelse($1,get,`Outcount,')') Status )
#endif
#ifdef NETCDF
      CASE ( IO_NETCDF   )
        IF ( multi_files(io_form) .OR.  wrf_dm_on_monitor() ) THEN
           CALL ext_ncd_$1_$2_$6_$3$4 ( Hndl, Element, ifelse($6,td,`DateStr,') ifelse($2,var,`Varname,') Data, &
                                 ifelse($4,char,,`locCount, ifelse($1,get,`Outcount,')') Status )
        ENDIF
        IF ( .NOT. multi_files(io_form) ) THEN
          ifelse($1,get,ifelse($3,integer,`CALL wrf_dm_bcast_bytes( locCount, IWORDSIZE )'))
          ifelse($1,get,ifelse($3,integer,`CALL wrf_dm_bcast_bytes( Data, IWORDSIZE*locCount )'))
          ifelse($1,get,ifelse($3,real,   `CALL wrf_dm_bcast_bytes( locCount, IWORDSIZE )'))
          ifelse($1,get,ifelse($3,real,   `CALL wrf_dm_bcast_bytes( Data, RWORDSIZE*locCount )'))
          ifelse($1,get,ifelse($3,logical,`CALL wrf_dm_bcast_bytes( locCount, IWORDSIZE )'))
          ifelse($1,get,ifelse($3,logical,`CALL wrf_dm_bcast_bytes( Data, LWORDSIZE*locCount )'))
          ifelse($1,get,ifelse($4,char,   `len_of_str = LEN(Data)'))
          ifelse($1,get,ifelse($4,char,   `CALL wrf_dm_bcast_bytes( len_of_str, IWORDSIZE )'))
          ifelse($1,get,ifelse($4,char,   `CALL wrf_dm_bcast_string( Data, len_of_str )'))
          CALL wrf_dm_bcast_bytes( Status, IWORDSIZE )
        ENDIF
#endif
#ifdef PHDF5
      CASE ( IO_PHDF5   )
        CALL ext_phdf5_$1_$2_$6_$3$4 ( Hndl, Element, ifelse($6,td,`DateStr,') ifelse($2,var,`Varname,') Data, &
                              ifelse($4,char,,`locCount, ifelse($1,get,`Outcount,')') Status )
#endif
#ifdef HDF
      CASE ( IO_HDF   )
        CALL ext_hdf_$1_$2_$6_$3$4 ( Hndl, Element, ifelse($6,td,`DateStr,') ifelse($2,var,`Varname,') Data, &
                              ifelse($4,char,,`locCount, ifelse($1,get,`Outcount,')') Status )
#endif
#ifdef XXX
      CASE ( IO_XXX   )
        CALL ext_xxx_$1_$2_$6_$3$4 ( Hndl, Element, ifelse($6,td,`DateStr,') ifelse($2,var,`Varname,') Data, &
                              ifelse($4,char,,`locCount, ifelse($1,get,`Outcount,')') Status )
#endif
#ifdef YYY
      CASE ( IO_YYY   )
        CALL ext_yyy_$1_$2_$6_$3$4 ( Hndl, Element, ifelse($6,td,`DateStr,') ifelse($2,var,`Varname,') Data, &
                              ifelse($4,char,,`locCount, ifelse($1,get,`Outcount,')') Status )
#endif
#ifdef INTIO
      CASE ( IO_INTIO   )
        IF ( multi_files(io_form) .OR.  wrf_dm_on_monitor() ) THEN
           CALL ext_int_$1_$2_$6_$3$4 ( Hndl, Element, ifelse($6,td,`DateStr,') ifelse($2,var,`Varname,') Data, &
                                 ifelse($4,char,,`locCount, ifelse($1,get,`Outcount,')') Status )
        ENDIF
        IF ( .NOT. multi_files(io_form) ) THEN
           ifelse($1,get,ifelse($3,integer,`CALL wrf_dm_bcast_bytes( locCount, IWORDSIZE )'))
           ifelse($1,get,ifelse($3,integer,`CALL wrf_dm_bcast_bytes( Data, IWORDSIZE*locCount )'))
           ifelse($1,get,ifelse($3,real,   `CALL wrf_dm_bcast_bytes( locCount, IWORDSIZE )'))
           ifelse($1,get,ifelse($3,real,   `CALL wrf_dm_bcast_bytes( Data, RWORDSIZE*locCount )'))
           ifelse($1,get,ifelse($3,logical,`CALL wrf_dm_bcast_bytes( locCount, IWORDSIZE )'))
           ifelse($1,get,ifelse($3,logical,`CALL wrf_dm_bcast_bytes( Data, LWORDSIZE*locCount )'))
           ifelse($1,get,ifelse($4,char,   `len_of_str = LEN(Data)'))
           ifelse($1,get,ifelse($4,char,   `CALL wrf_dm_bcast_bytes( len_of_str, IWORDSIZE )'))
           ifelse($1,get,ifelse($4,char,   `CALL wrf_dm_bcast_string( Data, len_of_str )'))
           CALL wrf_dm_bcast_bytes( Status, IWORDSIZE )
        ENDIF
#endif
      CASE DEFAULT
    END SELECT
  ELSE IF ( for_out .AND. use_output_servers() ) THEN
    CALL ext_quilt_$1_$2_$6_$3$4 ( Hndl, Element, ifelse($6,td,`DateStr,') ifelse($2,var,`Varname,') Data, &
                          ifelse($4,char,,`locCount, ifelse($1,get,`Outcount,')') Status )
  ELSE
    Status = 0
ENDIF
ELSE
  Status = WRF_ERR_FATAL_BAD_FILE_STATUS
ENDIF
RETURN
END SUBROUTINE wrf_$1_$2_$6_$3$4_$5' )

define( md_call,
`ifelse($4,char,
`md_call_2($1,$2,$3,$4,arr,$5)'
,
`md_call_2($1,$2,$3,$4,arr,$5)
md_call_2($1,$2,$3,$4,sca,$5)'
)'
)

define( md_interface,
`ifelse($4,char,
`INTERFACE wrf_$1_$2_$5_$3$4
  MODULE PROCEDURE wrf_$1_$2_$5_$3$4_arr
END INTERFACE'
,
`INTERFACE wrf_$1_$2_$5_$3$4
  MODULE PROCEDURE wrf_$1_$2_$5_$3$4_arr, wrf_$1_$2_$5_$3$4_sca
END INTERFACE'
)'
)

md_interface(get,dom,real,,ti)
md_interface(put,dom,real,,ti)
md_interface(get,dom,double,,ti)
md_interface(put,dom,double,,ti)
md_interface(get,dom,integer,,ti)
md_interface(put,dom,integer,,ti)
md_interface(get,dom,logical,,ti)
md_interface(put,dom,logical,,ti)
md_interface(get,dom,,char,ti)
md_interface(put,dom,,char,ti)

md_interface(get,dom,real,,td)
md_interface(put,dom,real,,td)
md_interface(get,dom,double,,td)
md_interface(put,dom,double,,td)
md_interface(get,dom,integer,,td)
md_interface(put,dom,integer,,td)
md_interface(get,dom,logical,,td)
md_interface(put,dom,logical,,td)
md_interface(get,dom,,char,td)
md_interface(put,dom,,char,td)

md_interface(get,var,real,,ti)
md_interface(put,var,real,,ti)
md_interface(get,var,double,,ti)
md_interface(put,var,double,,ti)
md_interface(get,var,integer,,ti)
md_interface(put,var,integer,,ti)
md_interface(get,var,logical,,ti)
md_interface(put,var,logical,,ti)
md_interface(get,var,,char,ti)
md_interface(put,var,,char,ti)

md_interface(get,var,real,,td)
md_interface(put,var,real,,td)
md_interface(get,var,double,,td)
md_interface(put,var,double,,td)
md_interface(get,var,integer,,td)
md_interface(put,var,integer,,td)
md_interface(get,var,logical,,td)
md_interface(put,var,logical,,td)
md_interface(get,var,,char,td)
md_interface(put,var,,char,td)

CONTAINS

md_call(get,dom,real,,ti)
md_call(put,dom,real,,ti)
md_call(get,dom,double,,ti)
md_call(put,dom,double,,ti)
md_call(get,dom,integer,,ti)
md_call(put,dom,integer,,ti)
md_call(get,dom,logical,,ti)
md_call(put,dom,logical,,ti)
md_call(get,dom,,char,ti)
md_call(put,dom,,char,ti)

md_call(get,dom,real,,td)
md_call(put,dom,real,,td)
md_call(get,dom,double,,td)
md_call(put,dom,double,,td)
md_call(get,dom,integer,,td)
md_call(put,dom,integer,,td)
md_call(get,dom,logical,,td)
md_call(put,dom,logical,,td)
md_call(get,dom,,char,td)
md_call(put,dom,,char,td)

md_call(get,var,real,,ti)
md_call(put,var,real,,ti)
md_call(get,var,double,,ti)
md_call(put,var,double,,ti)
md_call(get,var,integer,,ti)
md_call(put,var,integer,,ti)
md_call(get,var,logical,,ti)
md_call(put,var,logical,,ti)
md_call(get,var,,char,ti)
md_call(put,var,,char,ti)

md_call(get,var,real,,td)
md_call(put,var,real,,td)
md_call(get,var,double,,td)
md_call(put,var,double,,td)
md_call(get,var,integer,,td)
md_call(put,var,integer,,td)
md_call(get,var,logical,,td)
md_call(put,var,logical,,td)
md_call(get,var,,char,td)
md_call(put,var,,char,td)

