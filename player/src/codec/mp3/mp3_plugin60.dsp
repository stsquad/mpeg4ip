# Microsoft Developer Studio Project File - Name="mp3_plugin" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=mp3_plugin - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mp3_plugin60.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mp3_plugin60.mak" CFG="mp3_plugin - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mp3_plugin - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mp3_plugin - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mp3_plugin - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I "../.." /I "../../../lib" /I "../../../lib/audio" /I "../../../../include" /I "../../../../lib" /I "../../../../SDL/include" /I "../../../../lib/mp4v2" /I "../../../../lib/mp4av" /I "../../../../lib/sdp" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WIN32" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib ../../../../SDL/lib/SDL.lib ws2_32.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"libc.lib" /out:"Release/mp3_plugin.dll"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying mp3 plugin to player/src
PostBuild_Cmds=copy Release\mp3_plugin.dll ..\..
# End Special Build Tool

!ELSEIF  "$(CFG)" == "mp3_plugin - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "." /I "../.." /I "../../../lib" /I "../../../lib/audio" /I "../../../../include" /I "../../../../lib" /I "../../../../SDL/include" /I "../../../../lib/mp4v2" /I "../../../../lib/mp4av" /I "../../../../lib/sdp" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WIN32" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib ../../../../SDL/lib/SDL.lib ws2_32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"libcd.lib" /out:"Debug/mp3_plugin.dll" /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying to player/src directory
PostBuild_Cmds=copy Debug\mp3_plugin.dll ..\..
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "mp3_plugin - Win32 Release"
# Name "mp3_plugin - Win32 Debug"
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

SOURCE=.\mp3_file.cpp
# End Source File
# Begin Source File

SOURCE=.\mp3_file.h
# End Source File
# Begin Source File

SOURCE=.\mp3if.cpp
# End Source File
# Begin Source File

SOURCE=.\mp3if.h
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
