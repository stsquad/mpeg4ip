/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000  Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@devolution.com
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id: default_cursor.h,v 1.1 2001/02/05 20:26:29 cahighlander Exp $";
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Default cursor - it happens to be the Mac cursor, but could be anything   */

#define DEFAULT_CWIDTH	16
#define DEFAULT_CHEIGHT	16
#define DEFAULT_CHOTX	0
#define DEFAULT_CHOTY	0


static unsigned char default_cdata[] =
{
 0x00,0x00,
 0x40,0x00,
 0x60,0x00,
 0x70,0x00,
 0x78,0x00,
 0x7C,0x00,
 0x7E,0x00,
 0x7F,0x00,
 0x7F,0x80,
 0x7C,0x00,
 0x6C,0x00,
 0x46,0x00,
 0x06,0x00,
 0x03,0x00,
 0x03,0x00,
 0x00,0x00
};

static unsigned char default_cmask[] =
{
 0x40,0x00,
 0xE0,0x00,
 0xF0,0x00,
 0xF8,0x00,
 0xFC,0x00,
 0xFE,0x00,
 0xFF,0x00,
 0xFF,0x80,
 0xFF,0xC0,
 0xFF,0x80,
 0xFE,0x00,
 0xEF,0x00,
 0x4F,0x00,
 0x07,0x80,
 0x07,0x80,
 0x03,0x00
};
