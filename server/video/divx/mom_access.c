
/**************************************************************************
 *                                                                        *
 * This code is developed by Adam Li.  This software is an                *
 * implementation of a part of one or more MPEG-4 Video tools as          *
 * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
 * software module in hardware or software products are advised that its  *
 * use may infringe existing patents or copyrights, and any such use      *
 * would be at such party's own risk.  The original developer of this     *
 * software module and his/her company, and subsequent editors and their  *
 * companies (including Project Mayo), will have no liability for use of  *
 * this software or modifications or derivatives thereof.                 *
 *                                                                        *
 * Project Mayo gives users of the Codec a license to this software       *
 * module or modifications thereof for use in hardware or software        *
 * products claiming conformance to the MPEG-4 Video Standard as          *
 * described in the Open DivX license.                                    *
 *                                                                        *
 * The complete Open DivX license can be found at                         *
 * http://www.projectmayo.com/opendivx/license.php .                      *
 *                                                                        *
 **************************************************************************/

/**************************************************************************
 *
 *  mom_access.c
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

/* This file contains memory access for the Image, Vop and VolConfig data */
/* structures.                                                            */
/* Some codes of this project come from MoMuSys MPEG-4 implementation.    */
/* Please see seperate acknowledgement file for a list of contributors.   */

#include "mom_access.h"

/***********************************************************CommentBegin******
 *
 * -- GetImage{xxx} -- Access components of intermediate formats
 *
 *	Char *GetImageData(Image *image)
 *      UInt GetImageSize(Image *image)
 *      UInt GetImageSizeX(Image *image)
 *      UInt GetImageSizeY(Image *image)
 *      Int GetImageVersion(Image *image)
 *      ImageType GetImageType(Image *image)
 *
 * Purpose :
 *	These are common functions to access specific components
 *      of the common data structures which are used for the
 *      intermediate formats.
 *
 ***********************************************************CommentEnd********/

Char *
GetImageData(Image *image)
{
	switch(GetImageType(image))
	{
		case SHORT_TYPE:
			return((Char *)image->data->s);
			break;
		case FLOAT_TYPE:
			return((Char *)image->data->f);
			break;
		case UCHAR_TYPE:
			return((Char *)image->data->u);
			break;
		default:
			printf("Image type >>%d<< not supported\n",image->type);
			return(NULL);
	}
}


UInt
GetImageSize(Image *image)
{
	return(image->x * image->y);
}


UInt
GetImageSizeX(Image *image)
{
	return(image->x);
}


UInt
GetImageSizeY(Image *image)
{
	return(image->y);
}


Int
GetImageVersion(Image *image)
{
	return(image->version);
}


ImageType
GetImageType(Image *image)
{
	return(image->type);
}



/***********************************************************CommentBegin******
 *
 * -- GetVop{xxx} -- Functions to access components of the Vop structure
 *
 * Purpose :
 *	These are common functions to access specific components
 *      of the Vop data structure.
 *
 ***********************************************************CommentEnd********/


Int GetVopNot8Bit(Vop *vop)
{
	return (vop->bits_per_pixel != 8);
}


Int GetVopQuantPrecision(Vop *vop)
{
	return (vop->quant_precision);
}


Int GetVopBitsPerPixel(Vop *vop)
{
	return (vop->bits_per_pixel);
}


Int GetVopMidGrey(Vop *vop)
{
	return (1 << (vop->bits_per_pixel - 1));
}


Int GetVopBrightWhite(Vop *vop)
{
	return ((1 << vop->bits_per_pixel) - 1);
}


Int GetVopTimeIncrementResolution(Vop *vop)
{
	return (vop->time_increment_resolution);
}


Int
GetVopModTimeBase(Vop *vop)
{
	return(vop->mod_time_base);
}


Int
GetVopTimeInc(Vop *vop)
{
	return((int)vop->time_inc);
}


Int
GetVopPredictionType(Vop *vop)
{
	return(vop->prediction_type);
}


Int GetVopIntraDCVlcThr(Vop *vop)
{
	return (vop->intra_dc_vlc_thr);
}


Int
GetVopRoundingType(Vop *vop)
{
	return(vop->rounding_type);
}


Int
GetVopWidth(Vop *vop)
{
	return(vop->width);
}


Int
GetVopHeight(Vop *vop)
{
	return(vop->height);
}


Int
GetVopHorSpatRef(Vop *vop)
{
	return(vop->hor_spat_ref);
}


Int
GetVopVerSpatRef(Vop *vop)
{
	return(vop->ver_spat_ref);
}


Int
GetVopQuantizer(Vop *vop)
{
	return(vop->quantizer);
}


Int
GetVopIntraQuantizer(Vop *vop)
{
	return(vop->intra_quantizer);
}


Int
GetVopIntraACDCPredDisable(Vop *vop)
{
	return(vop->intra_acdc_pred_disable);
}


Int
GetVopFCodeFor(Vop *vop)
{
	return(vop->fcode_for);
}


Int
GetVopSearchRangeFor(Vop *vop)
{
	return(vop->sr_for);
}


Image *
GetVopY(Vop *vop)
{
	return(vop->y_chan);
}


Image *
GetVopU(Vop *vop)
{
	return(vop->u_chan);
}


Image *
GetVopV(Vop *vop)
{
	return(vop->v_chan);
}


/***********************************************************CommentBegin******
 *
 * -- PutVop{xxx} -- Functions to write to components of the Vop structure
 *
 *	These are common functions to write to specific components
 *      of the Vop structure.
 *
 ***********************************************************CommentEnd********/


Void PutVopQuantPrecision(Int quant_precision,Vop *vop)
{
	vop->quant_precision = quant_precision;
}


