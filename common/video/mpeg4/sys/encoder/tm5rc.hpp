/* header file for TM-5 rate control */

/* MPEG-2 Video TM-5 rate control 
   For the purpose of comparing
   MPEG-4 VM with MPEG-2 TM-5
   27.03.97 By X. Chen in G.I.
 * 16.03.99 David Ruhoff: conversion to microsoft-vfdis-v10-990124
 */

#ifndef __TM5RC_H_
#define __TM5RC_H_

#define RC_MPEG4 1
#define RC_TM5   3

#include "basic.hpp"

Class CVOPU8YUVBA;
Class CVideoObjectEncoder;

Class TM5rc
{
public:

     TM5rc () 
     {
         Xi = Xp = Xb = r_tm5 = d0i = d0p = d0b = 0;
         avg_act = 0.0;
         mbact = NULL;
         R_tm5 = T_tm5 = d_tm5 = 0;
         actsum = 0.0;
         Ni = Np = Nb = S_tm5 = Q_tm5 = prev_mquant = 0;
         mb_width = mb_height = mquant = bitrate = 0;
         pic_rate = 0.0;
         rc_type = 0;
         Qfile = NULL;
         linectr = 0;
     };
    ~TM5rc () {};

    void    tm5rc_init_seq( char       *pchQname,
                            UInt       rc_type, 
                            AlphaUsage fAUsage, 
                            UInt       uiWidth, 
                            UInt       uiHeight, 
                            UInt       uiBitRate,
                            Double     dFrameRate
                           );
    void    tm5rc_init_GOP(Int np, Int nb);
    void    tm5rc_init_pict(VOPpredType vopPredType,
                            const PixelC* ppxlcOrigY,
                            Int row_size,
                            Int iNumMBX,
                            Int iNumMBY);
    Int     tm5rc_start_mb();
    Int		tm5rc_calc_mquant(Int iMBindex, UInt uiBitsUsed);
    void	tm5rc_update_pict(VOPpredType vopPredType, Int iVOPtotalBits);


private:

    void    tm5_calc_actj(const PixelC* ppxlcOrigY,
                          Int row_size,
                          Int iNumMBX,
                          Int iNumMBY);
    Double  tm5_var_sblk(const PixelC *p, Int lx);

    Int     Xi;         /* I-picture complexity */
    Int     Xp;         /* P-picture complexity */
    Int     Xb;         /* B-picture complexity */
    Int     r_tm5;      /* Reaction parameter */
    Int     d0i;        /* I-pic virtual buffer occupancy */
    Int     d0p;        /* P-pic virtual buffer occupancy */
    Int     d0b;        /* B-pic virtual buffer occupancy */
    Double  avg_act;    /* Ave activity (of last picture) */
    Double  *mbact;     /* Activity of each macroblock */
    Int     R_tm5;      
    Int     T_tm5;
    Int     d_tm5;      /* buffer occupancy accumulator */
    Double  actsum;     /* Cumulative sum of activities */
    Int     Ni;         /* Nbr I-pics reamining in GOP */
    Int     Np;         /* Nbr P-pics remaining in GOP */
    Int     Nb;         /* Nbr B-pics remaining in GOP */
    Int     S_tm5;
    Int     Q_tm5;      /* Sum of quantizer value (for activity calc) */
    Int     prev_mquant;/* Previous quantizer */
    Int     mb_width;   /* Nbr MBs in horiz direction */
    Int     mb_height;  /* Nbr MBs in vert direction */
    Int     mquant;     /* running quantizer value */
    Int     bitrate;    /* Bit rate (bits/second) */
    Double  pic_rate;   /* Pictures per second */

    Int     rc_type;    /* Rate control type code */
    FILE    *Qfile;     /* Quantizer file */
    Int     linectr;    /* numbers on current Qfile line */
};


#endif
