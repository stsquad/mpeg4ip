# Microsoft Developer Studio Project File - Name="ISOencoder" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ISOencoder - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "encoder60.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "encoder60.mak" CFG="ISOencoder - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ISOencoder - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ISOencoder - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ISOencoder - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\encoder_"
# PROP BASE Intermediate_Dir ".\encoder_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\release"
# PROP Intermediate_Dir ".\release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\..\..\..\include" /I "..\.." /I "..\..\type" /I "..\..\sys" /I "..\..\sys\encoder" /I "..\..\tools" /I "..\..\vtc\include" /I "..\..\vtc\ztq" /I "..\..\vtc\wavelet" /I "..\..\vtc\pezw" /I "..\..\vtc\zte" /I "..\..\..\..\..\include" /I "..\..\vtc\shape" /I "..\..\sys\decoder" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__PC_COMPILER_" /D "_MBCS" /D "_MFC_" /D "__TRACE_AND_STATS_" /D "__DOUBLE_PRECISION_" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:".\release/encoder.exe"

!ELSEIF  "$(CFG)" == "ISOencoder - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\encoder0"
# PROP BASE Intermediate_Dir ".\encoder0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\debug"
# PROP Intermediate_Dir ".\debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /GX /ZI /Od /I "..\..\..\..\..\include" /I "..\.." /I "..\..\type" /I "..\..\sys" /I "..\..\sys\encoder" /I "..\..\tools" /I "..\..\vtc\include" /I "..\..\vtc\ztq" /I "..\..\vtc\wavelet" /I "..\..\vtc\pezw" /I "..\..\vtc\zte" /I "..\..\vtc\shape" /I "..\..\sys\decoder" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__PC_COMPILER_" /D "_MBCS" /D "__TRACE_AND_STATS_" /D "__DOUBLE_PRECISION_" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:".\debug/encoder.exe"

!ENDIF 

# Begin Target

# Name "ISOencoder - Win32 Release"
# Name "ISOencoder - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\..\vtc\zte\ac.cpp
# End Source File
# Begin Source File

SOURCE=..\..\type\basic.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\shape\BinArCodec.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\zte\bitpack.cpp
# End Source File
# Begin Source File

SOURCE=..\..\tools\entropy\bitstrm.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\blkenc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\block.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\cae.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\dct.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\ztq\decQM.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\wavelet\download_filter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\wavelet\dwt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\wavelet\dwt_aux.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\wavelet\dwtmask.cpp
# End Source File
# Begin Source File

SOURCE=.\encoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\ztq\encQM.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\errenc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\error.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\ztq\errorHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\type\geom.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\gmc_enc_util.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\gmc_motion.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\gmc_util.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\gme_for_iso.cpp
# End Source File
# Begin Source File

SOURCE=..\..\type\grayc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\type\grayf.cpp
# End Source File
# Begin Source File

SOURCE=..\..\type\grayi.cpp
# End Source File
# Begin Source File

SOURCE=..\..\tools\entropy\huffman.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\wavelet\idwt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\wavelet\idwt_aux.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\wavelet\idwtmask.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\wavelet\imagebox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\mb.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\mbenc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\mbinterlace.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\mc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\mcenc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\mcpad.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\mode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\motest.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\ztq\msg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\mv.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\mvenc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\newpenc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\newpred.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\padenc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\paramset.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\pezw\PEZW_ac.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\pezw\PEZW_globals.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\pezw\PEZW_textureLayerBQ.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\pezw\PEZW_utils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\ztq\QMInit.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\ztq\QMUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\ztq\quant.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\main\read_image.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\rrv.cpp
# End Source File
# Begin Source File

SOURCE=..\..\tools\sadct\sadct.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\main\seg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\sesenc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\shape.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\shape\ShapeBaseCommon.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\shape\ShapeBaseDecode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\shape\ShapeBaseEncode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\shape\ShapeDecoding.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\shape\ShapeEncoding.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\shape\ShapeEnhCommon.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\shape\ShapeEnhDecode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\shape\ShapeEnhEncode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\shape\ShapeUtil.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\shpenc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\spt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\sptenc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\type\svd.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\tm5rc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\tps_bfshape.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\tps_enhcbuf.cpp
# End Source File
# Begin Source File

