# Microsoft Developer Studio Project File - Name="divxenc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=divxenc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "divxenc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "divxenc.mak" CFG="divxenc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "divxenc - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "divxenc - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "divxenc - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\..\..\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "MPEG4IP" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "divxenc - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "..\..\..\include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "MPEG4IP" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "divxenc - Win32 Release"
# Name "divxenc - Win32 Debug"
# Begin Source File

SOURCE=.\bitstream.c
# End Source File
# Begin Source File

SOURCE=.\bitstream.h
# End Source File
# Begin Source File

SOURCE=.\divxenc.c
# End Source File
# Begin Source File

SOURCE=.\encore.c
# End Source File
# Begin Source File

SOURCE=.\encore.h
# End Source File
# Begin Source File

SOURCE=.\max_level.h
# End Source File
# Begin Source File

SOURCE=.\mom_access.c
# End Source File
# Begin Source File

SOURCE=.\mom_access.h
# End Source File
# Begin Source File

SOURCE=.\mom_structs.h
# End Source File
# Begin Source File

SOURCE=.\mom_util.c
# End Source File
# Begin Source File

SOURCE=.\mom_util.h
# End Source File
# Begin Source File

SOURCE=.\momusys.h
# End Source File
# Begin Source File

SOURCE=.\mot_code.c
# End Source File
# Begin Source File

SOURCE=.\mot_code.h
# End Source File
# Begin Source File

SOURCE=.\mot_est_comp.c
# End Source File
# Begin Source File

SOURCE=.\mot_est_comp.h
# End Source File
# Begin Source File

SOURCE=.\mot_est_mb.c
# End Source File
# Begin Source File

SOURCE=.\mot_est_mb.h
# End Source File
# Begin Source File

SOURCE=.\mot_util.c
# End Source File
# Begin Source File

SOURCE=.\mot_util.h
# End Source File
# Begin Source File

SOURCE=.\non_unix.h
# End Source File
# Begin Source File

SOURCE=.\putvlc.c
# End Source File
# Begin Source File

SOURCE=.\putvlc.h
# End Source File
# Begin Source File

SOURCE=.\rate_ctl.c
# End Source File
# Begin Source File

SOURCE=.\rate_ctl.h
# End Source File
# Begin Source File

SOURCE=.\text_bits.c
# End Source File
# Begin Source File

SOURCE=.\text_bits.h
# End Source File
# Begin Source File

SOURCE=.\text_code.c
# End Source File
# Begin Source File

SOURCE=.\text_code.h
# End Source File
# Begin Source File

SOURCE=.\text_code_mb.c
# End Source File
# Begin Source File

SOURCE=.\text_code_mb.h
# End Source File
# Begin Source File

SOURCE=.\text_dct.c
# End Source File
# Begin Source File

SOURCE=.\vlc.h
# End Source File
# Begin Source File

SOURCE=.\vm_common_defs.h
# End Source File
# Begin Source File

SOURCE=.\vm_enc_defs.h
# End Source File
# Begin Source File

SOURCE=.\vop_code.c
# End Source File
# Begin Source File

SOURCE=.\vop_code.h
# End Source File
# Begin Source File

SOURCE=.\zigzag.h
# End Source File
# End Target
# End Project
