/*
 * Adapted from:
 *
 * H263/MPEG4 backend for ffmpeg encoder and decoder
 * Copyright (c) 2000,2001 Gerard Lantau.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef H263_H_
#define H263_H_

/* #define FFMPEG_VERSION_INT 0x000405 */
/* #define FFMPEG_VERSION     "0.4.5" */

#ifdef WIN32
#define CONFIG_WIN32
#endif

#ifdef CONFIG_WIN32

/* windows */

typedef unsigned short UINT16;
typedef signed short INT16;
typedef unsigned char UINT8;
typedef unsigned int UINT32;
typedef unsigned __int64 UINT64;
typedef signed char INT8;
typedef signed int INT32;
typedef signed __int64 INT64;

typedef UINT8 uint8_t;
typedef INT8 int8_t;
typedef UINT16 uint16_t;
typedef INT16 int16_t;
typedef UINT32 uint32_t;
typedef INT32 int32_t;

#define INT64_C(c)     (c ## i64)
#define UINT64_C(c)    (c ## i64)

#define inline __inline

/*
  Disable warning messages:
    warning C4244: '=' : conversion from 'double' to 'float', possible loss of data
    warning C4305: 'argument' : truncation from 'const double' to 'float'
*/
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )

#define M_PI    3.14159265358979323846
#define M_SQRT2 1.41421356237309504880  /* sqrt(2) */

#ifdef _DEBUG
#define DEBUG
#endif

// code from bits/byteswap.h (C) 1997, 1998 Free Software Foundation, Inc.
#define bswap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#define be2me_32(x) bswap_32(x)

#define snprintf _snprintf

#else

/* unix */

#include <inttypes.h>

#ifndef __WINE_WINDEF16_H
/* workaround for typedef conflict in MPlayer (wine typedefs) */
typedef unsigned short UINT16;
typedef signed short INT16;
#endif

typedef unsigned char UINT8;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
typedef signed char INT8;
typedef signed int INT32;
typedef signed long long INT64;

#endif /* !CONFIG_WIN32 */

typedef struct H263_VLC {
    int bits;
    INT16 *table_codes;
    INT8 *table_bits;
    int table_size, table_allocated;
} H263_VLC;

#endif