SOURCE=..\..\type\typeapi.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\ztq\Utils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\type\vop.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\vopmbenc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\vopseenc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\vopses.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\main\vtcdec.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\main\vtcenc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\type\warp.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\main\wavelet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\main\write_image.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\pezw\wvtPEZW.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\pezw\wvtpezw_tree_decode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\pezw\wvtpezw_tree_encode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\pezw\wvtpezw_tree_init_decode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\pezw\wvtpezw_tree_init_encode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\type\yuvac.cpp
# End Source File
# Begin Source File

SOURCE=..\..\type\yuvai.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\zte\ztscan_dec.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\zte\ztscan_enc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\zte\ztscanUtil.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\..\vtc\zte\ac.hpp
# End Source File
# Begin Source File

SOURCE=..\..\type\basic.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\zte\bitpack.hpp
# End Source File
# Begin Source File

SOURCE=..\..\tools\entropy\bitstrm.hpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\blkenc.hpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\block.hpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\cae.h
# End Source File
# Begin Source File

SOURCE=..\..\sys\codehead.h
# End Source File
# Begin Source File

SOURCE=..\..\vtc\zte\context.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\include\dataStruct.hpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\dct.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\wavelet\default.h
# End Source File
# Begin Source File

SOURCE=..\..\vtc\wavelet\download_filter.h
# End Source File
# Begin Source File

SOURCE=..\..\vtc\wavelet\dwt.h
# End Source File
# Begin Source File

SOURCE=..\..\vtc\wavelet\dwt.hpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\enhcbufenc.hpp
# End Source File
# Begin Source File

SOURCE=..\..\tools\entropy\entropy.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\ztq\errorHandler.hpp
# End Source File
# Begin Source File

SOURCE=..\..\type\geom.hpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\global.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\ztq\globals.hpp
# End Source File
# Begin Source File

SOURCE=..\..\type\grayc.hpp
# End Source File
# Begin Source File

SOURCE=..\..\type\grayf.hpp
# End Source File
# Begin Source File

SOURCE=..\..\type\grayi.hpp
# End Source File
# Begin Source File

SOURCE=..\..\tools\entropy\huffman.hpp
# End Source File
# Begin Source File

SOURCE=..\..\tools\entropy\inbits.h
# End Source File
# Begin Source File

SOURCE=..\..\sys\mb.hpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\mbenc.hpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\mode.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\ztq\msg.hpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\paramset.h
# End Source File
# Begin Source File

SOURCE=..\..\vtc\pezw\PEZW_ac.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\pezw\PEZW_functions.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\pezw\PEZW_mpeg4.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\pezw\PEZW_zerotree.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\ztq\QM.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\ztq\QMUtils.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\ztq\quant.hpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\sesenc.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\zte\startcode.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\include\states.hpp
# End Source File
# Begin Source File

SOURCE=..\..\type\svd.h
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\tm5rc.hpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\tps_enhcbuf.hpp
# End Source File
# Begin Source File

SOURCE=..\..\type\transf.hpp
# End Source File
# Begin Source File

SOURCE=..\..\type\typeapi.h
# End Source File
# Begin Source File

SOURCE=..\..\vtc\ztq\Utils.hpp
# End Source File
# Begin Source File

SOURCE=..\..\tools\entropy\vlc.hpp
# End Source File
# Begin Source File

SOURCE=..\..\type\vop.hpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\vopmbenc.hpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\encoder\vopseenc.hpp
# End Source File
# Begin Source File

SOURCE=..\..\sys\vopses.hpp
# End Source File
# Begin Source File

SOURCE=..\..\type\warp.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\wavelet\wvtfilter.h
# End Source File
# Begin Source File

SOURCE=..\..\vtc\pezw\wvtPEZW.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\pezw\wvtpezw_tree_codec.hpp
# End Source File
# Begin Source File

SOURCE=..\..\type\yuvac.hpp
# End Source File
# Begin Source File

SOURCE=..\..\type\yuvai.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\zte\ztscan_common.hpp
# End Source File
# Begin Source File

SOURCE=..\..\vtc\zte\ztscanUtil.hpp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
