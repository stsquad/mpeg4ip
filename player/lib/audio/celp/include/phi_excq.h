/*====================================================================*/
/*         MPEG-4 Audio (ISO/IEC 14496-3) Copyright Header            */
/*====================================================================*/
/*
This software module was originally developed by Rakesh Taori and Andy
Gerrits (Philips Research Laboratories, Eindhoven, The Netherlands) in
the course of development of the MPEG-4 Audio (ISO/IEC 14496-3). This
software module is an implementation of a part of one or more MPEG-4
Audio (ISO/IEC 14496-3) tools as specified by the MPEG-4 Audio
(ISO/IEC 14496-3). ISO/IEC gives users of the MPEG-4 Audio (ISO/IEC
14496-3) free license to this software module or modifications thereof
for use in hardware or software products claiming conformance to the
MPEG-4 Audio (ISO/IEC 14496-3). Those intending to use this software
module in hardware or software products are advised that its use may
infringe existing patents. The original developer of this software
module and his/her company, the subsequent editors and their
companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not
released for non MPEG-4 Audio (ISO/IEC 14496-3) conforming products.
CN1 retains full right to use the code for his/her own purpose, assign
or donate the code to a third party and to inhibit third parties from
using the code for non MPEG-4 Audio (ISO/IEC 14496-3) conforming
products.  This copyright notice must be included in all copies or
derivative works. Copyright 1996.
*/
/*====================================================================*/
/*======================================================================*/
/*                                                                      */
/*      INCLUDE_FILE:   PHI_EXCQ.H                                      */
/*      PACKAGE:        WDBxx                                           */
/*      COMPONENT:      Tables for Gain Quantization                    */
/*                                                                      */
/*======================================================================*/

#ifndef _phi_excq_h_
#define _phi_excq_h_

/*======================================================================*/
/*     L O C A L    S Y M B O L     D E C L A R A T I O N               */
/*======================================================================*/
#define QLf       31     /* # quantizer levels of fixed codebook gain   */ 
#define QLa       32     /* # quantizer levels of adaptive codebook gain*/
#define QLfd       8     /* # quantizer levels of fixed codebook gain   */

#define FIXED_POINT_BITS      16L
#define SCALING_BITS_cba_dir1 (FIXED_POINT_BITS + 1L) /* scaling bits for cba gain fact.  */
#define SCALING_BITS_cba_dir2 (FIXED_POINT_BITS - 1L) /* scaling bits for cba gain fact.  */
#define SCALING_BITS_cba_dir3 (FIXED_POINT_BITS - 8L) /* scaling bits for cba gain fact.  */
#define SCALING_BITS_cbf_dir1 (FIXED_POINT_BITS - 4L) /* scaling bits for cbf gain fact.  */
#define SCALING_BITS_cbf_dir2 (FIXED_POINT_BITS - 7L) /* scaling bits for cbf gain fact.  */
#define SCALING_BITS_cbf_dir3 (FIXED_POINT_BITS -11L) /* scaling bits for cbf gain fact.  */
#define SCALING_BITS_cbf_dif  (FIXED_POINT_BITS - 3L) /* scaling bits for cbf diff-gain f */
#define SCALE_cba_dir1 (1L << SCALING_BITS_cba_dir1) /* scal. of cba gain      */
#define SCALE_cba_dir2 (1L << SCALING_BITS_cba_dir2) /* scal. of cba gain      */
#define SCALE_cba_dir3 (1L << SCALING_BITS_cba_dir3) /* scal. of cba gain      */
#define SCALE_cbf_dir1 (1L << SCALING_BITS_cbf_dir1) /* scal. of cbf gain      */
#define SCALE_cbf_dir2 (1L << SCALING_BITS_cbf_dir2) /* scal. of cbf gain      */
#define SCALE_cbf_dir3 (1L << SCALING_BITS_cbf_dir3) /* scal. of cbf gain      */
#define SCALE_cbf_dif  (1L << SCALING_BITS_cbf_dif)  /* scal. of cbf diff-gain */

/*======================================================================*/
/*     L O C A L    T Y P E     D E C L A R A T I O N                   */
/*======================================================================*/
struct  table_cb                         /* codebook gain quant. table  */
{
   double dec;
   double rep;
};

