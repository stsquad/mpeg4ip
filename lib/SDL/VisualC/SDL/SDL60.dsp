# Microsoft Developer Studio Project File - Name="SDL" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=SDL - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SDL60.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SDL60.mak" CFG="SDL - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SDL - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "SDL - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SDL - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\src" /I "..\..\src\audio" /I "..\..\src\video" /I "..\..\src\video\wincommon" /I "..\..\src\video\windx5" /I "..\..\src\events" /I "..\..\src\joystick" /I "..\..\src\cdrom" /I "..\..\src\thread" /I "..\..\src\thread\win32" /I "..\..\src\timer" /I "..\..\include" /I "..\..\include\SDL" /D "NDEBUG" /D "ENABLE_WINDIB" /D "ENABLE_DIRECTX" /D _WIN32_WINNT=0x0400 /D "WIN32" /D "_WINDOWS" /D DIRECTINPUT_VERSION=0x0500 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 winmm.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:yes /machine:I386

!ELSEIF  "$(CFG)" == "SDL - Win32 Debug"

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
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "..\..\src\thread\win32" /I "..\..\src" /I "..\..\src\audio" /I "..\..\src\video" /I "..\..\src\video\wincommon" /I "..\..\src\video\windx5" /I "..\..\src\events" /I "..\..\src\joystick" /I "..\..\src\cdrom" /I "..\..\src\thread" /I "..\..\src\timer" /I "..\..\include" /I "..\..\include\SDL" /D "_DEBUG" /D "ENABLE_WINDIB" /D "ENABLE_DIRECTX" /D _WIN32_WINNT=0x0400 /D "WIN32" /D "_WINDOWS" /D DIRECTINPUT_VERSION=0x0500 /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "SDL - Win32 Release"
# Name "SDL - Win32 Debug"
# Begin Source File

SOURCE=..\..\Src\Video\blank_cursor.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\default_cursor.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\Windx5\Directx.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\SDL.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Events\SDL_active.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Audio\SDL_audio.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Audio\SDL_audio_c.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Audio\SDL_audiocvt.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Audio\SDL_audiomem.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Audio\SDL_audiomem.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_blit.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_blit.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_blit_0.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_blit_1.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_blit_A.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_blit_A.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_blit_N.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_bmp.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Cdrom\SDL_cdrom.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_cursor.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_cursor_c.h
# End Source File
# Begin Source File

SOURCE=..\..\src\audio\windib\SDL_dibaudio.c
# End Source File
# Begin Source File

SOURCE=..\..\src\audio\windib\SDL_dibaudio.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\Windib\SDL_dibevents.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\Windib\SDL_dibevents_c.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\Windib\SDL_dibvideo.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\Windib\SDL_dibvideo.h
# End Source File
# Begin Source File

SOURCE=..\..\src\audio\windx5\SDL_dx5audio.c
# End Source File
# Begin Source File

SOURCE=..\..\src\audio\windx5\SDL_dx5audio.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\Windx5\SDL_dx5events.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\Windx5\SDL_dx5events_c.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\Windx5\SDL_dx5video.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\Windx5\SDL_dx5video.h
# End Source File
# Begin Source File

SOURCE=..\..\src\video\windx5\SDL_dx5yuv.c
# End Source File
# Begin Source File

SOURCE=..\..\src\video\windx5\SDL_dx5yuv_c.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Endian\SDL_endian.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\SDL_error.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\SDL_error_c.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Events\SDL_events.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Events\SDL_events_c.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\SDL_fatal.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\SDL_fatal.h
# End Source File
# Begin Source File

SOURCE=..\..\src\video\SDL_gamma.c
# End Source File
# Begin Source File

SOURCE=..\..\src\joystick\SDL_joystick.c
# End Source File
# Begin Source File

SOURCE=..\..\src\joystick\SDL_joystick_c.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Events\SDL_keyboard.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_leaks.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\wincommon\SDL_lowvideo.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Audio\SDL_mixer.c
# End Source File
# Begin Source File

SOURCE=..\..\src\joystick\win32\SDL_mmjoystick.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Events\SDL_mouse.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_pixels.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_pixels_c.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Events\SDL_quit.c
# End Source File
# Begin Source File

SOURCE=..\..\src\events\SDL_resize.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_RLEaccel.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_RLEaccel_c.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\File\SDL_rwops.c
# End Source File
# Begin Source File

SOURCE=..\..\src\video\SDL_stretch.c
# End Source File
# Begin Source File

SOURCE=..\..\src\video\SDL_stretch_c.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_surface.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Audio\SDL_sysaudio.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Cdrom\Win32\SDL_syscdrom.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Cdrom\SDL_syscdrom.h
# End Source File
# Begin Source File

SOURCE=..\..\src\thread\generic\SDL_syscond.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\wincommon\SDL_sysevents.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Events\SDL_sysevents.h
# End Source File
# Begin Source File

SOURCE=..\..\src\joystick\SDL_sysjoystick.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\wincommon\SDL_sysmouse.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\wincommon\SDL_sysmouse_c.h
# End Source File
# Begin Source File

SOURCE=..\..\src\thread\win32\SDL_sysmutex.c
# End Source File
# Begin Source File

SOURCE=..\..\src\thread\win32\SDL_syssem.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Thread\Win32\SDL_systhread.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Thread\SDL_systhread.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Thread\Win32\SDL_systhread_c.h
# End Source File
# Begin Source File

SOURCE=..\..\src\timer\win32\SDL_systimer.c
# End Source File
# Begin Source File

SOURCE=..\..\src\timer\SDL_systimer.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_sysvideo.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\wincommon\SDL_syswm.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\wincommon\SDL_syswm_c.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Thread\SDL_thread.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Thread\SDL_thread_c.h
# End Source File
# Begin Source File

SOURCE=..\..\src\timer\SDL_timer.c
# End Source File
# Begin Source File

SOURCE=..\..\src\timer\SDL_timer_c.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\SDL_video.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\Windib\SDL_vkeys.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Audio\SDL_wave.c
# End Source File
# Begin Source File

SOURCE=..\..\Src\Audio\SDL_wave.h
# End Source File
# Begin Source File

SOURCE=..\..\src\video\wincommon\SDL_wingl.c
# End Source File
# Begin Source File

SOURCE=..\..\src\video\wincommon\SDL_wingl_c.h
# End Source File
# Begin Source File

SOURCE=..\..\src\video\SDL_yuv.c
# End Source File
# Begin Source File

SOURCE=..\..\src\video\SDL_yuv_sw.c
# End Source File
# Begin Source File

SOURCE=..\..\src\video\SDL_yuv_sw_c.h
# End Source File
# Begin Source File

SOURCE=..\..\src\video\SDL_yuvfuncs.h
# End Source File
# Begin Source File

SOURCE=..\..\Src\Video\wincommon\Wmmsg.h
# End Source File
# End Target
# End Project