Void PutVopBitsPerPixel(Int bits_per_pixel,Vop *vop)
{
	vop->bits_per_pixel = bits_per_pixel;
}


Void PutVopTimeIncrementResolution(Int time_incre_res, Vop *vop)
{
	vop->time_increment_resolution=time_incre_res;
}


Void
PutVopModTimeBase(Int mod_time_base, Vop *vop)
{
	vop->mod_time_base = mod_time_base;
}


Void
PutVopTimeInc(Int time_inc, Vop *vop)
{
	vop->time_inc = (float)time_inc;
}


Void
PutVopPredictionType(Int prediction_type, Vop *vop)
{
	vop->prediction_type = prediction_type;
}


Void PutVopIntraDCVlcThr(Int intra_dc_vlc_thr,Vop *vop)
{
	vop->intra_dc_vlc_thr=intra_dc_vlc_thr;
}


Void
PutVopRoundingType(Int rounding_type, Vop *vop)
{
	vop->rounding_type = rounding_type;
}


Void
PutVopWidth(Int width, Vop *vop)
{
	vop->width = width;
}


Void
PutVopHeight(Int height, Vop *vop)
{
	vop->height = height;
}


Void
PutVopHorSpatRef(Int hor_spat_ref, Vop *vop)
{
	vop->hor_spat_ref = hor_spat_ref;
}


Void
PutVopVerSpatRef(Int ver_spat_ref, Vop *vop)
{
	vop->ver_spat_ref = ver_spat_ref;
}


Void
PutVopQuantizer(Int quantizer, Vop *vop)
{
	vop->quantizer = quantizer;
}


Void
PutVopIntraACDCPredDisable(Int intra_acdc_pred_disable, Vop *vop)
{
	vop->intra_acdc_pred_disable = intra_acdc_pred_disable;
}


Void
PutVopFCodeFor(Int fcode_for, Vop *vop)
{
	vop->fcode_for = fcode_for;
}


Void
PutVopSearchRangeFor(Int sr_for, Vop *vop)
{
	vop->sr_for = sr_for;
}


Void
PutVopY(Image *y_chan, Vop *vop)
{
	FreeImage(vop->y_chan);
	vop->y_chan = y_chan;
}


Void
PutVopU(Image *u_chan, Vop *vop)
{
	FreeImage(vop->u_chan);
	vop->u_chan = u_chan;
}


Void
PutVopV(Image *v_chan, Vop *vop)
{
	FreeImage(vop->v_chan);
	vop->v_chan = v_chan;
}


Void
PutVopIntraQuantizer(Int Q,Vop *vop)
{
	vop->intra_quantizer = Q;
}


/***********************************************************CommentBegin******
 *
 * -- PutVolConfigXXXX -- Access functions for VolConfig
 *
 * Purpose :
 *      To set particular fields in a VolConfig strcuture
 *
 ***********************************************************CommentEnd********/


Void
PutVolConfigFrameRate(Float fr, VolConfig *cfg)
{
	cfg->frame_rate = fr;
}


Void
PutVolConfigM(Int M, VolConfig *cfg)
{
	cfg->M = M;
}


Void
PutVolConfigStartFrame(Int frame, VolConfig *cfg)
{
	cfg->start_frame = frame;
}


Void
PutVolConfigEndFrame(Int frame, VolConfig *cfg)
{
	cfg->end_frame = frame;
}


Void
PutVolConfigBitrate(Int bit_rate,VolConfig *cfg)
{
	cfg->bit_rate = bit_rate;
}


Void
PutVolConfigIntraPeriod(Int ir,VolConfig *cfg)
{
	cfg->intra_period = ir;
}


Void
PutVolConfigQuantizer(Int Q,VolConfig *cfg)
{
	cfg->quantizer = Q;
}


Void
PutVolConfigIntraQuantizer(Int Q,VolConfig *cfg)
{
	cfg->intra_quantizer = Q;
}


Void
PutVolConfigFrameSkip(Int frame_skip,VolConfig *cfg)
{
	cfg->frame_skip = frame_skip;
}


Void
PutVolConfigModTimeBase(Int time,VolConfig *cfg)
{
	cfg->modulo_time_base[0] = cfg->modulo_time_base[1];
	cfg->modulo_time_base[1] = time;
}


/***********************************************************CommentBegin******
 *
 * -- GetVolConfigXXXX -- Access functions for VolConfig
 *
 * Purpose :
 *      To obtain the value of particular fields in a VolConfig structure
 *
 ***********************************************************CommentEnd********/


Float
GetVolConfigFrameRate(VolConfig *cfg)
{
	return(cfg->frame_rate);
}


Int
GetVolConfigM(VolConfig *cfg)
{
	return(cfg->M);
}


Int
GetVolConfigStartFrame(VolConfig *cfg)
{
	return(cfg->start_frame);
}


Int
GetVolConfigEndFrame(VolConfig *cfg)
{
	return(cfg->end_frame);
}


Int
GetVolConfigBitrate(VolConfig *cfg)
{
	return(cfg->bit_rate);
}


Int
GetVolConfigIntraPeriod(VolConfig *cfg)
{
	return(cfg->intra_period);
}


Int
GetVolConfigQuantizer(VolConfig *cfg)
{
	return(cfg->quantizer);
}


Int
GetVolConfigIntraQuantizer(VolConfig *cfg)
{
	return(cfg->intra_quantizer);
}


Int
GetVolConfigFrameSkip(VolConfig *cfg)
{
	return(cfg->frame_skip);
}


Int
GetVolConfigModTimeBase(VolConfig *cfg,Int i)
{
	return(cfg->modulo_time_base[i]);
}

