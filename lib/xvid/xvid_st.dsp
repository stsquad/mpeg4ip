# Microsoft Developer Studio Project File - Name="xvid_st" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=xvid_st - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "xvid_st.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xvid_st.mak" CFG="xvid_st - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xvid_st - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "xvid_st - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "xvid_st - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_st"
# PROP Intermediate_Dir "Release_st"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /Ob2 /I "..\..\include" /D "NDEBUG" /D "ARCH_X86" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_WIN32" /D "MPEG4IP" /YX /FD /c
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"xvid_st.lib"

!ELSEIF  "$(CFG)" == "xvid_st - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_st"
# PROP Intermediate_Dir "Debug_st"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\.." /I "..\..\include" /D "_DEBUG" /D "ARCH_X86" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_WIN32" /D "MPEG4IP" /YX /FD /GZ /c
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"xvid_st.lib"

!ENDIF 

# Begin Target

# Name "xvid_st - Win32 Release"
# Name "xvid_st - Win32 Debug"
# Begin Group "docs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=authors.txt
# End Source File
# Begin Source File

SOURCE=gpl.txt
# End Source File
# Begin Source File

SOURCE=todo.txt
# End Source File
# End Group
# Begin Group "bitstream"

# PROP Default_Filter ""
# Begin Group "bitstream_asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=bitstream\x86_asm\cbp_mmx.asm

!IF  "$(CFG)" == "xvid_st - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release_st
InputPath=bitstream\x86_asm\cbp_mmx.asm
InputName=cbp_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "xvid_st - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug_st
InputPath=bitstream\x86_asm\cbp_mmx.asm
InputName=cbp_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "bitstream_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=bitstream\bitstream.h
# End Source File
# Begin Source File

SOURCE=bitstream\cbp.h
# End Source File
# Begin Source File

SOURCE=bitstream\mbcoding.h
# End Source File
# Begin Source File

SOURCE=bitstream\vlc_codes.h
# End Source File
# Begin Source File

SOURCE=bitstream\zigzag.h
# End Source File
# End Group
# Begin Source File

SOURCE=bitstream\bitstream.c
# End Source File
# Begin Source File

SOURCE=bitstream\cbp.c
# End Source File
# Begin Source File

SOURCE=.\bitstream\h263.c
# End Source File
# Begin Source File

SOURCE=.\bitstream\h263.h
# End Source File
# Begin Source File

SOURCE=bitstream\mbcoding.c
# End Source File
# End Group
# Begin Group "dct"

# PROP Default_Filter ""
# Begin Group "dct_asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=dct\x86_asm\fdct_mmx.asm

!IF  "$(CFG)" == "xvid_st - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release_st
InputPath=dct\x86_asm\fdct_mmx.asm
InputName=fdct_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "xvid_st - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug_st
InputPath=dct\x86_asm\fdct_mmx.asm
InputName=fdct_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=dct\x86_asm\idct_mmx.asm

!IF  "$(CFG)" == "xvid_st - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release_st
InputPath=dct\x86_asm\idct_mmx.asm
InputName=idct_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "xvid_st - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug_st
InputPath=dct\x86_asm\idct_mmx.asm
InputName=idct_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "dct_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=dct\fdct.h
# End Source File
# Begin Source File

SOURCE=dct\idct.h
# End Source File
# End Group
# Begin Source File

SOURCE=dct\fdct.c
# End Source File
# Begin Source File

SOURCE=dct\idct.c
# End Source File
# End Group
# Begin Group "image"

# PROP Default_Filter ""
# Begin Group "image_asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=image\x86_asm\interpolate8x8_mmx.asm

!IF  "$(CFG)" == "xvid_st - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release_st
InputPath=image\x86_asm\interpolate8x8_mmx.asm
InputName=interpolate8x8_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "xvid_st - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug_st
InputPath=image\x86_asm\interpolate8x8_mmx.asm
InputName=interpolate8x8_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=image\x86_asm\rgb_to_yv12_mmx.asm

!IF  "$(CFG)" == "xvid_st - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release_st
InputPath=image\x86_asm\rgb_to_yv12_mmx.asm
InputName=rgb_to_yv12_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "xvid_st - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug_st
InputPath=image\x86_asm\rgb_to_yv12_mmx.asm
InputName=rgb_to_yv12_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=image\x86_asm\yuv_to_yv12_mmx.asm

!IF  "$(CFG)" == "xvid_st - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release_st
InputPath=image\x86_asm\yuv_to_yv12_mmx.asm
InputName=yuv_to_yv12_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "xvid_st - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug_st
InputPath=image\x86_asm\yuv_to_yv12_mmx.asm
InputName=yuv_to_yv12_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=image\x86_asm\yuyv_to_yv12_mmx.asm

!IF  "$(CFG)" == "xvid_st - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release_st
InputPath=image\x86_asm\yuyv_to_yv12_mmx.asm
InputName=yuyv_to_yv12_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "xvid_st - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug_st
InputPath=image\x86_asm\yuyv_to_yv12_mmx.asm
InputName=yuyv_to_yv12_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=image\x86_asm\yv12_to_rgb24_mmx.asm

!IF  "$(CFG)" == "xvid_st - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release_st
InputPath=image\x86_asm\yv12_to_rgb24_mmx.asm
InputName=yv12_to_rgb24_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "xvid_st - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug_st
InputPath=image\x86_asm\yv12_to_rgb24_mmx.asm
InputName=yv12_to_rgb24_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=image\x86_asm\yv12_to_rgb32_mmx.asm

!IF  "$(CFG)" == "xvid_st - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release_st
InputPath=image\x86_asm\yv12_to_rgb32_mmx.asm
InputName=yv12_to_rgb32_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "xvid_st - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug_st
InputPath=image\x86_asm\yv12_to_rgb32_mmx.asm
InputName=yv12_to_rgb32_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=image\x86_asm\yv12_to_yuyv_mmx.asm

