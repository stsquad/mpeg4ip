# Microsoft Developer Studio Project File - Name="libfaad" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libfaad - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libfaad.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libfaad.mak" CFG="libfaad - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libfaad - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libfaad - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe

!IF  "$(CFG)" == "libfaad - Win32 Release"

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
RSC=rc.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libfaad - Win32 Debug"

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
RSC=rc.exe
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_CONSOLE" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libfaad - Win32 Release"
# Name "libfaad - Win32 Debug"
# Begin Source File

SOURCE=.\block.c
# End Source File
# Begin Source File

SOURCE=.\block.h
# End Source File
# Begin Source File

SOURCE=.\config.c
# End Source File
# Begin Source File

SOURCE=.\decdata.c
# End Source File
# Begin Source File

SOURCE=.\decoder.c
# End Source File
# Begin Source File

SOURCE=.\dolby_adapt.c
# End Source File
# Begin Source File

SOURCE=.\dolby_def.h
# End Source File
# Begin Source File

SOURCE=.\dolby_win.h
# End Source File
# Begin Source File

SOURCE=.\faad_all.h
# End Source File
# Begin Source File

SOURCE=.\fastfft.c

!IF  "$(CFG)" == "libfaad - Win32 Release"

# SUBTRACT CPP /O<none>

!ELSEIF  "$(CFG)" == "libfaad - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\fastfft.h
# End Source File
# Begin Source File

SOURCE=.\filestream.c
# End Source File
# Begin Source File

SOURCE=.\filestream.h
# End Source File
# Begin Source File

SOURCE=.\http.c
# End Source File
# Begin Source File

SOURCE=.\http.h
# End Source File
# Begin Source File

SOURCE=.\huffdec1.c
# End Source File
# Begin Source File

SOURCE=.\huffdec2.c
# End Source File
# Begin Source File

SOURCE=.\huffdec3.c
# End Source File
# Begin Source File

SOURCE=.\huffinit.c
# End Source File
# Begin Source File

SOURCE=.\hufftables.c
# End Source File
# Begin Source File

SOURCE=.\IN2.H
# End Source File
# Begin Source File

SOURCE=.\intensity.c
# End Source File
# Begin Source File

SOURCE=.\interface.h
# End Source File
# Begin Source File

SOURCE=.\nok_lt_prediction.c
# End Source File
# Begin Source File

SOURCE=.\nok_lt_prediction.h
# End Source File
# Begin Source File

SOURCE=.\nok_ltp_common.h
# End Source File
# Begin Source File

SOURCE=.\nok_ltp_common_internal.h
# End Source File
# Begin Source File

SOURCE=.\OUT.H
# End Source File
# Begin Source File

SOURCE=.\pns.c
# End Source File
# Begin Source File

SOURCE=.\port.h
# End Source File
# Begin Source File

SOURCE=.\portio.c
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\stdinc.h
# End Source File
# Begin Source File

SOURCE=.\stereo.c
# End Source File
# Begin Source File

SOURCE=.\tns.c
# End Source File
# Begin Source File

SOURCE=.\tns.h
# End Source File
# Begin Source File

SOURCE=.\transfo.c
# End Source File
# Begin Source File

SOURCE=.\transfo.h
# End Source File
# Begin Source File

SOURCE=.\wav.c
# End Source File
# Begin Source File

SOURCE=.\wav.h
# End Source File
# Begin Source File

SOURCE=.\weave.h
# End Source File
# End Target
# End Project
