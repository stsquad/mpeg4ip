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

MP4RtpAtom::MP4RtpAtom() 
	: MP4Atom("rtp ")
{
	AddProperty(
		new MP4Integer32Property("descriptionFormat"));
	AddProperty(
		new MP4StringProperty("sdpText"));
}

void MP4RtpAtom::Read() 
{
	ReadProperties(0, 1);

	/* read sdp string, length is implicit in size of atom */
	u_int64_t size = m_end - m_pFile->GetPosition();
	char* data = (char*)MP4Malloc(size + 1);
	m_pFile->ReadBytes((u_int8_t*)data, size);
	data[size] = '\0';
	((MP4StringProperty*)m_pProperties[1])->SetValue(data);
	MP4Free(data);
}

// TBD rtp sdp write
