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
 *              Bill May        wmay@cisco.com
 */
/*
 * rawv.h - interface to rawv library
 */
#ifndef __RAWV_H__
#define __RAWV_H__ 1
#include "codec_plugin.h"

#define m_vft c.v.video_vft
#define m_ifptr c.ifptr

typedef struct rawv_codec_t {
  codec_data_t c;
  int m_h;
  int m_w;
} rawv_codec_t;
  
#endif
/* end file rawv.h */
