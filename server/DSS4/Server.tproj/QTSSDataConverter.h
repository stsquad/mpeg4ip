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
	File:		QTSSDataConverter.h

	Contains:	Utility routines for converting to and from
				QTSS_AttrDataTypes and text
				
	Written By:	Denis Serenyi

	Change History (most recent first):
	
*/

#include "QTSS.h"

class QTSSDataConverter
{
	public:
	
		//
		// This function converts a type string, eg, "UInt32" to the enum, qtssAttrDataTypeUInt32
		static QTSS_AttrDataType TypeStringToType(const char* inTypeString);
		
		//
		// This function does the opposite conversion
		static char*	TypeToTypeString(const QTSS_AttrDataType inType);
		
		//
		// This function converts a text-formatted value of a certain type
		// to its type. Returns: QTSS_NotEnoughSpace if the buffer provided
		// is not big enough.
		
		// String must be NULL-terminated.
		// If output value is a string, it will not be NULL-terminated
		static QTSS_Error	StringToValue(const char* inValueAsString,
												const QTSS_AttrDataType inType,
												void* ioBuffer,
												UInt32* ioBufSize);
		
		// If value is a string, doesn't have to be NULL-terminated.
		// Output string will be NULL terminated.
		static char* ValueToString(	const void* inValue,
											const UInt32 inValueLen,
											const QTSS_AttrDataType inType);
		
private:

		// Takes a pointer to buffer and converts to hex in high to low order
		static char* ConvertBytesToCHexString(const void* inValue, const UInt32 inValueLen);
		
		// Takes a string of hex values and converts to bytes in high to low order
		static QTSS_Error ConvertCHexStringToBytes(	const char* inValueAsString,
													void* ioBuffer,
													UInt32* ioBufSize);
};

