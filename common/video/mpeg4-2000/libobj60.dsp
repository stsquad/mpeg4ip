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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "sys" /I "sys/encoder" /I "sys/decoder" /I "type" /I "tools" /I "tools/entropy" /I "vtc" /I "vtc/include" /I "vtc/main" /I "vtc/pezw" /I "vtc/wavelet" /I "vtc/zte" /I "vtc/ztq" /I "..\..\..\include" /I "vtc/shape" /I "idct" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__TRACE_AND_STATS_" /D "__DOUBLE_PRECISION_" /D "__PC_COMPILER_" /YX /FD /c
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
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "idct" /I "sys" /I "sys/encoder" /I "sys/decoder" /I "type" /I "tools" /I "tools/entropy" /I "vtc" /I "vtc/include" /I "vtc/main" /I "vtc/pezw" /I "vtc/wavelet" /I "vtc/zte" /I "vtc/ztq" /I "..\..\..\include" /I "vtc/shape" /D "_DEBUG" /D "__PC_COMPILER_" /D "WIN32" /D "_WINDOWS" /D "__TRACE_AND_STATS_" /D "__DOUBLE_PRECISION_" /D "_MBC" /D "_CONSOLE" /D "_OBSS_" /D "_VTC_DECODER_" /YX /FD /c
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
# Begin Group "sys"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sys\block.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\block.hpp
# End Source File
# Begin Source File

SOURCE=.\sys\cae.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\cae.h
# End Source File
# Begin Source File

SOURCE=.\sys\codehead.h
# End Source File
# Begin Source File

SOURCE=.\sys\dct.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\dct.hpp
# End Source File
# Begin Source File

SOURCE=.\sys\error.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\global.hpp
# End Source File
# Begin Source File

SOURCE=.\sys\gmc_motion.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\gmc_util.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\mb.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\mb.hpp
# End Source File
# Begin Source File

SOURCE=.\sys\mc.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\mcpad.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\mode.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\mode.hpp
# End Source File
# Begin Source File

SOURCE=.\sys\mv.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\newpred.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\rrv.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\shape.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\shape.hpp
# End Source File
# Begin Source File

SOURCE=.\sys\spt.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\tps_bfshape.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\tps_enhcbuf.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\tps_enhcbuf.hpp
# End Source File
# Begin Source File

SOURCE=.\sys\vopses.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\vopses.hpp
# End Source File
# End Group
# Begin Group "sys/decoder"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sys\decoder\blkdec.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\blkdec.hpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\enhcbufdec.hpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\errdec.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\mbdec.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\mbdec.hpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\mbheaddec.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\mbinterlacedec.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\mcdec.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\mvdec.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\newpdec.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\rvlcdec.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\shpdec.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\shpdec.hpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\sptdec.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\vopmbdec.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\vopmbdec.hpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\vopsedec.cpp
# End Source File
# Begin Source File

SOURCE=.\sys\decoder\vopsedec.hpp
# End Source File
# End Group
# Begin Group "tools/entropy"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\tools\entropy\bitstrm.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\entropy\bitstrm.hpp
# End Source File
# Begin Source File

SOURCE=.\tools\entropy\entropy.hpp
# End Source File
# Begin Source File

SOURCE=.\tools\entropy\huffman.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\entropy\huffman.hpp
# End Source File
# Begin Source File

SOURCE=.\tools\entropy\inbits.h
# End Source File
# Begin Source File

SOURCE=.\tools\entropy\vlc.hpp
# End Source File
# End Group
# Begin Group "type"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\type\basic.cpp
# End Source File
# Begin Source File

SOURCE=.\type\basic.hpp
# End Source File
# Begin Source File

SOURCE=.\type\geom.cpp
# End Source File
# Begin Source File

SOURCE=.\type\geom.hpp
# End Source File
# Begin Source File

SOURCE=.\type\grayc.cpp
# End Source File
# Begin Source File

SOURCE=.\type\grayc.hpp
# End Source File
# Begin Source File

SOURCE=.\type\grayf.cpp
# End Source File
# Begin Source File

