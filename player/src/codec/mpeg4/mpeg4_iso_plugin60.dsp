# Microsoft Developer Studio Project File - Name="mpeg4_iso_plugin" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=mpeg4_iso_plugin - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mpeg4_iso_plugin60.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mpeg4_iso_plugin60.mak" CFG="mpeg4_iso_plugin - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mpeg4_iso_plugin - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mpeg4_iso_plugin - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mpeg4_iso_plugin - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../../../common/video/mpeg4-2000/vtc/zte" /I "../.." /I "../../../lib" /I "../../../../lib" /I "../../../../common/video/iso-mpeg4/include" /I "../../../../include" /I "../../../../lib/SDL/include" /I "../../../../lib/mp4v2" /I "../../../../lib/sdp" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WIN32" /D "__TRACE_AND_STATS_" /D "__DOUBLE_PRECISION_" /D "__PC_COMPILER_" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"libc.lib" /out:"Release/mpeg4_iso_plugin.dll"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying dll to player/src
PostBuild_Cmds=copy Release\mpeg4_iso_plugin.dll ..\..
# End Special Build Tool

!ELSEIF  "$(CFG)" == "mpeg4_iso_plugin - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "../.." /I "../../../lib" /I "../../../../lib" /I "../../../../common/video/iso-mpeg4/include" /I "../../../../include" /I "../../../../lib/SDL/include" /I "../../../../lib/mp4v2" /I "../../../../lib/sdp" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WIN32" /D "__TRACE_AND_STATS_" /D "__DOUBLE_PRECISION_" /D "_MBC" /D "_CONSOLE" /D "_OBSS_" /D "_VTC_DECODER_" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"libcd.lib" /out:"Debug/mpeg4_iso_plugin.dll" /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Debug\mpeg4_iso_plugin.dll ..\..
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "mpeg4_iso_plugin - Win32 Release"
# Name "mpeg4_iso_plugin - Win32 Debug"
# Begin Source File

SOURCE=.\mpeg4.cpp
# End Source File
# Begin Source File

SOURCE=.\mpeg4.h
# End Source File
# Begin Source File

SOURCE=.\mpeg4_file.cpp
# End Source File
# Begin Source File

SOURCE=.\mpeg4_file.h
# End Source File
# End Target
# End Project
