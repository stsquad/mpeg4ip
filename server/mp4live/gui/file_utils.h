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
 * Copyright (C) Cisco Systems Inc. 2000-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 * 		Bill May        wmay@cisco.com
 * 		Dave Mackie 	dmackie@cisco.com
 */

#ifndef __FILE_UTILS_INCLUDED__
#define __FILE_UTILS_INCLUDED__

GtkWidget* CreateFileCombo(const char* entryText);

void FileBrowser(
	GtkWidget* fileEntry, 
	GtkWidget* mainwindow,
	GtkSignalFunc okFunc = (GtkSignalFunc)NULL);

GtkWidget* CreateTrackMenu(
	GtkWidget* menu,
	char type, 
	const char* source,
	u_int32_t* pMenuIndex,
	u_int32_t* pMenuNumber,
	u_int32_t** ppMenuValues);

bool IsDevice(const char* name);
bool IsUrl(const char* name);
bool IsMp4File(const char* name);
bool IsMpeg2File(const char* name);

int32_t FileDefaultAudio(const char* fileName);

#endif /* __FILE_UTILS_INCLUDED__ */
