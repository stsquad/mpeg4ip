/******************************************************************************
 *                                                                          
 * This software module was originally developed by 
 *
 *   Fujitsu Laboratories Ltd. (contact: Eishi Morimatsu)
 *
 * in the course of development of the MPEG-4 Video (ISO/IEC 14496-2) standard.
 * This software module is an implementation of a part of one or more MPEG-4 
 * Video (ISO/IEC 14496-2) tools as specified by the MPEG-4 Video (ISO/IEC 
 * 14496-2) standard. 
 *
 * ISO/IEC gives users of the MPEG-4 Video (ISO/IEC 14496-2) standard free 
 * license to this software module or modifications thereof for use in hardware
 * or software products claiming conformance to the MPEG-4 Video (ISO/IEC 
 * 14496-2) standard. 
 *
 * Those intending to use this software module in hardware or software products
 * are advised that its use may infringe existing patents. THE ORIGINAL 
 * DEVELOPER OF THIS SOFTWARE MODULE AND HIS/HER COMPANY, THE SUBSEQUENT 
 * EDITORS AND THEIR COMPANIES, AND ISO/IEC HAVE NO LIABILITY FOR USE OF THIS 
 * SOFTWARE MODULE OR MODIFICATIONS THEREOF. No license to this software
 * module is granted for non MPEG-4 Video (ISO/IEC 14496-2) standard 
 * conforming products.
 *
 * Fujitsu Laboratories Ltd. retains full right to use the software module for 
 * his/her own purpose, assign or donate the software module to a third party 
 * and to inhibit third parties from using the code for non MPEG-4 Video 
 * (ISO/IEC 14496-2) standard conforming products. This copyright notice must 
 * be included in all copies or derivative works of the software module. 
 *
 * Copyright (c) 1999.  
 *

Module Name:

	rrv.hpp

Abstract:

	List of utility functions for Reduced Resolution Vop (RRV) mode

Revision History:

 *****************************************************************************/
#ifndef __RRU_HPP__
#define __RRU_HPP__

extern Void	MotionVectorScalingDown(CMotionVector*, Int, Int);
extern Void	MotionVectorScalingUp(CMotionVector*,Int, Int);
extern Void	DownSamplingTextureForRRV(PixelC*, PixelC*,	Int, Int);
extern Void	DownSamplingTextureForRRV(PixelI*, PixelI*, Int, Int);
extern Void	UpSamplingTextureForRRV(PixelC*, PixelC*, Int, Int, Int);
extern Void	UpSamplingTextureForRRV(PixelI*, PixelI*, Int, Int, Int);
extern Void	writeCubicRct(Int, Int, PixelC*, PixelC*);
extern Void	writeCubicRct(Int, Int, PixelI*, PixelI*);
extern Void	MeanUpSampling(PixelC*, PixelC*, Int, Int);
extern Void	MeanUpSampling(PixelI*, PixelI*, Int, Int);

#endif
