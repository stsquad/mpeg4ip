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
 * Andrea Graziani
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// debug.h //

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifndef _DECORE
#ifdef _DEBUG

extern void _SetPrintCond(
	int picnum_start, int picnum_end, 
	int mba_start, int mba_end);
extern void _Print(const char * format, ...);
extern void _Break(int picnum, int mba);
extern void _Error(const char * format, ...);

#else

#define _SetPrintCond(a, b, c, d) 
#define _Break(a, b) 
#define _Print(...)  
#define _Error(...)

#endif // _DEBUG
#else

#define _SetPrintCond(a, b, c, d) 
#define _Break(a, b) 
#ifdef _WINDOWS
#define _Print()
#define _Error()
#else
#define _Print(...)  
#define _Error(...) 
#endif

#endif // ! _DECORE
#endif // _DEBUG_H_
