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
 * $Id: util.c,v 1.1 2001/06/28 23:54:22 wmaycisco Exp $
 */

#include "all.h"

/*
 * object_type dependent parameters
 */
int tns_max_bands(faacDecHandle hDecoder, int islong)
{
    int i;

    /* construct second index */
    i = islong ? 0 : 1;
    
    return tns_max_bands_tbl[hDecoder->mc_info.sampling_rate_idx][i];
}

int tns_max_order(faacDecHandle hDecoder, int islong)
{
	if (hDecoder->isMpeg4) {
		switch (hDecoder->mc_info.object_type) {
		case AACMAIN:
		case AACLC:
		case AACLTP:
			if (islong) {
				if (hDecoder->mc_info.sampling_rate_idx <= 5) /* sr > 32000Hz */
					return 12;
				else
					return 20;
			} else
				return 7;
		case AACSSR:
			return 12;
		}
	} else {
		if (islong) {
			switch (hDecoder->mc_info.object_type) {
			case AACMAIN:
				return 20;
			case AACLC:
			case AACSSR:
				return 12;
			}
		} else { /* MPEG2 short window */
			return 7;
		}
    }

    return 0;
}

int pred_max_bands(faacDecHandle hDecoder)
{
    return pred_max_bands_tbl[hDecoder->mc_info.sampling_rate_idx];
}

int stringcmp(char const *str1, char const *str2, unsigned long len)
{
	signed int c1 = 0, c2 = 0;

	while (len--) {
		c1 = *str1++;
		c2 = *str2++;

		if (c1 == 0 || c1 != c2)
			break;
	}

	return c1 - c2;
}
