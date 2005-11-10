# Microsoft Developer Studio Project File - Name="wmp4player60" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=wmp4player60 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "wmp4player60.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wmp4player60.mak" CFG="wmp4player60 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wmp4player60 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "wmp4player60 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "wmp4player60 - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Gui_Release"
# PROP Intermediate_Dir "Gui_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I "win_common" /I "..\..\include" /I "..\..\lib" /I "..\..\SDL\include" /I "..\..\lib\utils" /I "..\..\lib\msg_queue" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_WIN32" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386 /out:"Release/wmp4player.exe"
# SUBTRACT LINK32 /debug
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying SDL
PostBuild_Cmds=copy ..\..\SDL\lib\SDL.dll
# End Special Build Tool

!ELSEIF  "$(CFG)" == "wmp4player60 - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Gui_Debug"
# PROP Intermediate_Dir "Gui_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "." /I "win_common" /I "..\..\include" /I "..\..\lib" /I "..\..\SDL\include" /I "..\..\lib\utils" /I "..\..\lib\msg_queue" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "DEBUG" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/wmp4player.exe" /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy SDL
PostBuild_Cmds=copy ..\..\SDL\lib\SDL.dll
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "wmp4player60 - Win32 Release"
# Name "wmp4player60 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\win_common\config.cpp
# End Source File
# Begin Source File

SOURCE=.\win_gui\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\win_gui\mp4if.cpp
# End Source File
# Begin Source File

SOURCE=.\win_gui\mp4process.cpp
# End Source File
# Begin Source File

SOURCE=.\our_config_file.cpp
# End Source File
# Begin Source File

SOURCE=.\win_gui\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\win_gui\wmp4player.cpp
# End Source File
# Begin Source File

SOURCE=.\win_gui\wmp4player.rc
# End Source File
# Begin Source File

SOURCE=.\win_gui\wmp4playerDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\win_gui\wmp4playerView.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\win_gui\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\win_gui\mp4if.h
# End Source File
# Begin Source File

SOURCE=.\win_gui\mp4process.h
# End Source File
# Begin Source File

SOURCE=.\win_gui\resource.h
# End Source File
# Begin Source File

SOURCE=.\win_gui\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\win_gui\wmp4player.h
# End Source File
# Begin Source File

SOURCE=.\win_gui\wmp4playerDoc.h
# End Source File
# Begin Source File

SOURCE=.\win_gui\wmp4playerView.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=".\win_gui\res\mpeg4ip-banner.bmp"
# End Source File
# Begin Source File

SOURCE=.\win_gui\res\pause_dis.bmp
# End Source File
# Begin Source File

SOURCE=.\win_gui\res\pause_u.bmp
# End Source File
# Begin Source File

SOURCE=.\win_gui\res\paused.bmp
# End Source File
# Begin Source File

SOURCE=.\win_gui\res\play.bmp
# End Source File
# Begin Source File

SOURCE=.\win_gui\res\play_d.bmp
# End Source File
# Begin Source File

SOURCE=.\win_gui\res\playdis.bmp
# End Source File
# Begin Source File

SOURCE=.\win_gui\res\stop_dis.bmp
# End Source File
# Begin Source File

SOURCE=.\win_gui\res\stop_u.bmp
# End Source File
# Begin Source File

SOURCE=.\win_gui\res\stopd.bmp
# End Source File
# Begin Source File

SOURCE=.\win_gui\res\wmp4player.ico
# End Source File
# Begin Source File

SOURCE=.\win_gui\res\wmp4player.rc2
# End Source File
# Begin Source File

SOURCE=.\win_gui\res\wmp4playerDoc.ico
# End Source File
# End Group
# Begin Group "Help Files"

# PROP Default_Filter "cnt;rtf"
# Begin Source File

SOURCE=.\hlp\AfxCore.rtf
# End Source File
# Begin Source File

SOURCE=.\hlp\AppExit.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\Bullet.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurArw2.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurArw4.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurHelp.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditCopy.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditCut.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditPast.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditUndo.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileNew.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileOpen.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FilePrnt.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileSave.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\HlpSBar.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\HlpTBar.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecFirst.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecLast.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecNext.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecPrev.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\Scmax.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\ScMenu.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\Scmin.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\wmp4player.cnt
# PROP Ignore_Default_Tool 1
# End Source File
# End Group
# Begin Source File

SOURCE=.\checkout.txt
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
