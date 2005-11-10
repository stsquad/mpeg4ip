# Microsoft Developer Studio Project File - Name="libmp3" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libmp3 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libmp360.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libmp360.mak" CFG="libmp3 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libmp3 - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libmp3 - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libmp3 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libmp3__"
# PROP BASE Intermediate_Dir "libmp3__"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\..\..\include" /I "..\..\..\..\SDL\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libmp3 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libmp3_0"
# PROP BASE Intermediate_Dir "libmp3_0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "..\..\..\..\include" /I "..\..\..\..\SDL\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
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

# Name "libmp3 - Win32 Release"
# Name "libmp3 - Win32 Debug"
# Begin Source File

SOURCE=.\bitwindow.cpp
# End Source File
# Begin Source File

SOURCE=.\filter.cpp
# End Source File
# Begin Source File

SOURCE=.\filter_2.cpp
# End Source File
# Begin Source File

SOURCE=.\huffmantable.cpp
# End Source File
# Begin Source File

SOURCE=.\MPEGaction.h
# End Source File
# Begin Source File

SOURCE=.\MPEGaudio.cpp
# End Source File
# Begin Source File

SOURCE=.\MPEGaudio.h
# End Source File
# Begin Source File

SOURCE=.\MPEGerror.h
# End Source File
# Begin Source File

SOURCE=.\MPEGfilter.h
# End Source File
# Begin Source File

SOURCE=.\mpeglayer1.cpp
# End Source File
# Begin Source File

SOURCE=.\mpeglayer2.cpp
# End Source File
# Begin Source File

SOURCE=.\mpeglayer3.cpp
# End Source File
# Begin Source File

SOURCE=.\MPEGring.h
# End Source File
# Begin Source File

SOURCE=.\mpegtable.cpp
# End Source File
# Begin Source File

SOURCE=.\mpegtoraw.cpp
# End Source File
# Begin Source File

SOURCE=.\ourmpegaudio.cpp
# End Source File
# End Target
# End Project
