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
 *	17.03.2002	Added interpolate8x8_halfpel_hv_xmm
 *  22.12.2001  API change: added xvid_init() - Isibaar
 *	16.12.2001	inital version; (c)2001 peter ross <pross@cs.rmit.edu.au>
 *
 *************************************************************************/


#include "xvid.h"
#include "decoder.h"
#include "encoder.h"
#include "bitstream/cbp.h"
#include "dct/idct.h"
#include "dct/fdct.h"
#include "image/colorspace.h"
#include "image/interpolate8x8.h"
#include "utils/mem_transfer.h"
#include "quant/quant_h263.h"
#include "quant/quant_mpeg4.h"
#include "motion/sad.h"
#include "utils/emms.h"
#include "utils/timer.h"
#include "bitstream/mbcoding.h"

#ifdef MPEG4IP
#ifndef _WIN32
#include <signal.h>
#include <setjmp.h>

static jmp_buf env;
static int i_illegal;
static void IllegalSignalHandler(int i_signal)
{
  i_illegal = 1;
  longjmp(env, 1);
}
#endif
#endif

int xvid_init(void *handle, int opt, void *param1, void *param2)
{
	int cpu_flags;
	XVID_INIT_PARAM *init_param;

	init_param = (XVID_INIT_PARAM *) param1;

	// force specific cpu settings?
	if((init_param->cpu_flags & XVID_CPU_FORCE) > 0)
		cpu_flags = init_param->cpu_flags;
	else {

#ifdef ARCH_X86
		cpu_flags = check_cpu_features();
#else
		cpu_flags = 0;
#endif
		init_param->cpu_flags = cpu_flags;
	}

	// initialize the function pointers
	idct_int32_init();
	init_vlc_tables();

	fdct = fdct_int32;
	idct = idct_int32;

	sadInit = 0;
	
	emms = emms_c;

	quant_intra = quant_intra_c;
	dequant_intra = dequant_intra_c;
	quant_inter = quant_inter_c;
	dequant_inter = dequant_inter_c;

	quant4_intra = quant4_intra_c;
	dequant4_intra = dequant4_intra_c;
	quant4_inter = quant4_inter_c;
	dequant4_inter = dequant4_inter_c;

	transfer_8to16copy = transfer_8to16copy_c;
	transfer_16to8copy = transfer_16to8copy_c;
	transfer_8to16sub = transfer_8to16sub_c;
	transfer_16to8add = transfer_16to8add_c;
	transfer8x8_copy = transfer8x8_copy_c;

	interpolate8x8_halfpel_h = interpolate8x8_halfpel_h_c;
	interpolate8x8_halfpel_v = interpolate8x8_halfpel_v_c;
	interpolate8x8_halfpel_hv = interpolate8x8_halfpel_hv_c;

	colorspace_init();

	rgb555_to_yv12 = rgb555_to_yv12_c;
	rgb565_to_yv12 = rgb565_to_yv12_c;
	rgb24_to_yv12 = rgb24_to_yv12_c;
	rgb32_to_yv12 = rgb32_to_yv12_c;
	yuv_to_yv12 = yuv_to_yv12_c;
	yuyv_to_yv12 = yuyv_to_yv12_c;
	uyvy_to_yv12 = uyvy_to_yv12_c;

	yv12_to_rgb555 = yv12_to_rgb555_c;
	yv12_to_rgb565 = yv12_to_rgb565_c;
	yv12_to_rgb24 = yv12_to_rgb24_c;
	yv12_to_rgb32 = yv12_to_rgb32_c;
	yv12_to_yuv = yv12_to_yuv_c;
	yv12_to_yuyv = yv12_to_yuyv_c;
	yv12_to_uyvy = yv12_to_uyvy_c;

	calc_cbp = calc_cbp_c;
	sad16 = sad16_c;
	sad8 = sad8_c;
	dev16 = dev16_c;

#ifdef ARCH_X86
	if((cpu_flags & XVID_CPU_MMX) > 0) {
		fdct = fdct_mmx;
		idct = idct_mmx;

		emms = emms_mmx;

		quant_intra = quant_intra_mmx;
		dequant_intra = dequant_intra_mmx;
		quant_inter = quant_inter_mmx;
		dequant_inter = dequant_inter_mmx;

		quant4_intra = quant4_intra_mmx;
		dequant4_intra = dequant4_intra_mmx;
		quant4_inter = quant4_inter_mmx;
		dequant4_inter = dequant4_inter_mmx;

		transfer_8to16copy = transfer_8to16copy_mmx;
		transfer_16to8copy = transfer_16to8copy_mmx;
		transfer_8to16sub = transfer_8to16sub_mmx;
		transfer_16to8add = transfer_16to8add_mmx;
		transfer8x8_copy = transfer8x8_copy_mmx;

		interpolate8x8_halfpel_h = interpolate8x8_halfpel_h_mmx;
		interpolate8x8_halfpel_v = interpolate8x8_halfpel_v_mmx;
		interpolate8x8_halfpel_hv = interpolate8x8_halfpel_hv_mmx;

		rgb24_to_yv12 = rgb24_to_yv12_mmx;
		rgb32_to_yv12 = rgb32_to_yv12_mmx;
		yuv_to_yv12 = yuv_to_yv12_mmx;
		yuyv_to_yv12 = yuyv_to_yv12_mmx;
		uyvy_to_yv12 = uyvy_to_yv12_mmx;

		yv12_to_rgb24 = yv12_to_rgb24_mmx;
		yv12_to_rgb32 = yv12_to_rgb32_mmx;
		yv12_to_yuyv = yv12_to_yuyv_mmx;
		yv12_to_uyvy = yv12_to_uyvy_mmx;

		calc_cbp = calc_cbp_mmx;
		sad16 = sad16_mmx;
		sad8 = sad8_mmx;
		dev16 = dev16_mmx;

	}

	if((cpu_flags & XVID_CPU_MMXEXT) > 0) {
		idct = idct_xmm;
		interpolate8x8_halfpel_h = interpolate8x8_halfpel_h_xmm;
		interpolate8x8_halfpel_v = interpolate8x8_halfpel_v_xmm;
		interpolate8x8_halfpel_hv = interpolate8x8_halfpel_hv_xmm;
		yuv_to_yv12 = yuv_to_yv12_xmm;

		sad16 = sad16_xmm;
		sad8 = sad8_xmm;
		dev16 = dev16_xmm;

	}

	if((cpu_flags & XVID_CPU_3DNOW) > 0) {
		interpolate8x8_halfpel_h = interpolate8x8_halfpel_h_3dn;
		interpolate8x8_halfpel_v = interpolate8x8_halfpel_v_3dn;
		interpolate8x8_halfpel_hv = interpolate8x8_halfpel_hv_3dn;
	}

#ifdef MPEG4IP
#ifndef _WIN32
	// Thanks to Steve Schultz
	signal(SIGILL, IllegalSignalHandler);
	i_illegal = 0;
	if (setjmp(env) == 0) {
	  __asm__ __volatile__ ( "xorps %%xmm0,%%xmm0\n" : :);
	}
	signal(SIGILL, NULL);
	if (i_illegal != 0) {
	  cpu_flags &= ~XVID_CPU_SSE2;
	  printf("Disabling SSE2\n");
	}
#endif
	if((cpu_flags & XVID_CPU_SSE2) > 0) {
		sad16 = sad16_sse2;
	}
#endif

#endif
#if 0
	// should be defined(ARCH_PPC) || defined(ARCH_PPC_ALTIVEC)
#ifdef ARCH_PPC_ALTIVEC
	calc_cbp = calc_cbp_altivec;
	fdct = fdct_altivec;
	idct = idct_altivec;
	sadInit = sadInit_altivec;
	sad16 = sad16_altivec;
	sad8 = sad8_altivec;
	dev16 = dev16_altivec;
#else
	calc_cbp = calc_cbp_ppc;
#endif
#endif
	
	// API version
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

	case XVID_DEC_ALLOC:
	  return decoder_alloc((XVID_DEC_PARAM *)param1);

	case XVID_DEC_FIND_VOL:
	  return decoder_find_vol((DECODER *)handle, 
				  (XVID_DEC_FRAME *)param1,
				  (XVID_DEC_PARAM *)param2);

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