!IF  "$(CFG)" == "xvid_st - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release_st
InputPath=image\x86_asm\yv12_to_yuyv_mmx.asm
InputName=yv12_to_yuyv_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "xvid_st - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug_st
InputPath=image\x86_asm\yv12_to_yuyv_mmx.asm
InputName=yv12_to_yuyv_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "image_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=image\colorspace.h
# End Source File
# Begin Source File

SOURCE=image\image.h
# End Source File
# Begin Source File

SOURCE=image\interpolate8x8.h
# End Source File
# End Group
# Begin Source File

SOURCE=image\colorspace.c
# End Source File
# Begin Source File

SOURCE=image\image.c
# End Source File
# Begin Source File

SOURCE=image\interpolate8x8.c
# End Source File
# End Group
# Begin Group "motion"

# PROP Default_Filter ""
# Begin Group "motion_asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=motion\x86_asm\sad_mmx.asm

!IF  "$(CFG)" == "xvid_st - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release_st
InputPath=motion\x86_asm\sad_mmx.asm
InputName=sad_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "xvid_st - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug_st
InputPath=motion\x86_asm\sad_mmx.asm
InputName=sad_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "motion_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=motion\motion.h
# End Source File
# Begin Source File

SOURCE=motion\sad.h
# End Source File
# End Group
# Begin Source File

SOURCE=motion\motion_comp.c
# End Source File
# Begin Source File

SOURCE=motion\motion_est.c
# End Source File
# Begin Source File

SOURCE=motion\sad.c
# End Source File
# End Group
# Begin Group "prediction"

# PROP Default_Filter ""
# Begin Source File

SOURCE=prediction\mbprediction.c
# End Source File
# Begin Source File

SOURCE=prediction\mbprediction.h
# End Source File
# End Group
# Begin Group "quant"

# PROP Default_Filter ""
# Begin Group "quant_asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=quant\x86_asm\quantize4_mmx.asm

!IF  "$(CFG)" == "xvid_st - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release_st
InputPath=quant\x86_asm\quantize4_mmx.asm
InputName=quantize4_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "xvid_st - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug_st
InputPath=quant\x86_asm\quantize4_mmx.asm
InputName=quantize4_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=quant\x86_asm\quantize_mmx.asm

!IF  "$(CFG)" == "xvid_st - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release_st
InputPath=quant\x86_asm\quantize_mmx.asm
InputName=quantize_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "xvid_st - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug_st
InputPath=quant\x86_asm\quantize_mmx.asm
InputName=quantize_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "quant_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=quant\adapt_quant.h
# End Source File
# Begin Source File

SOURCE=quant\quant_h263.h
# End Source File
# Begin Source File

SOURCE=quant\quant_matrix.h
# End Source File
# Begin Source File

SOURCE=quant\quant_mpeg4.h
# End Source File
# End Group
# Begin Source File

SOURCE=quant\adapt_quant.c
# End Source File
# Begin Source File

SOURCE=quant\quant_h263.c
# End Source File
# Begin Source File

SOURCE=quant\quant_matrix.c
# End Source File
# Begin Source File

SOURCE=quant\quant_mpeg4.c
# End Source File
# End Group
# Begin Group "utils"

# PROP Default_Filter ""
# Begin Group "utils_asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=utils\x86_asm\cpuid.asm

!IF  "$(CFG)" == "xvid_st - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release_st
InputPath=utils\x86_asm\cpuid.asm
InputName=cpuid

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "xvid_st - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug_st
InputPath=utils\x86_asm\cpuid.asm
InputName=cpuid

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=utils\x86_asm\mem_transfer_mmx.asm

!IF  "$(CFG)" == "xvid_st - Win32 Release"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Release_st
InputPath=utils\x86_asm\mem_transfer_mmx.asm
InputName=mem_transfer_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "xvid_st - Win32 Debug"

# Begin Custom Build - Assembling $(InputPath)
IntDir=.\Debug_st
InputPath=utils\x86_asm\mem_transfer_mmx.asm
InputName=mem_transfer_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "utils_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=utils\emms.h
# End Source File
# Begin Source File

SOURCE=utils\mbfunctions.h
# End Source File
# Begin Source File

SOURCE=utils\mem_align.h
# End Source File
# Begin Source File

SOURCE=utils\mem_transfer.h
# End Source File
# Begin Source File

SOURCE=utils\ratecontrol.h
# End Source File
# Begin Source File

SOURCE=utils\timer.h
# End Source File
# End Group
# Begin Source File

SOURCE=utils\emms.c
# End Source File
# Begin Source File

SOURCE=utils\mbtransquant.c
# End Source File
# Begin Source File

SOURCE=utils\mem_align.c
# End Source File
# Begin Source File

SOURCE=utils\mem_transfer.c
# End Source File
# Begin Source File

SOURCE=utils\ratecontrol.c
# End Source File
# Begin Source File

SOURCE=utils\timer.c
# End Source File
# End Group
# Begin Group "xvidxvid_st_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=decoder.h
# End Source File
# Begin Source File

SOURCE=divx4.h
# End Source File
# Begin Source File

SOURCE=encoder.h
# End Source File
# Begin Source File

SOURCE=global.h
# End Source File
# Begin Source File

SOURCE=portab.h
# End Source File
# Begin Source File

SOURCE=xvid.h
# End Source File
# End Group
# Begin Source File

SOURCE=decoder.c
# End Source File
# Begin Source File

SOURCE=divx4.c
# End Source File
# Begin Source File

SOURCE=encoder.c
# End Source File
# Begin Source File

SOURCE=xvid.c
# End Source File
# End Target
# End Project
