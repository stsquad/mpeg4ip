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
 *              Bill May     wmay@cisco.com
 *              Charlie Normand  charlienormand@cantab.net alias cpn24
 */


#include "video_util_convert.h"

void convert_yuyv_to_yuv420p (uint8_t *dest, 
			      const uint8_t *src, 
			      uint32_t width, 
			      uint32_t height)
{
  uint8_t *pY = dest;
  uint8_t *pU = pY + (width * height);
  uint8_t *pV = pU + (width * height)/4;
  uint32_t uv_offset = width * 4 * sizeof(uint8_t);
  uint32_t ix, jx;

  for (ix = 0; ix < height; ix++) {
    for (jx = 0; jx < width; jx += 2) {
      uint16_t calc;
      *pY++ = *src++;
      if ((ix&1) == 0) {
	calc = *src;
	calc += *(src + uv_offset);
	calc /= 2;
	*pU++ = (uint8_t) calc;
      }
      src++;
      *pY++ = *src++;
      if ((ix&1) == 0) {
	calc = *src;
	calc += *(src + uv_offset);
	calc /= 2;
	*pV++ = (uint8_t) calc;
      }
      src++;
    }
  }
}

void convert_uyvy_to_yuv420p (uint8_t *dest, 
			      const uint8_t *src, 
			      uint32_t width, 
			      uint32_t height)
{
  uint8_t *pY = dest;
  uint8_t *pU = pY + (width * height);
  uint8_t *pV = pU + (width * height)/4;
  uint32_t uv_offset = width * 4 * sizeof(uint8_t);
  uint32_t ix, jx;

  for (ix = 0; ix < height; ix++) {
    for (jx = 0; jx < width; jx += 2) {
      uint16_t calc;
      if ((ix&1) == 0) {
	calc = *src;
	calc += *(src + uv_offset);
	calc /= 2;
	*pU++ = (uint8_t) calc;
      }
      src++;
      *pY++ = *src++;
      if ((ix&1) == 0) {
	calc = *src;
	calc += *(src + uv_offset);
	calc /= 2;
	*pV++ = (uint8_t) calc;
      }
      src++;
      *pY++ = *src++;
    }
  }
}
void convert_yyuv_to_yuv420p (uint8_t *dest, 
			      const uint8_t *src, 
			      uint32_t width, 
			      uint32_t height)
{
  uint8_t *pY = dest;
  uint8_t *pU = pY + (width * height);
  uint8_t *pV = pU + (width * height)/4;
  uint32_t uv_offset = width * 4 * sizeof(uint8_t);
  uint32_t ix, jx;

  for (ix = 0; ix < height; ix++) {
    for (jx = 0; jx < width; jx += 2) {
      *pY++ = *src++;
      *pY++ = *src++;
      uint16_t calc;
      if ((ix&1) == 0) {
	calc = *src;
	calc += *(src + uv_offset);
	calc /= 2;
	*pU++ = (uint8_t) calc;
	src++;
	calc = *src;
	calc += *(src + uv_offset);
	calc /= 2;
	*pV++ = (uint8_t) calc;
	src++;
      } else src += 2;
    }
  }
}

void convert_nv12_to_yuv420p (uint8_t *dest,
			      const uint8_t *src,
			      uint32_t Ysize,
			      uint32_t width, 
			      uint32_t height)
{
  // Y plane
  uint8_t *u_dest, *v_dest;
  const uint8_t *uv_src;
  uint32_t ix, jx;

  memcpy(dest, src, Ysize);
  width /= 2;
  height /= 2;

  dest += Ysize;
  src += Ysize;

  uv_src = src;

  u_dest = dest;
  v_dest = dest + (width * height);

  for (jx = 0; jx < height; jx++) {
    for (ix = 0; ix < height; ix++) {
      *u_dest++ = *uv_src++;
      *v_dest++ = *uv_src++;
    }
  }
}
