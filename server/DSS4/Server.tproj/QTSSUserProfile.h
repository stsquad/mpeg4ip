/*
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * Copyright (c) 1999-2001 Apple Computer, Inc.  All Rights Reserved. The
 * contents of this file constitute Original Code as defined in and are
 * subject to the Apple Public Source License Version 1.2 (the 'License').
 * You may not use this file except in compliance with the License.  Please
 * obtain a copy of the License at http://www.apple.com/publicsource and
 * read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.  Please
 * see the License for the specific language governing rights and
 * limitations under the License.
 *
 *
 * @APPLE_LICENSE_HEADER_END@
 *
 */
/*
	File:		QTSSUserProfile.h

	Contains:	An object to store User Profile, for authentication
				and authorization
				
				Implements the RTSP Request dictionary for QTSS API.
	
	
*/


#ifndef __QTSSUSERPROFILE_H__
#define __QTSSUSERPROFILE_H__

//INCLUDES:
#include "QTSS.h"
#include "QTSSDictionary.h"
#include "StrPtrLen.h"

class QTSSUserProfile : public QTSSDictionary
{
	public:

		//Initialize
		//Call initialize before instantiating this class. For maximum performance, this class builds
		//any response header it can at startup time.
		static void 		Initialize();
		
		//CONSTRUCTOR & DESTRUCTOR
		QTSSUserProfile();
		virtual ~QTSSUserProfile() {}
		
	protected:
		
		enum
		{
			kMaxUserProfileNameLen 		= 32,
			kMaxUserProfilePasswordLen 	= 32
		};
		
		char	fUserNameBuf[kMaxUserProfileNameLen];		// Set by RTSPRequest object
		char	fUserPasswordBuf[kMaxUserProfilePasswordLen];// Set by authentication module through API

		//Dictionary support
		static QTSSAttrInfoDict::AttrInfo	sAttributes[];
};
#endif // __QTSSUSERPROFILE_H__

