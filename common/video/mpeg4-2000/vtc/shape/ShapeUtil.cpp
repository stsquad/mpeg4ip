/****************************************************************************/
/*   MPEG4 Visual Texture Coding (VTC) Mode Software                        */
/*                                                                          */
/*   This software was jointly developed by the following participants:     */
/*                                                                          */
/*   Single-quant,  multi-quant and flow control                            */
/*   are provided by  Sarnoff Corporation                                   */
/*     Iraj Sodagar   (iraj@sarnoff.com)                                    */
/*     Hung-Ju Lee    (hjlee@sarnoff.com)                                   */
/*     Paul Hatrack   (hatrack@sarnoff.com)                                 */
/*     Shipeng Li     (shipeng@sarnoff.com)                                 */
/*     Bing-Bing Chai (bchai@sarnoff.com)                                   */
/*     B.S. Srinivas  (bsrinivas@sarnoff.com)                               */
/*                                                                          */
/*   Bi-level is provided by Texas Instruments                              */
/*     Jie Liang      (liang@ti.com)                                        */
/*                                                                          */
/*   Shape Coding is provided by  OKI Electric Industry Co., Ltd.           */
/*     Zhixiong Wu    (sgo@hlabs.oki.co.jp)                                 */
/*     Yoshihiro Ueda (yueda@hlabs.oki.co.jp)                               */
/*     Toshifumi Kanamaru (kanamaru@hlabs.oki.co.jp)                        */
/*                                                                          */
/*   OKI, Sharp, Sarnoff, TI and Microsoft contributed to bitstream         */
/*   exchange and bug fixing.                                               */
/*                                                                          */
/*                                                                          */
/* In the course of development of the MPEG-4 standard, this software       */
/* module is an implementation of a part of one or more MPEG-4 tools as     */
/* specified by the MPEG-4 standard.                                        */
/*                                                                          */
/* The copyright of this software belongs to ISO/IEC. ISO/IEC gives use     */
/* of the MPEG-4 standard free license to use this  software module or      */
/* modifications thereof for hardware or software products claiming         */
/* conformance to the MPEG-4 standard.                                      */
/*                                                                          */
/* Those intending to use this software module in hardware or software      */
/* products are advised that use may infringe existing  patents. The        */
/* original developers of this software module and their companies, the     */
/* subsequent editors and their companies, and ISO/IEC have no liability    */
/* and ISO/IEC have no liability for use of this software module or         */
/* modification thereof in an implementation.                               */
/*                                                                          */
/* Permission is granted to MPEG members to use, copy, modify,              */
/* and distribute the software modules ( or portions thereof )              */
/* for standardization activity within ISO/IEC JTC1/SC29/WG11.              */
/*                                                                          */
/* Copyright 1995, 1996, 1997, 1998 ISO/IEC                                 */
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "dataStruct.hpp"
//#include "ShapeUtil.h"

Void * CVTCCommon::
mymalloc(size_t size)
{
    Void *tmp;

    if ( (tmp=malloc(size))==NULL ) {
        fprintf(stderr,"Malloc error.\n");
        exit(1);
    }
    return(tmp);
}

UChar ** CVTCCommon::
malloc_2d_Char(Int d1,Int d2)
{
    UChar **array_2d;
    Int i;

    array_2d = (UChar **) mymalloc(d1*sizeof(UChar *));
    for ( i=0; i<d1; i++ ) {
        array_2d[i] = (UChar *) mymalloc(d2*sizeof(UChar));
    }
    return(array_2d);
}

Void CVTCCommon::
free_2d_Char(UChar **array_2d,Int d1)
{
    Int i;

    for ( i=0; i<d1; i++ ) {
        free(array_2d[i]);
    }
    free(array_2d);
}

Int ** CVTCCommon::
malloc_2d_Int(Int d1,Int d2)
{
    Int **array_2d;
    Int i;

    array_2d = (Int **) mymalloc(d1*sizeof(Int *));
    for ( i=0; i<d1; i++ ) {
        array_2d[i] = (Int *) mymalloc(d2*sizeof(Int));
    }
    return(array_2d);
}

Void CVTCCommon::
free_2d_Int(Int **array_2d,Int d1)
{
    Int i;

    for ( i=0; i<d1; i++ ) {
        free(array_2d[i]);
    }
    free(array_2d);
}
