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

#include "mp4.h"

extern "C" MP4FileHandle MP4Open(char* fileName, char* mode)
{
	MP4File* pFile;
	try {
 		pFile = new MP4File(fileName, mode);
		return (MP4FileHandle)pFile;
	}
	catch (MP4Error* e) {
		return (MP4FileHandle)NULL;
	}
}

extern "C" int MP4Read(MP4FileHandle pFile)
{
	try {
		((MP4File*)pFile)->Read();
		return 0;
	}
	catch (MP4Error* e) {
		return -1;
	}
}

extern "C" int MP4Write(MP4FileHandle pFile)
{
	try {
		((MP4File*)pFile)->Write();
		return 0;
	}
	catch (MP4Error* e) {
		return -1;
	}
}

extern "C" int MP4Close(MP4FileHandle pFile)
{
	delete (MP4File*)pFile;
	return 0;
}

extern "C" int MP4Dump(MP4FileHandle pFile, FILE* pDumpFile)
{
	try {
		((MP4File*)pFile)->Dump(pDumpFile);
		return 0;
	}
	catch (MP4Error* e) {
		return -1;
	}
}

extern "C" u_int32_t MP4GetVerbosity(MP4FileHandle pFile)
{
	return ((MP4File*)pFile)->GetVerbosity();
}

extern "C" void MP4SetVerbosity(MP4FileHandle pFile, u_int32_t verbosity)
{
	return ((MP4File*)pFile)->SetVerbosity(verbosity);
}

extern "C" u_int64_t MP4GetIntegerProperty(
	MP4FileHandle pFile, char* propName)
{
	return ((MP4File*)pFile)->GetIntegerProperty(propName);
}

extern "C" const char* MP4GetStringProperty(
	MP4FileHandle pFile, char* propName)
{
	return ((MP4File*)pFile)->GetStringProperty(propName);
}

extern "C" void MP4GetBytesProperty(
	MP4FileHandle pFile, char* propName, 
	u_int8_t** ppValue, u_int32_t* pValueSize)
{
	((MP4File*)pFile)->GetBytesProperty(propName, ppValue, pValueSize);
}
