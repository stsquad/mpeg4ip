/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	opendivx api wrapper
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
 *	26.02.2001	fixed dec_csp bugs
 *	26.12.2001	xvid_init() support
 *	22.12.2001	removed some compiler warnings
 *	16.12.2001	inital version; (c)2001 peter ross <pross@cs.rmit.edu.au>
 *
 *************************************************************************/


#include <stdlib.h>
#include <string.h>     // memset

#include "xvid.h"
#include "divx4.h"
#include "decoder.h"
#include "encoder.h"

#define EMULATED_DIVX_VERSION 20011001

// decore


typedef struct DINST
{
	unsigned long key;
	struct DINST * next;

	void * handle;
	XVID_DEC_FRAME xframe;

} DINST;

	
static DINST * dhead = NULL;

static
DINST * dinst_find(unsigned long key)
{
	DINST * dcur = dhead;

	while (dcur)
	{
		if (dcur->key == key)
		{
			return dcur;
		}
		dcur = dcur->next;
	}

	return NULL;
}

static
DINST * dinst_add(unsigned long key)
{
	DINST * dnext = dhead;

	dhead = malloc(sizeof(DINST));
	if (dhead == NULL)
	{
		dhead = dnext;
		return NULL;
	}

	dhead->key = key;
	dhead->next = dnext;

	return dhead;
}

static
void dinst_remove(unsigned long key)
{
	DINST * dcur = dhead;

	if (dhead == NULL)
	{
		return;
	}

	if (dcur->key == key)
	{
		dhead = dcur->next;
		free(dcur);
		return;
	}

	while (dcur->next)
	{
		if (dcur->next->key == key)
		{
			DINST * tmp = dcur->next;
			dcur->next = tmp->next;
			free(tmp);
			return;
		}
		dcur = dcur->next;
	}
}

static
int xvid_to_opendivx_dec_csp(int csp)
{
	switch(csp)
	{
	case DEC_YV12 :
		return XVID_CSP_YV12;
	case DEC_420 :
		return XVID_CSP_I420;
	case DEC_YUY2 :
		return XVID_CSP_YUY2;
	case DEC_UYVY :
		return XVID_CSP_UYVY;
	case DEC_RGB32 :
		return XVID_CSP_VFLIP | XVID_CSP_RGB32;
	case DEC_RGB24 :
		return XVID_CSP_VFLIP | XVID_CSP_RGB24;
	case DEC_RGB565 :
		return XVID_CSP_VFLIP | XVID_CSP_RGB565;
	case DEC_RGB555 :
		return XVID_CSP_VFLIP | XVID_CSP_RGB555;
	case DEC_RGB32_INV :
		return XVID_CSP_RGB32;
	case DEC_RGB24_INV :
		return XVID_CSP_RGB24;
	case DEC_RGB565_INV :
		return XVID_CSP_RGB565;
	case DEC_RGB555_INV :
		return XVID_CSP_RGB555;
	case DEC_USER :
		return XVID_CSP_USER;    
	default :
		return -1;
	}
}


int decore(unsigned long key, unsigned long opt,
					void * param1, void * param2)
{
	int xerr;

	switch (opt)
	{
	case DEC_OPT_MEMORY_REQS :
		{
			memset(param2, 0, sizeof(DEC_MEM_REQS));
			return DEC_OK;
		}

	case DEC_OPT_INIT :
		{
			DEC_PARAM * dparam = (DEC_PARAM *)param1;
			XVID_INIT_PARAM xinit;
			XVID_DEC_PARAM xparam;
			DINST * dcur = dinst_find(key);
			if (dcur == NULL)
			{
				dcur = dinst_add(key);
			}

			xinit.cpu_flags = 0;
			xvid_init(NULL, 0, &xinit, NULL);

			xparam.width = dparam->x_dim;
			xparam.height = dparam->y_dim;
			dcur->xframe.colorspace = xvid_to_opendivx_dec_csp(dparam->output_format);
				
			xerr = decoder_create(&xparam);

			dcur->handle = xparam.handle;
			
			break;
		}

	case DEC_OPT_RELEASE :
		{
			DINST * dcur = dinst_find(key);
			if (dcur == NULL)
			{
				return DEC_EXIT;
			}
			
			xerr = decoder_destroy(dcur->handle);

			dinst_remove(key);

			break;
		}
	
	case DEC_OPT_SETPP :
		{
			// DEC_SET * dset = (DEC_SET *)param1;
			DINST * dcur = dinst_find(key);
			if (dcur == NULL)
			{
				return DEC_EXIT;
			}

			// dcur->xframe.pp = dset->postproc_level;

			return DEC_OK;
		}

	case DEC_OPT_SETOUT :
		{
			DEC_PARAM * dparam = (DEC_PARAM *)param1;
			DINST * dcur = dinst_find(key);
			if (dcur == NULL)
			{
				return DEC_EXIT;
			}

			dcur->xframe.colorspace = xvid_to_opendivx_dec_csp(dparam->output_format);

			return DEC_OK;
		}
	
	case DEC_OPT_FRAME:
		{
			int csp_tmp = 0;

			DEC_FRAME * dframe = (DEC_FRAME *)param1;
			DINST * dcur = dinst_find(key);
			if (dcur == NULL)
			{
				return DEC_EXIT;
			}

			dcur->xframe.bitstream = dframe->bitstream;
			dcur->xframe.length = dframe->length;
			dcur->xframe.image = dframe->bmp;
			dcur->xframe.stride = dframe->stride;
			
			if (!dframe->render_flag)
			{
				csp_tmp = dcur->xframe.colorspace;
				dcur->xframe.colorspace = XVID_CSP_NULL;
			}
			
			xerr = decoder_decode(dcur->handle, &dcur->xframe);

			if (!dframe->render_flag)
			{
				dcur->xframe.colorspace = csp_tmp;
			}

			break;
		}


	case DEC_OPT_FRAME_311 :
		return DEC_EXIT;


	case DEC_OPT_VERSION:
		return EMULATED_DIVX_VERSION;
	default :
		return DEC_EXIT;
	}


	switch(xerr)
	{
	case XVID_ERR_OK : return DEC_OK;
	case XVID_ERR_MEMORY : return DEC_MEMORY;
	case XVID_ERR_FORMAT : return DEC_BAD_FORMAT;
	default : // case XVID_ERR_FAIL : 
		return DEC_EXIT;
	}
}




