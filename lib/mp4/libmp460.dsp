# Microsoft Developer Studio Project File - Name="libmp4" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libmp4 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libmp460.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libmp460.mak" CFG="libmp4 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libmp4 - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libmp4 - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libmp4 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libmp4 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libmp4 - Win32 Release"
# Name "libmp4 - Win32 Debug"
# Begin Source File

SOURCE=.\atom.c
# End Source File
# Begin Source File

SOURCE=.\ctab.c
# End Source File
# Begin Source File

SOURCE=.\ctts.c
# End Source File
# Begin Source File

SOURCE=.\dimm.c
# End Source File
# Begin Source File

SOURCE=.\dinf.c
# End Source File
# Begin Source File

SOURCE=.\dmax.c
# End Source File
# Begin Source File

SOURCE=.\dmed.c
# End Source File
# Begin Source File

SOURCE=.\dref.c
# End Source File
# Begin Source File

SOURCE=.\drep.c
# End Source File
# Begin Source File

SOURCE=.\edts.c
# End Source File
# Begin Source File

SOURCE=.\elst.c
# End Source File
# Begin Source File

SOURCE=.\esds.c
# End Source File
# Begin Source File

SOURCE=.\funcprotos.h
# End Source File
# Begin Source File

SOURCE=.\gmhd.c
# End Source File
# Begin Source File

SOURCE=.\gmin.c
# End Source File
# Begin Source File

SOURCE=.\hdlr.c
# End Source File
# Begin Source File

SOURCE=.\hinf.c
# End Source File
# Begin Source File

SOURCE=.\hint.c
# End Source File
# Begin Source File

SOURCE=.\hinthnti.c
# End Source File
# Begin Source File

SOURCE=.\hintudta.c
# End Source File
# Begin Source File

SOURCE=.\hmhd.c
# End Source File
# Begin Source File

SOURCE=.\hnti.c
# End Source File
# Begin Source File

SOURCE=.\iods.c
# End Source File
# Begin Source File

SOURCE=.\matrix.c
# End Source File
# Begin Source File

SOURCE=.\maxr.c
# End Source File
# Begin Source File

SOURCE=.\mdat.c
# End Source File
# Begin Source File

SOURCE=.\mdhd.c
# End Source File
# Begin Source File

SOURCE=.\mdia.c
# End Source File
# Begin Source File

SOURCE=.\minf.c
# End Source File
# Begin Source File

SOURCE=.\moov.c
# End Source File
# Begin Source File

SOURCE=.\mvhd.c
# End Source File
# Begin Source File

SOURCE=.\nump.c
# End Source File
# Begin Source File

SOURCE=.\payt.c
# End Source File
# Begin Source File

SOURCE=.\pmax.c
# End Source File
# Begin Source File

SOURCE=.\private.h
# End Source File
# Begin Source File

SOURCE=.\quicktime.c
# End Source File
# Begin Source File

SOURCE=.\quicktime.h
# End Source File
# Begin Source File

SOURCE=.\rtp.c
# End Source File
# Begin Source File

SOURCE=.\rtphint.c
# End Source File
# Begin Source File

SOURCE=.\rtphint.h
# End Source File
# Begin Source File

SOURCE=.\sdp.c
# End Source File
# Begin Source File

SOURCE=.\sizes.h
# End Source File
# Begin Source File

SOURCE=.\smhd.c
# End Source File
# Begin Source File

SOURCE=.\stbl.c
# End Source File
# Begin Source File

SOURCE=.\stco.c
# End Source File
# Begin Source File

SOURCE=.\stsc.c
# End Source File
# Begin Source File

SOURCE=.\stsd.c
# End Source File
# Begin Source File

SOURCE=.\stsdtable.c
# End Source File
# Begin Source File

SOURCE=.\stss.c
# End Source File
# Begin Source File

SOURCE=.\stsz.c
# End Source File
# Begin Source File

SOURCE=.\stts.c
# End Source File
# Begin Source File

SOURCE=.\tkhd.c
# End Source File
# Begin Source File

SOURCE=.\tmax.c
# End Source File
# Begin Source File

SOURCE=.\tmin.c
# End Source File
# Begin Source File

SOURCE=.\tpyl.c
# End Source File
# Begin Source File

SOURCE=.\trak.c
# End Source File
# Begin Source File

SOURCE=.\tref.c
# End Source File
# Begin Source File

SOURCE=.\trpy.c
# End Source File
# Begin Source File

SOURCE=.\udta.c
# End Source File
# Begin Source File

SOURCE=.\util.c
# End Source File
# Begin Source File

SOURCE=.\vmhd.c
# End Source File
# End Target
# End Project
