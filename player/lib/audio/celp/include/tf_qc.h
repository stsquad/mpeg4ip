/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
 "This software module was originally developed by 
 Fraunhofer Gesellschaft IIS / University of Erlangen (UER) in the course of 
 development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 
 14496-1,2 and 3. This software module is an implementation of a part of one or more 
 MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 
 Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio 
 standards free license to this software module or modifications thereof for use in 
 hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
 Audio  standards. Those intending to use this software module in hardware or 
 software products are advised that this use may infringe existing patents. 
 The original developer of this software module and his/her company, the subsequent 
 editors and their companies, and ISO/IEC have no liability for use of this software 
 module or modifications thereof in an implementation. Copyright is not released for 
 non MPEG-2 NBC/MPEG-4 Audio conforming products.The original developer
 retains full right to use the code for his/her  own purpose, assign or donate the 
 code to a third party and to inhibit third party from using the code for non 
 MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
 be included in all copies or derivative works." 
 Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

/* CREATED BY :  Bernhard Grill -- June-96  */

#ifdef __cplusplus
extern "C" {
#endif

void tf_init_encode_spectrum( void );

int tf_encode_spectrum( 
   double      *p_spectrum[MAX_TIME_CHANNELS],
   double      allowed_dist[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
   WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS],
   int         sfb_width_table[MAX_TIME_CHANNELS][MAX_SCFAC_BANDS],
   int         nr_of_sfb[MAX_TIME_CHANNELS],
   int         average_block_bits,
   int         available_bitreservoir_bits,
   int         padding_limit,
   BsBitStream *fixed_stream,
   BsBitStream *var_stream,
   int         nr_of_ch
);

void tf_init_decode_spectrum( long sampling_rate );

int tf_decode_spectrum(
   double      *p_spectrum[MAX_TIME_CHANNELS],
   BsBitStream *fixed_stream,
   BsBitStream *var_stream,
   int         nr_of_chan,
   int         bits_avail,
   WINDOW_SEQUENCE windowSequence[MAX_TIME_CHANNELS]
);

#ifdef __cplusplus
}
#endif




