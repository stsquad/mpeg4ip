/*
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin.  See the accompanying file "COPYRIGHT" for
 * details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 */

/* $Header: /cvsroot/mpeg4ip/mpeg4ip/server/lib/sndfile/src/GSM610/Attic/gsm_destroy.c,v 1.2 2001/05/09 21:04:38 cahighlander Exp $ */

#include "gsm.h"
#include "config.h"
#include "proto.h"

#ifdef	HAS_STDLIB_H
#	include	<stdlib.h>
#else
#	ifdef	HAS_MALLOC_H
#		include 	<malloc.h>
#	else
		extern void free();
#	endif
#endif

void gsm_destroy P1((S), gsm S)
{
	if (S) free((char *)S);
}
