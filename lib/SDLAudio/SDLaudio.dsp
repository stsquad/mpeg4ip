# Microsoft Developer Studio Project File - Name="SDLaudio" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=SDLaudio - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SDLaudio.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SDLaudio.mak" CFG="SDLaudio - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SDLaudio - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "SDLaudio - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SDLaudio - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "Src" /I "src/audio" /I "include" /I "../../SDL/include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "ENABLE_WINDIB" /D "ENABLE_DIRECTX" /D _WIN32_WINNT=0x0400 /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "SDLaudio - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "Src" /I "src/audio" /I "include" /I "../../SDL/include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "ENABLE_WINDIB" /D "ENABLE_DIRECTX" /D _WIN32_WINNT=0x0400 /YX /FD /GZ /c
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

# Name "SDLaudio - Win32 Release"
# Name "SDLaudio - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\audio\SDL_audio.c
# End Source File
# Begin Source File

SOURCE=.\src\audio\SDL_audiocvt.c
# End Source File
# Begin Source File

SOURCE=.\src\audio\SDL_audiodev.c
# End Source File
# Begin Source File

SOURCE=.\src\audio\SDL_audiomem.c
# End Source File
# Begin Source File

SOURCE=.\src\audio\windib\SDL_dibaudio.c
# End Source File
# Begin Source File

SOURCE=.\src\audio\windib\SDL_dibaudio.h
# End Source File
# Begin Source File

SOURCE=.\src\audio\windx5\SDL_dx5audio.c
# End Source File
# Begin Source File

SOURCE=.\src\audio\windx5\SDL_dx5audio.h
# End Source File
# Begin Source File

SOURCE=.\src\SDL_error.c
# End Source File
# Begin Source File

SOURCE=.\src\audio\SDL_mixer.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\include\Our_SDL_audio.h
# End Source File
# Begin Source File

SOURCE=.\src\audio\SDL_audio_c.h
# End Source File
# Begin Source File

SOURCE=.\src\audio\SDL_audiodev_c.h
# End Source File
# Begin Source File

SOURCE=.\src\audio\SDL_audiomem.h
# End Source File
# Begin Source File

SOURCE=.\src\audio\SDL_mixer_m68k.h
# End Source File
# Begin Source File

SOURCE=.\src\audio\SDL_mixer_MMX.h
# End Source File
# Begin Source File

SOURCE=.\src\audio\SDL_mixer_MMX_VC.h
# End Source File
# Begin Source File

SOURCE=.\src\audio\SDL_sysaudio.h
# End Source File
# Begin Source File

SOURCE=.\src\audio\SDL_wave.h
# End Source File
# End Group
# End Target
# End Project
