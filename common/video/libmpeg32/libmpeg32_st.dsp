# Microsoft Developer Studio Project File - Name="libmpeg32_st" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libmpeg32_st - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libmpeg32_st.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libmpeg32_st.mak" CFG="libmpeg32_st - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libmpeg32_st - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libmpeg32_st - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libmpeg32_st - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libmpeg32_st___Win32_Release"
# PROP BASE Intermediate_Dir "libmpeg32_st___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libmpeg32_st___Win32_Release"
# PROP Intermediate_Dir "libmpeg32_st___Win32_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "." /I "..\..\..\include" /I "..\..\..\lib\mp4v2" /I "..\..\..\lib\mp4av" /I "..\..\..\lib\sdl\include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_WIN32" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libmpeg32_st - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libmpeg32_st___Win32_Debug"
# PROP BASE Intermediate_Dir "libmpeg32_st___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "libmpeg32_st___Win32_Debug"
# PROP Intermediate_Dir "libmpeg32_st___Win32_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ  /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "." /I "..\..\..\include" /I "..\..\..\lib\mp4v2" /I "..\..\..\lib\mp4av" /I "..\..\..\lib\sdl\include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_WIN32" /YX /FD /GZ  /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libmpeg32_st - Win32 Release"
# Name "libmpeg32_st - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\libmpeg3.c
# End Source File
# Begin Source File

SOURCE=.\mpeg3atrack.c
# End Source File
# Begin Source File

SOURCE=.\mpeg3css.c
# End Source File
# Begin Source File

SOURCE=.\mpeg3demux.c
# End Source File
# Begin Source File

SOURCE=.\mpeg3io.c
# End Source File
# Begin Source File

SOURCE=.\mpeg3title.c
# End Source File
# Begin Source File

SOURCE=.\mpeg3vtrack.c
# End Source File
# Begin Source File

SOURCE=.\workarounds.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ifo.h
# End Source File
# Begin Source File

SOURCE=.\libmpeg3.h
# End Source File
# Begin Source File

SOURCE=.\mpeg3private.h
# End Source File
# Begin Source File

SOURCE=.\mpeg3private.inc
# End Source File
# Begin Source File

SOURCE=.\mpeg3protos.h
# End Source File
# Begin Source File

SOURCE=.\video\slice.h
# End Source File
# Begin Source File

SOURCE=.\timecode.h
# End Source File
# End Group
# End Target
# End Project
