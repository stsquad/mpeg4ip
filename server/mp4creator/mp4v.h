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
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#ifndef __MP4V_INCLUDED__
#define __MP4V_INCLUDED__

#define VOSH_START	0xB0
#define VOL_START	0x20
#define GOV_START	0xB3
#define VOP_START	0xB6

u_int8_t Mp4vVideoToSystemsProfileLevel(u_int8_t videoProfileLevel);

u_char Mp4vGetVopType(u_int8_t* pVopBuf, u_int32_t vopSize);

#endif /* __MP4V_INCLUDED__ */
