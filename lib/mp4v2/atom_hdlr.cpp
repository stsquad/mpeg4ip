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
 *		Dave Mackie		dmackie@cisco.com
 */

#include "mp4common.h"

MP4HdlrAtom::MP4HdlrAtom() 
	: MP4Atom("hdlr")
{
	AddVersionAndFlags();
	AddReserved("reserved1", 4);
	AddProperty(
		new MP4Integer32Property("handlerType"));
	AddReserved("reserved2", 12);
	AddProperty(
		new MP4StringProperty("name"));
}

// There is a spec incompatiblity between QT and MP4
// QT says name field is a counted string
// MP4 says name field is a null terminated string
// Here we attempt to make all things work
void MP4HdlrAtom::Read() 
{
	u_int32_t numProps = m_pProperties.Size();

	// read all the properties but the "name" field
	ReadProperties(0, numProps - 1);

	// take a peek at the next byte
	u_int8_t strLength;
	m_pFile->PeekBytes(&strLength, 1);

	// if the value matches the remaining atom length
	if (strLength + 1 == m_end - m_pFile->GetPosition()) {
		// read a counted string
		MP4StringProperty* pNameProp = 
			(MP4StringProperty*)m_pProperties[numProps - 1];
		pNameProp->SetCountedFormat(true);
		ReadProperties(numProps - 1, 1);
		pNameProp->SetCountedFormat(false);
	} else {
		// read a null terminated string
		ReadProperties(numProps - 1, 1);
	}

	Skip();	// to end of atom
}
