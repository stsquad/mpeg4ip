# Microsoft Developer Studio Project File - Name="mp4player" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=mp4player - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mp4player60.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mp4player60.mak" CFG="mp4player - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mp4player - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "mp4player - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mp4player - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /I "../../../include" /I "../lib/SDL/include" /I "../../lib" /I "../../SDL/include" /I "../../include" /I "../lib" /I "../../common/video/mpeg4" /I "../../lib/sdp" /I "../../lib/utils" /I "../../lib/mp4v2" /I "../../lib/msg_queue" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D _WIN32_WINNT=0x0400 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib advapi32.lib ../../SDL/lib/SDL.lib ../../SDL/lib/SDLMain.lib winmm.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc.lib" /out:"Release/mp4player.exe"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=copying SDL
PostBuild_Cmds=copy ..\..\SDL\lib\SDL.dll
# End Special Build Tool

!ELSEIF  "$(CFG)" == "mp4player - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "mp4playe"
# PROP BASE Intermediate_Dir "mp4playe"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "../../lib" /I "../../SDL/include" /I "../../include" /I "../lib" /I "../../common/video/mpeg4" /I "../../lib/sdp" /I "../../lib/utils" /I "../../lib/mp4v2" /I "../../lib/msg_queue" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D _WIN32_WINNT=0x0400 /D "_CONSOLE" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib ws2_32.lib advapi32.lib user32.lib ../../SDL/lib/SDL.lib ../../SDL/lib/SDLMain.lib winmm.lib /nologo /subsystem:windows /pdb:none /debug /machine:I386 /nodefaultlib:"libcd.lib" /nodefaultlib:"msvcrt.lib" /out:"debug/mp4player.exe"
# SUBTRACT LINK32 /nodefaultlib
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy ..\..\SDL\lib\SDL.dll
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "mp4player - Win32 Release"
# Name "mp4player - Win32 Debug"
# Begin Group "source files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\player_util.c
# End Source File
# End Group
# Begin Group "include"

# PROP Default_Filter ".h"
# End Group
# End Target
# End Project