SOURCE=.\type\grayf.hpp
# End Source File
# Begin Source File

SOURCE=.\type\grayi.cpp
# End Source File
# Begin Source File

SOURCE=.\type\grayi.hpp
# End Source File
# Begin Source File

SOURCE=.\type\header.h
# End Source File
# Begin Source File

SOURCE=.\type\svd.cpp
# End Source File
# Begin Source File

SOURCE=.\type\svd.h
# End Source File
# Begin Source File

SOURCE=.\type\transf.hpp
# End Source File
# Begin Source File

SOURCE=.\type\typeapi.cpp
# End Source File
# Begin Source File

SOURCE=.\type\typeapi.h
# End Source File
# Begin Source File

SOURCE=.\type\vop.cpp
# End Source File
# Begin Source File

SOURCE=.\type\vop.hpp
# End Source File
# Begin Source File

SOURCE=.\type\warp.cpp
# End Source File
# Begin Source File

SOURCE=.\type\warp.hpp
# End Source File
# Begin Source File

SOURCE=.\type\yuvac.cpp
# End Source File
# Begin Source File

SOURCE=.\type\yuvac.hpp
# End Source File
# Begin Source File

SOURCE=.\type\yuvai.cpp
# End Source File
# Begin Source File

SOURCE=.\type\yuvai.hpp
# End Source File
# End Group
# Begin Group "vtc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\vtc\zte\ac.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\zte\ac.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\shape\BinArCodec.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\zte\bitpack.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\zte\bitpack.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\main\computePSNR.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\include\dataStruct.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\ztq\decQM.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\wavelet\default.h
# End Source File
# Begin Source File

SOURCE=.\vtc\wavelet\download_filter.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\wavelet\download_filter.h
# End Source File
# Begin Source File

SOURCE=.\vtc\wavelet\dwt.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\wavelet\dwt.h
# End Source File
# Begin Source File

SOURCE=.\vtc\wavelet\dwt.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\wavelet\dwt_aux.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\wavelet\dwtmask.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\ztq\encQM.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\ztq\errorHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\ztq\errorHandler.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\ztq\globals.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\wavelet\idwt.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\wavelet\idwt_aux.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\wavelet\imagebox.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\ztq\msg.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\ztq\msg.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\pezw\PEZW_ac.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\pezw\PEZW_ac.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\pezw\PEZW_functions.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\pezw\PEZW_globals.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\pezw\PEZW_mpeg4.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\pezw\PEZW_textureLayerBQ.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\pezw\PEZW_utils.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\pezw\PEZW_zerotree.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\ztq\QM.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\ztq\QMInit.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\ztq\QMUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\ztq\QMUtils.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\ztq\quant.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\ztq\quant.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\main\read_image.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\main\seg.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\shape\ShapeBaseCommon.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\shape\ShapeBaseDecode.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\shape\ShapeDecoding.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\shape\ShapeEnhCommon.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\shape\ShapeEnhDecode.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\shape\ShapeUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\zte\startcode.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\include\states.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\ztq\Utils.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\ztq\Utils.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\main\vtcdec.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\main\vtcenc.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\main\wavelet.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\main\write_image.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\wavelet\wvtfilter.h
# End Source File
# Begin Source File

SOURCE=.\vtc\pezw\wvtPEZW.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\pezw\wvtPEZW.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\pezw\wvtpezw_tree_codec.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\pezw\wvtpezw_tree_decode.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\pezw\wvtpezw_tree_encode.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\pezw\wvtpezw_tree_init_decode.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\pezw\wvtpezw_tree_init_encode.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\zte\ztscan_common.hpp
# End Source File
# Begin Source File

SOURCE=.\vtc\zte\ztscan_dec.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\zte\ztscan_enc.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\zte\ztscanUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\vtc\zte\ztscanUtil.hpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\tools\entropy\bytestrm_file.hpp
# End Source File
# Begin Source File

SOURCE=.\idct\idct.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\sadct\sadct.cpp
# End Source File
# End Target
# End Project
