# Microsoft Developer Studio Project File - Name="libourdivx" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libourdivx - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libourdivx.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libourdivx.mak" CFG="libourdivx - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libourdivx - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libourdivx - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe

!IF  "$(CFG)" == "libourdivx - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_DECORE" /D "MPEG4IP" /D "POSTPROCESS" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libourdivx - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_DECORE" /D "MPEG4IP" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libourdivx - Win32 Release"
# Name "libourdivx - Win32 Debug"
# Begin Source File

SOURCE=.\basic_prediction.c
# End Source File
# Begin Source File

SOURCE=.\basic_prediction.h
# End Source File
# Begin Source File

SOURCE=.\divxif.h
# End Source File
# Begin Source File

SOURCE=.\getbits.c
# End Source File
# Begin Source File

SOURCE=.\global.h
# End Source File
# Begin Source File

SOURCE=.\idct.c
# End Source File
# Begin Source File

SOURCE=.\idct.h
# End Source File
# Begin Source File

SOURCE=.\mp4_block.c
# End Source File
# Begin Source File

SOURCE=.\mp4_block.h
# End Source File
# Begin Source File

SOURCE=.\mp4_decoder.c
# End Source File
# Begin Source File

SOURCE=.\mp4_decoder.h
# End Source File
# Begin Source File

SOURCE=.\mp4_header.c
# End Source File
# Begin Source File

SOURCE=.\mp4_header.h
# End Source File
# Begin Source File

SOURCE=.\mp4_mblock.c
# End Source File
# Begin Source File

SOURCE=.\mp4_mblock.h
# End Source File
# Begin Source File

SOURCE=.\mp4_picture.c
# End Source File
# Begin Source File

SOURCE=.\mp4_predict.c
# End Source File
# Begin Source File

SOURCE=.\mp4_predict.h
# End Source File
# Begin Source File

SOURCE=.\mp4_recon.c
# End Source File
# Begin Source File

SOURCE=.\mp4_vld.c
# End Source File
# Begin Source File

SOURCE=.\mp4_vld.h
# End Source File
# Begin Source File

SOURCE=.\newmain.c
# End Source File
# Begin Source File

SOURCE=.\our_assert.cpp
# End Source File
# Begin Source File

SOURCE=.\our_assert.h
# End Source File
# Begin Source File

SOURCE=.\portab.h
# End Source File
# Begin Source File

SOURCE=.\postprocess.c
# End Source File
# Begin Source File

SOURCE=.\postprocess.h
# End Source File
# Begin Source File

SOURCE=.\transferIDCT.c
# End Source File
# Begin Source File

SOURCE=.\transferIDCT.h
# End Source File
# Begin Source File

SOURCE=.\yuv2rgb.h
# End Source File
# End Target
# End Project
