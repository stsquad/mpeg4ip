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
 * Copyright (C) Cisco Systems Inc. 2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May, wmay@cisco.com
 */

#ifndef __VIDEO_UTIL_CONVERT_H__
#define __VIDEO_UTIL_CONVERT_H__ 1

#include "mpeg4ip.h"

#ifdef __cplusplus
extern "C" {
#endif

  void convert_yuyv_to_yuv420p(uint8_t *dest, 
			       const uint8_t *src, 
			       uint32_t width, 
			       uint32_t height);
  void convert_uyvy_to_yuv420p(uint8_t *dest, 
			       const uint8_t *src, 
			       uint32_t width, 
			       uint32_t height);
  void convert_yyuv_to_yuv420p(uint8_t *dest, 
			       const uint8_t *src, 
			       uint32_t width, 
			       uint32_t height);
  void convert_nv12_to_yuv420p(uint8_t *dest,
			       const uint8_t *src,
			       uint32_t Ysize,
			       uint32_t width, 
			       uint32_t height);
#ifdef __cplusplus
}
#endif
#endif
