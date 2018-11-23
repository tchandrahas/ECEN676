# Definition of macros for library, and C startup code.
osedir     = $(osrdir)$(DIRSEPSTR)$(OSENVIRONMENT)

.IMPORT .IGNORE : MSC_VER
MSC_VER      *= 6.0

CFLAGS       += -I$(osedir) -D_MSC_VER=$(MSC_VER:s,.,,)0

NDB_CFLAGS   += -Osecgl -Gs
NDB_LDFLAGS  += -exe -packc -batch
NDB_LDLIBS  +=
