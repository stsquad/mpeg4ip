/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		  dmackie@cisco.com
 *              Alix Marchandise-Franquet alix@cisco.com
 */

#ifndef __MP4CREATOR_INCLUDED__
#define __MP4CREATOR_INCLUDED__ 

#include <mpeg4ip.h>
#include <mp4.h>
#include <mp4av.h>
#include <ismacryplib.h>

#ifdef NODEBUG
#define ASSERT(expr)
#else
#include <assert.h>
#define ASSERT(expr)	assert(expr)
#endif

// exit status
#define EXIT_SUCESS		0
#define EXIT_COMMAND_LINE	1
#define EXIT_CREATE_FILE	2
#define EXIT_CREATE_MEDIA	3
#define EXIT_CREATE_HINT	4
#define EXIT_OPTIMIZE_FILE	5
#define EXIT_EXTRACT_TRACK	6	
#define EXIT_INFO		7
#define EXIT_ISMACRYP_INIT     8
#define EXIT_ISMACRYP_END      9

// global variables
#ifdef MP4CREATOR_GLOBALS
#define MP4CREATOR_GLOBAL
#else
#define MP4CREATOR_GLOBAL extern
#endif

MP4CREATOR_GLOBAL char* ProgName;
MP4CREATOR_GLOBAL u_int32_t Verbosity;
MP4CREATOR_GLOBAL double VideoFrameRate;
MP4CREATOR_GLOBAL u_int32_t Mp4TimeScale;
MP4CREATOR_GLOBAL bool TimeScaleSpecified;
MP4CREATOR_GLOBAL bool VideoProfileLevelSpecified;
MP4CREATOR_GLOBAL int VideoProfileLevel;
MP4CREATOR_GLOBAL bool aacUseOldFile;
MP4CREATOR_GLOBAL int aacProfileLevel;
#endif /* __MP4CREATOR_INCLUDED__ */
