# Microsoft Developer Studio Project File - Name="libcelp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libcelp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libcelp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libcelp.mak" CFG="libcelp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libcelp - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libcelp - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libcelp - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libcelp - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libcelp - Win32 Release"
# Name "libcelp - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\BS\adif.c
# End Source File
# Begin Source File

SOURCE=..\dec\att_abs_postp.c
# End Source File
# Begin Source File

SOURCE=..\dec\att_bwx.c
# End Source File
# Begin Source File

SOURCE=..\dec\att_firfilt.c
# End Source File
# Begin Source File

SOURCE=..\dec\att_iirfilt.c
# End Source File
# Begin Source File

SOURCE=..\dec\att_lsf2pc.c
# End Source File
# Begin Source File

SOURCE=..\dec\att_pc2lsf.c
# End Source File
# Begin Source File

SOURCE=..\dec\att_testbound.c
# End Source File
# Begin Source File

SOURCE=..\BS\audio.c
# End Source File
# Begin Source File

SOURCE=..\BS\austream.c
# End Source File
# Begin Source File

SOURCE=..\BS\bitstream.c
# End Source File
# Begin Source File

SOURCE=..\dec\celp_bitstream_demux.c
# End Source File
# Begin Source File

SOURCE=..\dec\celp_decoder.c
# End Source File
# Begin Source File

SOURCE=..\BS\cmdline.c
# End Source File
# Begin Source File

SOURCE=..\BS\common_m4a.c
# End Source File
# Begin Source File

SOURCE=..\dec\dec_lpc.c
# End Source File
# Begin Source File

SOURCE=..\dec\dec_lpc_dmy.c

!IF  "$(CFG)" == "libcelp - Win32 Release"

!ELSEIF  "$(CFG)" == "libcelp - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\BS\fir_filt.c
# End Source File
# Begin Source File

SOURCE=..\BS\flex_mux.c
# End Source File
# Begin Source File

SOURCE=..\dec\nb_celp_dec.c
# End Source File
# Begin Source File

SOURCE=..\dec\nec_abs_exc_generation.c
# End Source File
# Begin Source File

SOURCE=..\dec\nec_bws_acb_dec.c
# End Source File
# Begin Source File

SOURCE=..\dec\nec_bws_exc_generation.c
# End Source File
# Begin Source File

SOURCE=..\dec\nec_bws_gain_dec.c
# End Source File
# Begin Source File

SOURCE=..\dec\nec_bws_module.c
# End Source File
# Begin Source File

SOURCE=..\dec\nec_bws_mp_dec.c
# End Source File
# Begin Source File

SOURCE=..\dec\nec_bws_qlsp_dec.c
# End Source File
# Begin Source File

SOURCE=..\dec\nec_bws_qrms_dec.c
# End Source File
# Begin Source File

SOURCE=..\dec\nec_enh_gain_dec.c
# End Source File
# Begin Source File

SOURCE=..\dec\nec_enh_mp_dec.c
# End Source File
# Begin Source File

SOURCE=..\dec\nec_exc_acb_dec.c
# End Source File
# Begin Source File

SOURCE=..\dec\nec_exc_gain_dec.c
# End Source File
# Begin Source File

SOURCE=..\dec\nec_exc_module.c
# End Source File
# Begin Source File

SOURCE=..\dec\nec_exc_mp_config.c
# End Source File
# Begin Source File

SOURCE=..\dec\nec_exc_mp_dec.c
# End Source File
# Begin Source File

SOURCE=..\dec\nec_qrms_dec.c
# End Source File
# Begin Source File

SOURCE=..\dec\pan_lsp_intp.c
# End Source File
# Begin Source File

SOURCE=..\dec\pan_lspdec.c
# End Source File
# Begin Source File

SOURCE=..\dec\pan_lspqtz2_dd.c
# End Source File
# Begin Source File

SOURCE=..\dec\pan_mv_data.c
# End Source File
# Begin Source File

SOURCE=..\dec\pan_sort.c
# End Source File
# Begin Source File

SOURCE=..\dec\pan_stab.c
# End Source File
# Begin Source File

SOURCE=..\dec\phi_fbit.c
# End Source File
# Begin Source File

SOURCE=..\dec\phi_gxit.c
# End Source File
# Begin Source File

SOURCE=..\dec\phi_lpc.c
# End Source File
# Begin Source File

SOURCE=..\dec\phi_lsfr.c
# End Source File
# Begin Source File

SOURCE=..\dec\phi_post.c
# End Source File
# Begin Source File

SOURCE=..\dec\phi_priv.c
# End Source File
# Begin Source File

SOURCE=..\dec\phi_xits.c
# End Source File
# Begin Source File

SOURCE=..\BS\tf_tables.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
