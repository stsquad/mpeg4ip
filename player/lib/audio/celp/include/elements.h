/**********************************************************************
MPEG-4 Audio VM
Bit stream module

This software module was originally developed by

Pierre Lauber (Fraunhofer Institut of Integrated Circuits)

and edited by

Ali Nowbakht-Irani (Fraunhofer Gesellschaft IIS)
Ralph Sperschneider (Fraunhofer Gesellschaft IIS)

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools
as specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 NBC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 NBC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
NBC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 NBC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.


This file contains all permitted bitstream elements. The macro E is
used to obtain an enumerated constant and a string constant 
simultaneous, which was the primary intension. The sequence is not
significant to the software.
**********************************************************************/
E(ID_SYN_ELE),
E(ELEMENT_INSTANCE_TAG),
E(GLOBAL_GAIN_MSB_1_5),
E(GLOBAL_GAIN_MSB_6_7),
E(GLOBAL_GAIN_MSB_8),
E(WINDOW_SEQUENCE_CODE),
E(WINDOW_SHAPE_CODE),
E(MAX_SFB),
E(PREDICTOR_DATA_PRESENT),
E(PREDICTOR_RESET),
E(PREDICTOR_RESET_GROUP_NUMBER),
E(PREDICTION_USED),
E(SECT_CB),
E(SECT_LEN_INCR),
E(HCOD_SF),
E(PULSE_DATA_PRESENT),
E(NUMBER_PULSE),
E(PULSE_START_SFB),
E(PULSE_OFFSET),
E(PULSE_AMP),
E(TNS_DATA_PRESENT),
E(N_FILT),
E(COEF_RES),
E(TNS_LENGTH),
E(ORDER),
E(DIRECTION),
E(COEF_COMPRESS),
E(TNS_COEF),
E(GAIN_CONTROL_DATA_PRESENT),
E(MAX_BAND),
E(ADJUST_NUM),
E(ALEVCODE),
E(ALOCCODE),
E(HCOD),
E(REORDERED_SPECTRAL_DATA),
E(SIGN_BITS),
E(ESCAPE_BITS),
E(COMMON_WINDOW),
E(MS_MASK_PRESENT),
E(MS_USED),
E(DATA_BYTE_ALIGN_FLAG),
E(COUNT),
E(ESC_COUNT),
E(DATA_STREAM_BYTE),
E(FILL_BYTE),
E(LENGTH_OF_LONGEST_CODEWORD),
E(LENGTH_OF_REORDERED_SPECTRAL_DATA),
E(ICS_RESERVED),
E(SCALE_FACTOR_GROUPING),
E(DPCM_NOISE_NRG),
E(PROFILE),
E(SAMPLING_FREQUENCY_INDEX),
E(NUM_FRONT_CHANNEL_ELEMENTS),
E(NUM_SIDE_CHANNEL_ELEMENTS),
E(NUM_BACK_CHANNEL_ELEMENTS),
E(NUM_LFE_CHANNEL_ELEMENTS),
E(NUM_ASSOC_DATA_ELEMENTS),
E(NUM_VALID_CC_ELEMENTS),
E(MONO_MIXDOWN_PRESENT),
E(MONO_MIXDOWN_ELEMENT_NUMBER),
E(STEREO_MIXDOWN_PRESENT),
E(STEREO_MIXDOWN_ELEMENT_NUMBER),
E(MATRIX_MIXDOWN_IDX_PRESENT),
E(MATRIX_MIXDOWN_IDX),
E(PSEUDO_SURROUND_ENABLE),
E(FRONT_SIDE_BACK_ELEMENT_IS_CPE),
E(FRONT_SIDE_BACK_LFE_DATA_CC_ELEMENT_TAG_SELECT),
E(COMMENT_FIELD_BYTES),
E(COMMENT_FIELD_DATA),
E(ADIF_ID),
E(COPYRIGHT_ID_PRESENT),
E(COPYRIGHT_ID),
E(ORIGINAL_COPY),
E(HOME),
E(BITSTREAM_TYPE),
E(BITRATE),
E(NUM_PROGRAM_CONFIG_ELEMENTS),
E(ADIF_BUFFER_FULLNESS),
E(IND_SW_CCE_FLAG),
E(NUM_COUPLED_ELEMENTS),
E(CC_TARGET_IS_CPE),
E(CC_TARGET_TAG_SELECT),
E(CC_L),
E(CC_R),
E(CC_DOMAIN),
E(GAIN_ELEMENT_SIGN),
E(GAIN_ELEMENT_SCALE),
E(COMMON_GAIN_ELEMENT_PRESENT),
E(LTP_DATA),
E(ALIGN_BITS),
E(REV_GLOBAL_GAIN),
E(LENGTH_OF_RVLC_SF),
E(RVLC_CODE),
E(SF_ESCAPES_PRESENT),
E(LENGTH_OF_RVLC_ESCAPES),
E(ESCAPE_CODE),
E(LENGTH_OF_SF_DATA),
E(SF_CONCEALMENT),
