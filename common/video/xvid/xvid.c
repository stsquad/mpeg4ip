/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	native api
 *
 *	This program is an implementation of a part of one or more MPEG-4
 *	Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
 *	to use this software module in hardware or software products are
 *	advised that its use may infringe existing patents or copyrights, and
 *	any such use would be at such party's own risk.  The original
 *	developer of this software module and his/her company, and subsequent
 *	editors and their companies, will have no liability for use of this
 *	software or modifications or derivatives thereof.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

/**************************************************************************
 *
 *	History:
 *
 *  22.12.2001  API change: added xvid_init() - Isibaar
 *	16.12.2001	inital version; (c)2001 peter ross <pross@cs.rmit.edu.au>
 *
 *************************************************************************/


#include "xvid.h"
#include "decoder/decoder.h"
#include "encoder/encoder.h"

int xvid_init(void *handle, int opt, void *param1, void *param2)
{
	int cpu_flags;
	XVID_INIT_PARAM *init_param;

	init_param = (XVID_INIT_PARAM *) param1;

	// force specific cpu settings?
	if((init_param->cpu_flags & XVID_CPU_FORCE) > 0)
		cpu_flags = init_param->cpu_flags;
	else {

#ifdef USE_MMX
		cpu_flags = check_cpu_features();
#else
		cpu_flags = 0;
#endif
		init_param->cpu_flags = cpu_flags;
	}

	// initialize the different modules
	init_image(cpu_flags);
	init_common(cpu_flags);
	init_decoder(cpu_flags);
	init_encoder(cpu_flags);
	
	// API version: 1.0
	init_param->api_version = API_VERSION;

	// something clever has to be done for this
	init_param->core_build = 1000;

	return XVID_ERR_OK;
}

int xvid_decore(void * handle, int opt, void * param1, void * param2)
{
	switch (opt)
	{
	case XVID_DEC_DECODE :
	return decoder_decode((DECODER *) handle, (XVID_DEC_FRAME *) param1);

	case XVID_DEC_CREATE :
	return decoder_create((XVID_DEC_PARAM *) param1);
	
	case XVID_DEC_DESTROY :
	return decoder_destroy((DECODER *) handle);

	default:
	return XVID_ERR_FAIL;
    }
}


int xvid_encore(void * handle, int opt, void * param1, void * param2)
{
	switch (opt)
	{
	case XVID_ENC_ENCODE :
	return encoder_encode((Encoder *) handle, (XVID_ENC_FRAME *) param1, (XVID_ENC_STATS *) param2);

	case XVID_ENC_CREATE :
	return encoder_create((XVID_ENC_PARAM *) param1);
	
	case XVID_ENC_DESTROY :
	return encoder_destroy((Encoder *) handle);

	default:
	return XVID_ERR_FAIL;
    }
}
