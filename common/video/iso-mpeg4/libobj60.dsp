# Microsoft Developer Studio Project File - Name="libobj" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libobj - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libobj60.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libobj60.mak" CFG="libobj - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libobj - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libobj - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libobj - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "include" /D "NDEBUG" /D "__PC_COMPILER_" /D "WIN32" /D "_WINDOWS" /D "__TRACE_AND_STATS_" /D "__DOUBLE_PRECISION_" /D "_WIN32" /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libobj - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "include" /D "_DEBUG" /D "_MBC" /D "_CONSOLE" /D "_OBSS_" /D "_VTC_DECODER_" /D "__PC_COMPILER_" /D "WIN32" /D "_WINDOWS" /D "__TRACE_AND_STATS_" /D "__DOUBLE_PRECISION_" /D "_WIN32" /YX /FD /c
# SUBTRACT CPP /Fr
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

# Name "libobj - Win32 Release"
# Name "libobj - Win32 Debug"
# Begin Source File

SOURCE=.\src\idct_idct.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_block.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_cae.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_dct.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_decoder_blkdec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_decoder_errdec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_decoder_mbdec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_decoder_mbheaddec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_decoder_mbinterlacedec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_decoder_mcdec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_decoder_mvdec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_decoder_newpdec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_decoder_rvlcdec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_decoder_shpdec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_decoder_sptdec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_decoder_vopmbdec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_decoder_vopsedec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_error.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_gmc_motion.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_gmc_util.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_mb.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_mc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_mcpad.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_mode.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_mv.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_newpred.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_rrv.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_shape.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_spt.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_tps_bfshape.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_tps_enhcbuf.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_vopses.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tools_entropy_bitstrm.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tools_entropy_huffman.cpp
# End Source File
# Begin Source File

SOURCE=.\src\tools_sadct_sadct.cpp
# End Source File
# Begin Source File

SOURCE=.\src\type_basic.cpp
# End Source File
# Begin Source File

SOURCE=.\src\type_geom.cpp
# End Source File
# Begin Source File

SOURCE=.\src\type_grayc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\type_grayf.cpp
# End Source File
# Begin Source File

SOURCE=.\src\type_grayi.cpp
# End Source File
# Begin Source File

SOURCE=.\src\type_svd.cpp
# End Source File
# Begin Source File

SOURCE=.\src\type_typeapi.cpp
# End Source File
# Begin Source File

SOURCE=.\src\type_vop.cpp
# End Source File
# Begin Source File

SOURCE=.\src\type_warp.cpp
# End Source File
# Begin Source File

SOURCE=.\src\type_yuvac.cpp
# End Source File
# Begin Source File

SOURCE=.\src\type_yuvai.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_main_computePSNR.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_main_read_image.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_main_seg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_main_vtcdec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_main_vtcenc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_main_wavelet.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_main_write_image.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_pezw_PEZW_ac.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_pezw_PEZW_globals.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_pezw_PEZW_textureLayerBQ.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_pezw_PEZW_utils.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_pezw_wvtPEZW.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_pezw_wvtpezw_tree_decode.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_pezw_wvtpezw_tree_encode.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_pezw_wvtpezw_tree_init_decode.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_pezw_wvtpezw_tree_init_encode.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_shape_BinArCodec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_shape_ShapeBaseCommon.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_shape_ShapeBaseDecode.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_shape_ShapeBaseEncode.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_shape_ShapeDecoding.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_shape_ShapeEncoding.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_shape_ShapeEnhCommon.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_shape_ShapeEnhDecode.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_shape_ShapeEnhEncode.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_shape_ShapeUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_wavelet_download_filter.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_wavelet_dwt.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_wavelet_dwt_aux.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_wavelet_dwtmask.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_wavelet_idwt.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_wavelet_idwt_aux.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_wavelet_idwtmask.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_wavelet_imagebox.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_zte_ac.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_zte_bitpack.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_zte_ztscan_dec.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_zte_ztscan_enc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_zte_ztscanUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_ztq_decQM.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_ztq_encQM.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_ztq_errorHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_ztq_msg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_ztq_QMInit.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_ztq_QMUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_ztq_quant.cpp
# End Source File
# Begin Source File

SOURCE=.\src\vtc_ztq_Utils.cpp
# End Source File
# End Target
# End Project
