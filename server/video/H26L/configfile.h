/*
***********************************************************************
* COPYRIGHT AND WARRANTY INFORMATION
*
* Copyright 2001, International Telecommunications Union, Geneva
*
* DISCLAIMER OF WARRANTY
*
* These software programs are available to the user without any
* license fee or royalty on an "as is" basis. The ITU disclaims
* any and all warranties, whether express, implied, or
* statutory, including any implied warranties of merchantability
* or of fitness for a particular purpose.  In no event shall the
* contributor or the ITU be liable for any incidental, punitive, or
* consequential damages of any kind whatsoever arising from the
* use of these programs.
*
* This disclaimer of warranty extends to the user of these programs
* and user's customers, employees, agents, transferees, successors,
* and assigns.
*
* The ITU does not represent or warrant that the programs furnished
* hereunder are free of infringement of any third-party patents.
* Commercial implementations of ITU-T Recommendations, including
* shareware, may be subject to royalty fees to patent holders.
* Information regarding the ITU-T patent policy is available from
* the ITU Web site at http://www.itu.int.
*
* THIS IS NOT A GRANT OF PATENT RIGHTS - SEE THE ITU-T PATENT POLICY.
************************************************************************
*/

/*!
 ***********************************************************************
 *  \file
 *     configfile.h
 *  \brief
 *     Prototypes for configfile.c and definitions of used structures.
 ***********************************************************************
 */

#ifndef _CONFIGFILE_H_
#define _CONFIGFILE_H_


#define DEFAULTCONFIGFILENAME "encoder.cfg"

typedef struct {
  char *TokenName;
  void *Place;
  int Type;
} Mapping;



InputParameters configinput;


#ifdef INCLUDED_BY_CONFIGFILE_C

Mapping Map[] = {
    {"IntraPeriod",              &configinput.intra_period,            0},
    {"FramesToBeEncoded",        &configinput.no_frames,               0},
    {"QPFirstFrame",             &configinput.qp0,                     0},
    {"QPRemainingFrame",         &configinput.qpN,                     0},
    {"FrameSkip",                &configinput.jumpd,                   0},
    {"MVResolution",             &configinput.mv_res,                  0},
    {"UseHadamard",              &configinput.hadamard,                0},
    {"SearchRange",              &configinput.search_range,            0},
    {"NumberRefereceFrames",     &configinput.no_multpred,             0},
    {"SourceWidth",              &configinput.img_width,               0},
    {"SourceHeight",             &configinput.img_height,              0},
    {"MbLineIntraUpdate",        &configinput.intra_upd,               0},
    {"SliceMode",                &configinput.slice_mode,              0},
    {"SliceArgument",            &configinput.slice_argument,          0},
    {"UseConstrainedIntraPred",  &configinput.UseConstrainedIntraPred, 0},
    {"InputFile",                &configinput.infile,                  1},
    {"InputHeaderLength",        &configinput.infile_header,           0},
    {"OutputFile",               &configinput.outfile,                 1},
    {"ReconFile",                &configinput.ReconFile,               1},
    {"TraceFile",                &configinput.TraceFile,               1},
    {"NumberBFrames",            &configinput.successive_Bframe,       0},
    {"QPBPicture",               &configinput.qpB,                     0},
    {"SPPicturePeriodicity",     &configinput.sp_periodicity,          0},
    {"QPSPPicture",              &configinput.qpsp,                    0},
    {"QPSP2Picture",             &configinput.qpsp_pred,               0},
    {"SymbolMode",               &configinput.symbol_mode,             0},
    {"OutFileMode",              &configinput.of_mode,                 0},
    {"PartitionMode",            &configinput.partition_mode,          0},
    {"SequenceHeaderType",       &configinput.SequenceHeaderType,      0},
    {"TRModulus",                &configinput.TRModulus,               0},
    {"PicIdModulus",             &configinput.PicIdModulus,            0},
    {"PictureTypeSequence",      &configinput.PictureTypeSequence,     1},
    {"InterSearch16x16",         &configinput.InterSearch16x16,        0},
    {"InterSearch16x8",          &configinput.InterSearch16x8 ,        0},
    {"InterSearch8x16",          &configinput.InterSearch8x16,         0},
    {"InterSearch8x8",           &configinput.InterSearch8x8 ,         0},
    {"InterSearch8x4",           &configinput.InterSearch8x4,          0},
    {"InterSearch4x8",           &configinput.InterSearch4x8,          0},
    {"InterSearch4x4",           &configinput.InterSearch4x4,          0},
#ifdef _FULL_SEARCH_RANGE_
    {"RestrictSearchRange",      &configinput.full_search,             0},
#endif
#ifdef _ADAPT_LAST_GROUP_
    {"LastFrameNumber",          &configinput.last_frame,              0},
#endif
#ifdef _CHANGE_QP_
    {"ChangeQPP",                &configinput.qpN2,                    0},
    {"ChangeQPB",                &configinput.qpB2,                    0},
    {"ChangeQPStart",            &configinput.qp2start,                0},
#endif
    {"RDOptimization",           &configinput.rdopt,                   0},
    {"LossRate",                 &configinput.LossRate,                0},
    {"NumberOfDecoders",         &configinput.NoOfDecoders,            0},
#ifdef _ADDITIONAL_REFERENCE_FRAME_
    {"AdditionalReferenceFrame", &configinput.add_ref_frame,           0},
#endif
#ifdef _LEAKYBUCKET_
    {"NumberofLeakyBuckets",     &configinput.NumberLeakyBuckets,      0},
    {"LeakyBucketRateFile",     &configinput.LeakyBucketRateFile,     1},
    {"LeakyBucketParamFile",    &configinput.LeakyBucketParamFile,    1},
#endif
    {NULL,                       NULL,                                -1}
};

#endif

#ifndef INCLUDED_BY_CONFIGFILE_C
extern Mapping Map[];
#endif


void Configure (int ac, char *av[]);

#endif
