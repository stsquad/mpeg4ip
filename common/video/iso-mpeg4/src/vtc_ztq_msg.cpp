/* $Id: vtc_ztq_msg.cpp,v 1.1 2005/05/09 21:29:49 wmaycisco Exp $ */

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

/************************************************************/
/*     Sarnoff Very Low Bit Rate Still Image Coder          */
/*     Copyright 1995, 1996, 1997, 1998 Sarnoff Corporation */
/************************************************************/

/*  Common Interface to error-reporting, warnings, and informational notes.
    noteStat()        :  for statistics file (no newline).
    noteDebug()       :  for debugging stuff.
    noteDetail()      :  for detailed running commentary.
    noteProgressNoNL():  for running commentary (no newline).
    noteProgress()    :  for running commentary.
    noteWarning()     :  for warning messages.
    noteError()       :  for problems which can't be continued from.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "basic.hpp"
#include "dataStruct.hpp"
#include "msg.hpp"

/* The filename where user messages (non-error) are written to */
FILE *ofp=stdout;

/* The filename where user error messages are written to */
FILE *efp=stderr;

/* The filename where statistics info. is written to */
FILE *sfp;

/* Filter variable for user messages */
Int quiet=QUIET_DETAIL;

Void CVTCCommon::noteStat(Char *s, ...)
{
  va_list ap;

  va_start(ap, s);
  vfprintf(sfp, s, ap);
  fflush(sfp);
  va_end(ap);
}

Void CVTCCommon::noteDebug(Char *s, ...)
{
  va_list ap;

  if (quiet<QUIET_DEBUG)
  {

    va_start(ap, s);
    vfprintf(ofp, s, ap);
    fprintf(ofp, "\n");
    fflush(ofp);
    va_end(ap);
  }
}

Void CVTCCommon::noteDetail(Char *s, ...)
{
  va_list ap;

  if (quiet<QUIET_DETAIL)
  {

    va_start(ap, s);
    vfprintf(ofp, s, ap);
    fprintf(ofp, "\n");
    fflush(ofp);
    va_end(ap);
  }
}

Void CVTCCommon::noteProgress(Char *s, ...)
{
  va_list ap;

  if (quiet<QUIET_PROGRESS)
  {
    va_start(ap, s);
    vfprintf(ofp, s, ap);
    fprintf(ofp, "\n");
    fflush(ofp);
    va_end(ap);
  }
}


Void CVTCCommon::noteProgressNoNL(Char *s, ...)
{
  va_list ap;

  if (quiet<QUIET_PROGRESS)
  {
    va_start(ap, s);
    vfprintf(ofp, s, ap);
    fflush(ofp);
    va_end(ap);
  }
}


Void CVTCCommon::noteWarning(Char *s, ...)
{
  va_list ap;

  if (quiet<QUIET_WARNINGS)
  {
    va_start(ap, s);
    fprintf(ofp, "Warning:  ");
    vfprintf(ofp, s, ap);
    fprintf(ofp, "\n");
    fflush(ofp);
    va_end(ap);
  }
}


Void CVTCCommon::noteError(Char *s, ...)
{
  va_list ap;

  if (quiet<QUIET_ERRORS)
  {
    va_start(ap, s);
    fprintf(efp, "Error:  ");
    vfprintf(efp, s, ap);
    fprintf(efp, "\n");
    fflush(efp);
    va_end(ap);
  }
}

Void CVTCCommon::noteErrorNoPre(Char *s, ...)
{
  va_list ap;

  if (quiet<QUIET_ERRORS)
  {
    va_start(ap, s);
    vfprintf(efp, s, ap);
    va_end(ap);
  }
}
