# Microsoft Developer Studio Project File - Name="libfaad" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libfaad - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libfaad60.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libfaad60.mak" CFG="libfaad - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libfaad - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libfaad - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\..\..\include" /D "_MBCS" /D "_LIB" /D "WIN32" /D "NDEBUG" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
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
# ADD BASE CPP /nologo /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "..\\" /I "..\..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
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
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\bits.c
# End Source File
# Begin Source File

SOURCE=.\block.c
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

SOURCE=.\fastfft.c

!IF  "$(CFG)" == "libfaad - Win32 Release"

# ADD CPP /GX-
# SUBTRACT CPP /O<none>

!ELSEIF  "$(CFG)" == "libfaad - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\huffdec.c
# End Source File
# Begin Source File

SOURCE=.\huffinit.c
# End Source File
# Begin Source File

SOURCE=.\hufftables.c
# End Source File
# Begin Source File

SOURCE=.\intensity.c
# End Source File
# Begin Source File

SOURCE=.\monopred.c
# End Source File
# Begin Source File

SOURCE=.\nok_lt_prediction.c
# End Source File
# Begin Source File

SOURCE=.\pns.c
# End Source File
# Begin Source File

SOURCE=.\stereo.c
# End Source File
# Begin Source File

SOURCE=.\tns.c
# End Source File
# Begin Source File

SOURCE=.\transfo.c
# End Source File
# Begin Source File

SOURCE=.\util.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\All.h
# End Source File
# Begin Source File

SOURCE=.\bits.h
# End Source File
# Begin Source File

SOURCE=.\Block.h
# End Source File
# Begin Source File

SOURCE=.\Fastfft.h
# End Source File
# Begin Source File

SOURCE=.\interface.h
# End Source File
# Begin Source File

SOURCE=.\kbd_win.h
# End Source File
# Begin Source File

SOURCE=.\monopred.h
# End Source File
# Begin Source File

SOURCE=.\mpeg4ip_bits.h
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

SOURCE=.\Port.h
# End Source File
# Begin Source File

SOURCE=.\Tns.h
# End Source File
# Begin Source File

SOURCE=.\Transfo.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# End Group
# End Target
# End Project
