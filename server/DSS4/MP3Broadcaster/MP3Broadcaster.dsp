# Microsoft Developer Studio Project File - Name="MP3Broadcaster" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=MP3Broadcaster - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MP3Broadcaster.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MP3Broadcaster.mak" CFG="MP3Broadcaster - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MP3Broadcaster - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe
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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /w /W0 /Gm /ZI /Od /I "../" /I "../CommonUtilitiesLib/" /I "../APICommonCode/" /I "../PlaylistBroadcaster.tproj/" /FI"../WinNTSupport/Win32header.h" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FD /I /GZ "../PrefsSourceLib/" /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib oleaut32.lib ws2_32.lib wsock32.lib winmm.lib libcmtd.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"libcd.lib" /out:"..\WinNTSupport\MP3Broadcaster.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# Begin Target

# Name "MP3Broadcaster - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\BroadcasterMain.cpp
# End Source File
# Begin Source File

SOURCE=.\MP3Broadcaster.cpp
# End Source File
# Begin Source File

SOURCE=.\MP3BroadcasterLog.cpp
# End Source File
# Begin Source File

SOURCE=.\MP3FileBroadcaster.cpp
# End Source File
# Begin Source File

SOURCE=.\MP3MetaInfoUpdater.cpp
# End Source File
# Begin Source File

SOURCE=..\PlaylistBroadcaster.tproj\NoRepeat.cpp
# End Source File
# Begin Source File

SOURCE=..\OSMemoryLib\OSMemory.cpp
# End Source File
# Begin Source File

SOURCE=..\PlaylistBroadcaster.tproj\PickerFromFile.cpp
# End Source File
# Begin Source File

SOURCE=..\PlaylistBroadcaster.tproj\PlaylistPicker.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
