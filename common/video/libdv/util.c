/* 
 *  util.c
 *
 *     Copyright (C) Charles 'Buck' Krasic - January 2001
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  libdv is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your
 *  option) any later version.
 *   
 *  libdv is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *  The libdv homepage is http://libdv.sourceforge.net/.  
 */

#include "util.h"

#if HAVE_LIBPOPT
void
dv_opt_usage(poptContext con, struct poptOption *opt, gint num)
{
  struct poptOption *o = opt + num;
  if(o->shortName && o->longName) {
    fprintf(stderr,"-%c, --%s", o->shortName, o->longName);
  } else if(o->shortName) {
    fprintf(stderr,"-%c", o->shortName);
  } else if(o->longName) {
    fprintf(stderr,"--%s", o->longName);
  } // if
  if(o->argDescrip) {
    fprintf(stderr, "=%s\n", o->argDescrip);
  } else {
    fprintf(stderr, ": invalid usage\n");
  } // else
  exit(-1);
} // dv_opt_usage
#endif // HAVE_LIBPOPT