#ifdef __cplusplus
extern "C" {
#endif
/*======================================================================*/
/*     TABLE FOR ADAPTIVE CODEBOOK GAIN FACTOR QUANTIZATION             */
/*======================================================================*/
struct table_cb tbl_cba_dir[32] =
{
   { 0.1622,     (long) (0.0945 * SCALE_cba_dir1 + 0.5) / (double) SCALE_cba_dir1},
   { 0.2542,     (long) (0.2121 * SCALE_cba_dir1 + 0.5) / (double) SCALE_cba_dir1},
   { 0.3285,     (long) (0.2936 * SCALE_cba_dir1 + 0.5) / (double) SCALE_cba_dir1},
   { 0.3900,     (long) (0.3597 * SCALE_cba_dir1 + 0.5) / (double) SCALE_cba_dir1},
   { 0.4457,     (long) (0.4183 * SCALE_cba_dir1 + 0.5) / (double) SCALE_cba_dir1},
   { 0.4952,     (long) (0.4705 * SCALE_cba_dir1 + 0.5) / (double) SCALE_cba_dir1},

   { 0.5425,     (long) (0.5186 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},
   { 0.5887,     (long) (0.5652 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},
   { 0.6341,     (long) (0.6110 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},
   { 0.6783,     (long) (0.6559 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},
   { 0.7227,     (long) (0.7000 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},
   { 0.7664,     (long) (0.7445 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},
   { 0.8104,     (long) (0.7884 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},
   { 0.8556,     (long) (0.8329 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},
   { 0.9005,     (long) (0.8779 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},
   { 0.9487,     (long) (0.9242 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},
   { 0.9989,     (long) (0.9728 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},
   { 1.0539,     (long) (1.0256 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},
   { 1.1183,     (long) (1.0849 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},
   { 1.1933,     (long) (1.1538 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},
   { 1.2877,     (long) (1.2394 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},
   { 1.4136,     (long) (1.3465 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},
   { 1.5842,     (long) (1.4907 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},
   { 1.8559,     (long) (1.7012 * SCALE_cba_dir2 + 0.5) / (double) SCALE_cba_dir2},

   { 2.3603,     (long) (2.0636 * SCALE_cba_dir3 + 0.5) / (double) SCALE_cba_dir3},
   { 3.8348,     (long) (2.8398 * SCALE_cba_dir3 + 0.5) / (double) SCALE_cba_dir3},
   { 7.6697,     (long) (5.4233 * SCALE_cba_dir3 + 0.5) / (double) SCALE_cba_dir3},
   { 15.339,     (long) (10.847 * SCALE_cba_dir3 + 0.5) / (double) SCALE_cba_dir3},
   { 30.679,     (long) (21.693 * SCALE_cba_dir3 + 0.5) / (double) SCALE_cba_dir3},
   { 61.357,     (long) (43.386 * SCALE_cba_dir3 + 0.5) / (double) SCALE_cba_dir3},
   { 122.71,     (long) (86.772 * SCALE_cba_dir3 + 0.5) / (double) SCALE_cba_dir3},
   { 245.43,     (long) (173.54 * SCALE_cba_dir3 + 0.5) / (double) SCALE_cba_dir3}
};

/*======================================================================*/
/*     TABLE FOR FIXED CODEBOOK GAIN FACTOR QUANTIZATION                */
/*======================================================================*/
struct table_cb tbl_cbf_dir[31] =
{
  {   2.4726,     (long) (  2.0    * SCALE_cbf_dir1 + 0.5) / (double) SCALE_cbf_dir1},
  {   3.1895,     (long) (  2.8267 * SCALE_cbf_dir1 + 0.5) / (double) SCALE_cbf_dir1},
  {   4.2182,     (long) (  3.6320 * SCALE_cbf_dir1 + 0.5) / (double) SCALE_cbf_dir1},
  {   5.6228,     (long) (  4.8605 * SCALE_cbf_dir1 + 0.5) / (double) SCALE_cbf_dir1},
  {   7.3781,     (long) (  6.4876 * SCALE_cbf_dir1 + 0.5) / (double) SCALE_cbf_dir1},
  {   9.5300,     (long) (  8.3800 * SCALE_cbf_dir1 + 0.5) / (double) SCALE_cbf_dir1},
  {  12.1013,     (long) ( 10.7813 * SCALE_cbf_dir1 + 0.5) / (double) SCALE_cbf_dir1},
  {  15.2262,     (long) ( 13.5654 * SCALE_cbf_dir1 + 0.5) / (double) SCALE_cbf_dir1},
  
  {  19.0319,     (long) ( 17.0676 * SCALE_cbf_dir2 + 0.5) / (double) SCALE_cbf_dir2},
  {  23.6342,     (long) ( 21.1388 * SCALE_cbf_dir2 + 0.5) / (double) SCALE_cbf_dir2},
  {  29.1562,     (long) ( 26.2852 * SCALE_cbf_dir2 + 0.5) / (double) SCALE_cbf_dir2},
  {  35.3606,     (long) ( 32.1299 * SCALE_cbf_dir2 + 0.5) / (double) SCALE_cbf_dir2},
  {  42.8301,     (long) ( 39.0193 * SCALE_cbf_dir2 + 0.5) / (double) SCALE_cbf_dir2},
  {  51.1987,     (long) ( 46.9503 * SCALE_cbf_dir2 + 0.5) / (double) SCALE_cbf_dir2},
  {  60.6440,     (long) ( 55.9060 * SCALE_cbf_dir2 + 0.5) / (double) SCALE_cbf_dir2},
  {  70.9884,     (long) ( 65.7851 * SCALE_cbf_dir2 + 0.5) / (double) SCALE_cbf_dir2},
  {  82.3374,     (long) ( 76.4025 * SCALE_cbf_dir2 + 0.5) / (double) SCALE_cbf_dir2},
  {  95.3755,     (long) ( 88.6170 * SCALE_cbf_dir2 + 0.5) / (double) SCALE_cbf_dir2},
  { 109.8997,     (long) (102.7842 * SCALE_cbf_dir2 + 0.5) / (double) SCALE_cbf_dir2},
  { 126.3037,     (long) (117.6620 * SCALE_cbf_dir2 + 0.5) / (double) SCALE_cbf_dir2},
  
  { 144.3995,     (long) (135.2277 * SCALE_cbf_dir3 + 0.5) / (double) SCALE_cbf_dir3},
  { 165.5142,     (long) (154.7980 * SCALE_cbf_dir3 + 0.5) / (double) SCALE_cbf_dir3},
  { 190.9742,     (long) (178.1373 * SCALE_cbf_dir3 + 0.5) / (double) SCALE_cbf_dir3},
  { 220.6299,     (long) (204.7267 * SCALE_cbf_dir3 + 0.5) / (double) SCALE_cbf_dir3},
  { 258.2699,     (long) (239.0199 * SCALE_cbf_dir3 + 0.5) / (double) SCALE_cbf_dir3},
  { 305.5086,     (long) (280.5263 * SCALE_cbf_dir3 + 0.5) / (double) SCALE_cbf_dir3},
  { 368.5894,     (long) (334.0282 * SCALE_cbf_dir3 + 0.5) / (double) SCALE_cbf_dir3},
  { 453.5156,     (long) (408.3218 * SCALE_cbf_dir3 + 0.5) / (double) SCALE_cbf_dir3},
  { 573.6164,     (long) (505.7104 * SCALE_cbf_dir3 + 0.5) / (double) SCALE_cbf_dir3},
  { 801.6422,     (long) (665.3362 * SCALE_cbf_dir3 + 0.5) / (double) SCALE_cbf_dir3},
  {   9999.9,     (long) (1026.0   * SCALE_cbf_dir3 + 0.5) / (double) SCALE_cbf_dir3}
};

/*======================================================================*/
/*     TABLE FOR FIXED CODEBOOK DIFFERETIAL-GAIN FACTOR QUANTIZATION    */
/*======================================================================*/
struct table_cb tbl_cbf_dif[8] =
{
 {  0.2500,     (long) (0.1000 * SCALE_cbf_dif + 0.5) / (double) SCALE_cbf_dif},
 {  0.5378,     (long) (0.3912 * SCALE_cbf_dif + 0.5) / (double) SCALE_cbf_dif},
 {  0.7795,     (long) (0.6614 * SCALE_cbf_dif + 0.5) / (double) SCALE_cbf_dif},
 {  1.0230,     (long) (0.8954 * SCALE_cbf_dif + 0.5) / (double) SCALE_cbf_dif},
 {  1.3356,     (long) (1.1651 * SCALE_cbf_dif + 0.5) / (double) SCALE_cbf_dif},
 {  1.8869,     (long) (1.5580 * SCALE_cbf_dif + 0.5) / (double) SCALE_cbf_dif},
 {  4.2000,     (long) (2.7000 * SCALE_cbf_dif + 0.5) / (double) SCALE_cbf_dif},
 {  9999.9,     (long) (6.5000 * SCALE_cbf_dif + 0.5) / (double) SCALE_cbf_dif}
};

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _phi_excq_h_ */

/*======================================================================*/
/*      H I S T O R Y                                                   */
/*======================================================================*/
/* 17-04-96 R. Taori  Initial Version                                   */
