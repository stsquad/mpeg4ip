# Microsoft Developer Studio Project File - Name="librtsp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=librtsp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "librtsp60.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "librtsp60.mak" CFG="librtsp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "librtsp - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "librtsp - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "librtsp - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../../lib/rtp" /I "../../../include" /I "../../../SDL/include" /I "../../../lib" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "IPTV_COMPATIBLE" /D "_REENTRANT" /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "librtsp - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "../../../include" /I "../../../SDL/include" /I "../../../lib" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "IPTV_COMPATIBLE" /D "_REENTRANT" /FR /YX /FD /c
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

# Name "librtsp - Win32 Release"
# Name "librtsp - Win32 Debug"
# Begin Source File

SOURCE=.\rtsp.c
# End Source File
# Begin Source File

SOURCE=.\rtsp_client.h
# End Source File
# Begin Source File

SOURCE=.\rtsp_comm.c
# End Source File
# Begin Source File

SOURCE=.\rtsp_command.c
# End Source File
# Begin Source File

SOURCE=.\rtsp_private.h
# End Source File
# Begin Source File

SOURCE=.\rtsp_resp.c
# End Source File
# Begin Source File

SOURCE=.\rtsp_thread.c
# End Source File
# Begin Source File

SOURCE=.\rtsp_thread_ipc.h
# End Source File
# Begin Source File

SOURCE=.\rtsp_thread_win.cpp
# End Source File
# Begin Source File

SOURCE=.\rtsp_thread_win.h
# End Source File
# Begin Source File

SOURCE=.\rtsp_util.c
# End Source File
# End Target
# End Project
