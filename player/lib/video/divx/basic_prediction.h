/**************************************************************************
 *                                                                        *
 * This code has been developed by Andrea Graziani. This software is an   *
 * implementation of a part of one or more MPEG-4 Video tools as          *
 * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
 * software module in hardware or software products are advised that its  *
 * use may infringe existing patents or copyrights, and any such use      *
 * would be at such party's own risk.  The original developer of this     *
 * software module and his/her company, and subsequent editors and their  *
 * companies (including Project Mayo), will have no liability for use of  *
 * this software or modifications or derivatives thereof.                 *
 *                                                                        *
 * Project Mayo gives users of the Codec a license to this software       *
 * module or modifications thereof for use in hardware or software        *
 * products claiming conformance to the MPEG-4 Video Standard as          *
 * described in the Open DivX license.                                    *
 *                                                                        *
 * The complete Open DivX license can be found at                         *
 * http://www.projectmayo.com/opendivx/license.php                        *
 *                                                                        *
 **************************************************************************/
/**
*  Copyright (C) 2001 - Project Mayo
 *
 * Andrea Graziani (Ag)
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
**/

#ifndef _BASIC_PREDICTION_H_
#define _BASIC_PREDICTION_H_

void CopyMemBlock(
	unsigned char * Src, 
	unsigned char * Dst, 
	int BlockHeight, 
	int BlockWidth, 
	int Stride);

void CopyMemBlockHor(
	unsigned char * Src, 
	unsigned char * Dst, 
	int BlockHeight, 
	int BlockWidth, 
	int Stride,
	int Round);

void CopyMemBlockVer(
	unsigned char * Src, 
	unsigned char * Dst, 
	int BlockHeight, 
	int BlockWidth, 
	int Stride,
	int Round);

void CopyMemBlockHorVer(
	unsigned char * Src, 
	unsigned char * Dst, 
	int BlockHeight, 
	int BlockWidth, 
	int Stride,
	int Round);

void CopyBlock(unsigned char * Src, unsigned char * Dst, int Stride);
void CopyBlockHor(unsigned char * Src, unsigned char * Dst, int Stride);
void CopyBlockVer(unsigned char * Src, unsigned char * Dst, int Stride);
void CopyBlockHorVer(unsigned char * Src, unsigned char * Dst, int Stride);
void CopyBlockHorRound(unsigned char * Src, unsigned char * Dst, int Stride);
void CopyBlockVerRound(unsigned char * Src, unsigned char * Dst, int Stride);
void CopyBlockHorVerRound(unsigned char * Src, unsigned char * Dst, int Stride);
/**/
void CopyMBlock(unsigned char * Src, unsigned char * Dst, int Stride);
void CopyMBlockHor(unsigned char * Src, unsigned char * Dst, int Stride);
void CopyMBlockVer(unsigned char * Src, unsigned char * Dst, int Stride);
void CopyMBlockHorVer(unsigned char * Src, unsigned char * Dst, int Stride);
void CopyMBlockHorRound(unsigned char * Src, unsigned char * Dst, int Stride);
void CopyMBlockVerRound(unsigned char * Src, unsigned char * Dst, int Stride);
void CopyMBlockHorVerRound(unsigned char * Src, unsigned char * Dst, int Stride);

#endif // _BASIC_PREDICTION_H_
