# Microsoft Developer Studio Project File - Name="faac" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=faac - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "faac.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "faac.mak" CFG="faac - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "faac - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "faac - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "faac - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\..\..\include" /I "..\..\lib\sndfile\src" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "faac - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "..\..\..\include" /I "..\..\lib\sndfile\src" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "faac - Win32 Release"
# Name "faac - Win32 Debug"
# Begin Source File

SOURCE=.\aacquant.c
# End Source File
# Begin Source File

SOURCE=.\aacquant.h
# End Source File
# Begin Source File

SOURCE=.\backpred.c
# End Source File
# Begin Source File

SOURCE=.\backpred.h
# End Source File
# Begin Source File

SOURCE=.\bitstream.c
# End Source File
# Begin Source File

SOURCE=.\bitstream.h
# End Source File
# Begin Source File

SOURCE=.\channels.c
# End Source File
# Begin Source File

SOURCE=.\channels.h
# End Source File
# Begin Source File

SOURCE=.\coder.h
# End Source File
# Begin Source File

SOURCE=.\faac.c
# End Source File
# Begin Source File

SOURCE=.\faac.h
# End Source File
# Begin Source File

SOURCE=.\fft.c
# End Source File
# Begin Source File

SOURCE=.\fft.h
# End Source File
# Begin Source File

SOURCE=.\filtbank.c
# End Source File
# Begin Source File

SOURCE=.\filtbank.h
# End Source File
# Begin Source File

SOURCE=.\frame.c
# End Source File
# Begin Source File

SOURCE=.\frame.h
# End Source File
# Begin Source File

SOURCE=.\huffman.c
# End Source File
# Begin Source File

SOURCE=.\huffman.h
# End Source File
# Begin Source File

SOURCE=.\hufftab.h
# End Source File
# Begin Source File

SOURCE=.\joint.c
# End Source File
# Begin Source File

SOURCE=.\joint.h
# End Source File
# Begin Source File

SOURCE=.\ltp.c
# End Source File
# Begin Source File

SOURCE=.\ltp.h
# End Source File
# Begin Source File

SOURCE=.\psych.c
# End Source File
# Begin Source File

SOURCE=.\psych.h
# End Source File
# Begin Source File

SOURCE=.\tns.c
# End Source File
# Begin Source File

SOURCE=.\tns.h
# End Source File
# Begin Source File

SOURCE=.\util.c
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# End Target
# End Project
