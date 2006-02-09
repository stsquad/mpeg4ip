# Microsoft Developer Studio Project File - Name="isoencoder" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=isoencoder - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "isoencoder60.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "isoencoder60.mak" CFG="isoencoder - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "isoencoder - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "isoencoder - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "isoencoder - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "__TRACE_AND_STATS_" /D "__DOUBLE_PRECISION_" /D "__PC_COMPILER_" /D "_WIN32" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "isoencoder - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "isoencod"
# PROP BASE Intermediate_Dir "isoencod"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "DEbug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "__TRACE_AND_STATS_" /D "__DOUBLE_PRECISION_" /D "__PC_COMPILER_" /D "_WIN32" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "isoencoder - Win32 Release"
# Name "isoencoder - Win32 Debug"
# Begin Group "Include"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\ac.hpp
# End Source File
# Begin Source File

SOURCE=.\include\basic.hpp
# End Source File
# Begin Source File

SOURCE=.\include\BinArCodec.hpp
# End Source File
# Begin Source File

SOURCE=.\include\bitpack.hpp
# End Source File
# Begin Source File

SOURCE=.\include\bitstrm.hpp
# End Source File
# Begin Source File

SOURCE=.\include\blkdec.hpp
# End Source File
# Begin Source File

SOURCE=.\include\blkenc.hpp
# End Source File
# Begin Source File

SOURCE=.\include\block.hpp
# End Source File
# Begin Source File

SOURCE=.\include\cae.h
# End Source File
# Begin Source File

SOURCE=.\include\codehead.h
# End Source File
# Begin Source File

SOURCE=.\include\dataStruct.hpp
# End Source File
# Begin Source File

SOURCE=.\include\dct.hpp
# End Source File
# Begin Source File

SOURCE=.\include\default.h
# End Source File
# Begin Source File

SOURCE=.\include\download_filter.h
# End Source File
# Begin Source File

SOURCE=.\include\dwt.h
# End Source File
# Begin Source File

SOURCE=.\include\enhcbufdec.hpp
# End Source File
# Begin Source File

SOURCE=.\include\enhcbufenc.hpp
# End Source File
# Begin Source File

SOURCE=.\include\entropy.hpp
# End Source File
# Begin Source File

SOURCE=.\include\errorHandler.hpp
# End Source File
# Begin Source File

SOURCE=.\include\geom.hpp
# End Source File
# Begin Source File

SOURCE=.\include\global.hpp
# End Source File
# Begin Source File

SOURCE=.\include\globals.hpp
# End Source File
# Begin Source File

SOURCE=.\include\grayc.hpp
# End Source File
# Begin Source File

SOURCE=.\include\grayf.hpp
# End Source File
# Begin Source File

SOURCE=.\include\grayi.hpp
# End Source File
# Begin Source File

SOURCE=.\include\header.h
# End Source File
# Begin Source File

SOURCE=.\include\huffman.hpp
# End Source File
# Begin Source File

SOURCE=.\include\idct.hpp
# End Source File
# Begin Source File

SOURCE=.\include\inbits.h
# End Source File
# Begin Source File

SOURCE=.\include\mb.hpp
# End Source File
# Begin Source File

SOURCE=.\include\mbdec.hpp
# End Source File
# Begin Source File

SOURCE=.\include\mode.hpp
# End Source File
# Begin Source File

SOURCE=.\include\msg.hpp
# End Source File
# Begin Source File

SOURCE=.\include\newpred.hpp
# End Source File
# Begin Source File

SOURCE=.\include\paramset.h
# End Source File
# Begin Source File

SOURCE=.\include\PEZW_ac.hpp
# End Source File
# Begin Source File

SOURCE=.\include\PEZW_functions.hpp
# End Source File
# Begin Source File

SOURCE=.\include\PEZW_mpeg4.hpp
# End Source File
# Begin Source File

SOURCE=.\include\PEZW_zerotree.hpp
# End Source File
# Begin Source File

SOURCE=.\include\QM.hpp
# End Source File
# Begin Source File

SOURCE=.\include\QMUtils.hpp
# End Source File
# Begin Source File

SOURCE=.\include\quant.hpp
# End Source File
# Begin Source File

SOURCE=.\include\rrv.hpp
# End Source File
# Begin Source File

SOURCE=.\include\sesenc.hpp
# End Source File
# Begin Source File

SOURCE=.\include\shape.hpp
# End Source File
# Begin Source File

SOURCE=.\include\shape_def.hpp
# End Source File
# Begin Source File

SOURCE=.\include\ShapeBaseCommon.hpp
# End Source File
# Begin Source File

SOURCE=.\include\ShapeEnhDef.hpp
# End Source File
# Begin Source File

SOURCE=.\include\shpdec.hpp
# End Source File
# Begin Source File

SOURCE=.\include\shpenc.hpp
# End Source File
# Begin Source File

SOURCE=.\include\startcode.hpp
# End Source File
# Begin Source File

SOURCE=.\include\states.hpp
# End Source File
# Begin Source File

SOURCE=.\include\svd.h
# End Source File
# Begin Source File

SOURCE=.\include\tm5rc.hpp
# End Source File
# Begin Source File

SOURCE=.\include\tps_enhcbuf.hpp
# End Source File
# Begin Source File

SOURCE=.\include\transf.hpp
# End Source File
# Begin Source File

SOURCE=.\include\typeapi.h
# End Source File
# Begin Source File

SOURCE=.\include\Utils.hpp
# End Source File
# Begin Source File

SOURCE=.\include\vlc.hpp
# End Source File
# Begin Source File

SOURCE=.\include\vop.hpp
# End Source File
# Begin Source File

SOURCE=.\include\vopmbdec.hpp
# End Source File
# Begin Source File

SOURCE=.\include\vopmbenc.hpp
# End Source File
# Begin Source File

SOURCE=.\include\vopsedec.hpp
# End Source File
# Begin Source File

SOURCE=.\include\vopseenc.hpp
# End Source File
# Begin Source File

SOURCE=.\include\vopses.hpp
# End Source File
# Begin Source File

SOURCE=.\include\warp.hpp
# End Source File
# Begin Source File

SOURCE=.\include\wvtfilter.h
# End Source File
# Begin Source File

SOURCE=.\include\wvtPEZW.hpp
# End Source File
# Begin Source File

SOURCE=.\include\wvtpezw_tree_codec.hpp
# End Source File
# Begin Source File

SOURCE=.\include\yuvac.hpp
# End Source File
# Begin Source File

SOURCE=.\include\yuvai.hpp
# End Source File
# Begin Source File

SOURCE=.\include\ztscan_common.hpp
# End Source File
# Begin Source File

SOURCE=.\include\ztscanUtil.hpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\app_encoder_encoder.cpp
# End Source File
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

SOURCE=.\src\sys_encoder_blkenc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_encoder_errenc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_encoder_gmc_enc_util.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_encoder_gme_for_iso.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_encoder_mbenc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_encoder_mbinterlace.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_encoder_mcenc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_encoder_motest.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_encoder_mvenc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_encoder_newpenc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_encoder_padenc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_encoder_paramset.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_encoder_sesenc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_encoder_shpenc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_encoder_sptenc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_encoder_tm5rc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_encoder_vopmbenc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sys_encoder_vopseenc.cpp
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