// encore

#define FRAMERATE_INCR		1001

int divx4_motion_presets[7] = {
	0, PMV_QUICKSTOP16, PMV_EARLYSTOP16, PMV_EARLYSTOP16 | PMV_EARLYSTOP8,
	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EARLYSTOP8 | PMV_HALFPELDIAMOND8,
	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EARLYSTOP8 | PMV_HALFPELDIAMOND8,
	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EXTSEARCH16 |
	PMV_EARLYSTOP8 | PMV_HALFPELREFINE8 | PMV_HALFPELDIAMOND8
};

int quality;

int encore(void * handle, int opt, void * param1, void * param2)
{
	int xerr;

	switch(opt)
	{
	case ENC_OPT_INIT :
		{
			ENC_PARAM * eparam = (ENC_PARAM *)param1;
			XVID_INIT_PARAM xinit;
			XVID_ENC_PARAM xparam;

			xinit.cpu_flags = 0;
			xvid_init(NULL, 0, &xinit, NULL);
						
			xparam.width = eparam->x_dim;
			xparam.height = eparam->y_dim;
			if ((eparam->framerate - (int)eparam->framerate) == 0)
			{
				xparam.fincr = 1; 
				xparam.fbase = (int)eparam->framerate;
			}
			else
			{
				xparam.fincr = FRAMERATE_INCR;
				xparam.fbase = (int)(FRAMERATE_INCR * eparam->framerate);
			}
			xparam.bitrate = eparam->bitrate;
			xparam.rc_buffersize = 16;
			xparam.min_quantizer = eparam->min_quantizer;
			xparam.max_quantizer = eparam->max_quantizer;
			xparam.max_key_interval = eparam->max_key_interval;
			quality = eparam->quality;

			xerr = encoder_create(&xparam);

			eparam->handle = xparam.handle;

			break;
		}

	case ENC_OPT_RELEASE :
		{
			xerr = encoder_destroy((Encoder *) handle);
			break;
		}

	case ENC_OPT_ENCODE :
	case ENC_OPT_ENCODE_VBR :
		{
			ENC_FRAME * eframe = (ENC_FRAME *)param1;
			ENC_RESULT * eresult = (ENC_RESULT *)param2;
			XVID_ENC_FRAME xframe;
			XVID_ENC_STATS xstats;

			xframe.bitstream = eframe->bitstream;
			xframe.length = eframe->length;

			xframe.general = XVID_HALFPEL | XVID_H263QUANT;

			if(quality > 3)
				xframe.general |= XVID_INTER4V;

			xframe.motion = divx4_motion_presets[quality];

			xframe.image = eframe->image;
			switch (eframe->colorspace)
			{
			case ENC_CSP_RGB24 : 
				xframe.colorspace = XVID_CSP_VFLIP | XVID_CSP_RGB24;
				break;
			case ENC_CSP_YV12 :
				xframe.colorspace = XVID_CSP_YV12;
				break;
			case ENC_CSP_YUY2 :
				xframe.colorspace = XVID_CSP_YUY2;
				break;
			case ENC_CSP_UYVY :
				xframe.colorspace = XVID_CSP_UYVY;
				break;
			case ENC_CSP_I420 :
				xframe.colorspace = XVID_CSP_I420;
				break;
			}

			if (opt == ENC_OPT_ENCODE_VBR)	
			{
				xframe.intra = eframe->intra;
				xframe.quant = eframe->quant;
			}
			else
			{
				xframe.intra = -1;
				xframe.quant = 0;
			}
			
			xerr = encoder_encode((Encoder *) handle, &xframe, (eresult ? &xstats : NULL) );

			if (eresult)
			{
				eresult->is_key_frame = xframe.intra;
				eresult->quantizer = xstats.quant;
				eresult->total_bits = xframe.length * 8;
				eresult->motion_bits = xstats.hlength * 8;
				eresult->texture_bits = eresult->total_bits - eresult->motion_bits;
			}

			eframe->length = xframe.length;

			break;
		}

	default:
		return ENC_FAIL;
	}

	switch(xerr)
	{
	case XVID_ERR_OK : return ENC_OK;
	case XVID_ERR_MEMORY : return ENC_MEMORY;
	case XVID_ERR_FORMAT : return ENC_BAD_FORMAT;
	default : // case XVID_ERR_FAIL : 
		return ENC_FAIL;
	}
}

