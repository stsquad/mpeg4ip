/*
 * FAAD - Freeware Advanced Audio Decoder
 * Copyright (C) 2001 Menno Bakker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: util.h,v 1.3 2003/01/23 22:33:51 wmaycisco Exp $
 */

#ifndef _UTIL_H_
#define _UTIL_H_

int tns_max_bands(faacDecHandle hDecoder, int islong);
int tns_max_order(faacDecHandle hDecoder, int islong);
int pred_max_bands(faacDecHandle hDecoder);
int stringcmp(char const *str1, char const *str2, unsigned long len);

/* Memory functions */
#ifdef _WIN32
#define AllocMemory(size) LocalAlloc(LPTR, size)
#define FreeMemory(block) LocalFree(block)
//#define SetMemory(block, value, size) FillMemory(block, size, value)
#define SetMemory(block, value, size) memset(block, value, size)
#else
#define AllocMemory(size) malloc(size)
#define FreeMemory(block) free(block)
#define SetMemory(block, value, size) memset(block, value, size)
#define CopyMemory(dest, source, len) memcpy(dest, source, len)
#endif

#endif
