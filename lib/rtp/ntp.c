/*
 * FILE:    ntp.h
 * AUTHOR:  O.Hodson
 * 
 * NTP utility functions to make rtp and rtp round time calculation
 * a little less painful.
 *
 * Copyright (c) 2000 University College London
 * All rights reserved.
 *
 * $Id: ntp.c,v 1.1 2001/08/01 00:34:01 wmaycisco Exp $
 */

#include "config_unix.h"
#include "config_win32.h"
#include "gettimeofday.h"

#include "ntp.h"

#define SECS_BETWEEN_1900_1970 2208988800u

void 
ntp64_time(uint32_t *ntp_sec, uint32_t *ntp_frac)
{
        struct timeval now;

        gettimeofday(&now, NULL);

        /* NB ntp_frac is in units of 1 / (2^32 - 1) secs. */
        *ntp_sec  = now.tv_sec + SECS_BETWEEN_1900_1970;
        *ntp_frac = (now.tv_usec << 12) + (now.tv_usec << 8) - ((now.tv_usec * 3650) >> 6);
}
