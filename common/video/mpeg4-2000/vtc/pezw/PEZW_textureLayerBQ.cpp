/****************************************************************************/
/*   MPEG4 Visual Texture Coding (VTC) Mode Software                        */
/*                                                                          */
/*   This software was jointly developed by the following participants:     */
/*                                                                          */
/*   Single-quant,  multi-quant and flow control                            */
/*   are provided by  Sarnoff Corporation                                   */
/*     Iraj Sodagar   (iraj@sarnoff.com)                                    */
/*     Hung-Ju Lee    (hjlee@sarnoff.com)                                   */
/*     Paul Hatrack   (hatrack@sarnoff.com)                                 */
/*     Shipeng Li     (shipeng@sarnoff.com)                                 */
/*     Bing-Bing Chai (bchai@sarnoff.com)                                   */
/*     B.S. Srinivas  (bsrinivas@sarnoff.com)                               */
/*                                                                          */
/*   Bi-level is provided by Texas Instruments                              */
/*     Jie Liang      (liang@ti.com)                                        */
/*                                                                          */
/*   Shape Coding is provided by  OKI Electric Industry Co., Ltd.           */
/*     Zhixiong Wu    (sgo@hlabs.oki.co.jp)                                 */
/*     Yoshihiro Ueda (yueda@hlabs.oki.co.jp)                               */
/*     Toshifumi Kanamaru (kanamaru@hlabs.oki.co.jp)                        */
/*                                                                          */
/*   OKI, Sharp, Sarnoff, TI and Microsoft contributed to bitstream         */
/*   exchange and bug fixing.                                               */
/*                                                                          */
/*                                                                          */
/* In the course of development of the MPEG-4 standard, this software       */
/* module is an implementation of a part of one or more MPEG-4 tools as     */
/* specified by the MPEG-4 standard.                                        */
/*                                                                          */
/* The copyright of this software belongs to ISO/IEC. ISO/IEC gives use     */
/* of the MPEG-4 standard free license to use this  software module or      */
/* modifications thereof for hardware or software products claiming         */
/* conformance to the MPEG-4 standard.                                      */
/*                                                                          */
/* Those intending to use this software module in hardware or software      */
/* products are advised that use may infringe existing  patents. The        */
/* original developers of this software module and their companies, the     */
/* subsequent editors and their companies, and ISO/IEC have no liability    */
/* and ISO/IEC have no liability for use of this software module or         */
/* modification thereof in an implementation.                               */
/*                                                                          */
/* Permission is granted to MPEG members to use, copy, modify,              */
/* and distribute the software modules ( or portions thereof )              */
/* for standardization activity within ISO/IEC JTC1/SC29/WG11.              */
/*                                                                          */
/* Copyright 1995, 1996, 1997, 1998 ISO/IEC                                 */
/****************************************************************************/

/****************************************************************************/
/*     Texas Instruments Predictive Embedded Zerotree (PEZW) Image Codec    */
/*	   Developed by Jie Liang (liang@ti.com)                                */
/*													                        */ 
/*     Copyright 1996, 1997, 1998 Texas Instruments	      		            */
/****************************************************************************/

/****************************************************************************
   File name:         PEZW_textureLayerBQ.c
   Author:            Jie Liang  (liang@ti.com)
   Functions:         main control module for PEZW coding. Functions in this
                      file are the only interface with other parts of MPEG4 
                      code along with the global variables in wvtPEZW.h.
   Revisions:         v1.0 (10/04/98)
*****************************************************************************/

#include "wvtPEZW.hpp"
#include "PEZW_mpeg4.hpp"
#include "PEZW_zerotree.hpp"
#include "wvtpezw_tree_codec.hpp"
#include <PEZW_functions.hpp>

#include "dataStruct.hpp"
#include "globals.hpp"
#include "msg.hpp"
#include "bitpack.hpp"
#include "startcode.hpp"

/* decoding parameters imported form main.c.
   they should be defined before calling PEZW decoder. */
int PEZW_target_spatial_levels=10;
int PEZW_target_snr_levels=20;
int PEZW_target_bitrate=0;

void CVTCEncoder::textureLayerBQ_Enc(FILE *bitfile)
{
  PEZW_SPATIAL_LAYER *SPlayer[3];
  int h,w;
  int Quant[3];
  int levels, col;
  static short **wvt_coeffs;
  int snrlev,splev;
  int dc_w, dc_h;
  int temp;
  int i,j,bplane;
   
  /*------- AC: encode all color components -------*/
  for (col=0; col<mzte_codec.m_iColors; col++)
    { 
      printf("Bilevel-Quant Mode - Color %d\n",col);
      
      if (col ==0)  /* Lum */
	    {
	        h = mzte_codec.m_iHeight;
	        w = mzte_codec.m_iWidth;
	        levels = mzte_codec.m_iWvtDecmpLev;
	    }
      else { /* should depend on mzte_codec.color_format;
		not implemented yet, only support 4:2:0  */
	    h = mzte_codec.m_iHeight/2;
	    w = mzte_codec.m_iWidth/2;
	    levels = mzte_codec.m_iWvtDecmpLev-1;
      }
      
      /* initialize data */
      SPlayer[col] = Init_PEZWdata (col,levels,w,h);
      wvt_coeffs = (WINT **)calloc(h,sizeof(WINT *));
      wvt_coeffs[0] = (WINT *)(SPlayer[col][0].SNRlayer[0].snr_image.data);
      for(i=1;i<h;i++)
        wvt_coeffs[i] = wvt_coeffs[0]+i*w;

      /* quantization */
      Quant[col] = mzte_codec.m_Qinfo[col]->Quant[0];
      dc_w = w>>levels;
      dc_h = h>>levels;
      for(i=0;i<h;i++)
          for(j=0;j<w;j++)
              {
                if((i<dc_h) && (j<dc_w))
                    continue;
                temp = abs(wvt_coeffs[i][j])/Quant[col];
                wvt_coeffs[i][j] = (wvt_coeffs[i][j]>0)?temp:-temp;
              }
      
      /* encode this color componenet */

      /* initialize the PEZW codec */
      PEZW_encode_init (levels, w, h);
      setbuffer_PEZW_encode ();

      /* encode the AC bands */
      PEZW_encode_block (wvt_coeffs,w,h);

      /* done coding */ 
      PEZW_encode_done ();

      /* copy bitstream to data structure */
      for (splev=0;splev<levels;splev++)   /* from coarse scale to fine scale */
          {
          SPlayer[col][splev].SNR_scalability_levels = Max_Bitplane;
          for(snrlev=SPlayer[col][splev].SNR_scalability_levels-1;snrlev>=0;snrlev--) /* from msb to lsb */
              {
                /* for SPlayer, snrlayer 0 means msb bits, need to invert order */
                bplane = SPlayer[col][splev].SNR_scalability_levels-snrlev-1;

                SPlayer[col][splev].SNRlayer[bplane].Quant = Quant[col];
                SPlayer[col][splev].SNRlayer[bplane].snr_bitstream.data = PEZW_bitstream[splev][snrlev];
                SPlayer[col][splev].SNRlayer[bplane].snr_bitstream.length = Init_Bufsize[splev][snrlev];
                SPlayer[col][splev].SNRlayer[bplane].bits_to_go = bits_to_go_inBuffer[splev][snrlev];
              }
          }

      /* free memory */
      //free (wvt_coeffs);      //deleted by SL 030499
      /* free memory */
      free (wvt_coeffs);      
      for (i=0;i<levels;i++)
          free(Init_Bufsize[i]);
      free(Init_Bufsize);
      for (i=0;i<levels;i++)
          free(PEZW_bitstream[i]);
      free(PEZW_bitstream);
      for (i=0;i<levels;i++)
          free(bits_to_go_inBuffer[i]);
      free(bits_to_go_inBuffer);
    }        
  
  /* package the bitstream and write to bitfile */
  PEZW_bitpack (SPlayer);
  flush_bits();
  flush_bytes();
  fclose(bitfile);

  /* free memory */
  PEZW_freeEnc (SPlayer);  
}


void CVTCDecoder::textureLayerBQ_Dec(FILE *bitfile)
{
   PEZW_SPATIAL_LAYER **SPlayer;
   int col,snrlev,splev;
   static short **wvt_coeffs;
   int levels = 0,h,w;
   int i,j,bplane;
   int Quant[3];
   int dc_w, dc_h, temp;
   int all_zero[3]={0,0,0};
   int all_non_zero[3]={1,1,1};
   int LH_zero[3]={0,0,0}, HL_zero[3]={0,0,0}, HH_zero[3]={0,0,0};
   int splev_start;
   unsigned char **splev0_bitstream = NULL;
   int *splev0_bitstream_len = NULL;
   long total_decoded_bytes=0, bytes_decoded=0;

   SPlayer = (PEZW_SPATIAL_LAYER **)calloc(mzte_codec.m_iColors, sizeof(void *))
;
   mzte_codec.m_iScanOrder = mzte_codec.m_iScanDirection;
   
   /* bytes already decoded for DC and header */
   bytes_decoded = decoded_bytes_from_bitstream ();

   /* parse the bitstreams into spatial and snr layers */
   PEZW_bit_unpack (SPlayer);

   /* rate control according to target_bit_rate */
   PEZW_decode_ratecontrol (SPlayer, bytes_decoded);

   /* decode each color component */
   for (col=0; col<mzte_codec.m_iColors; col++)
    { 
      printf("Bilevel-Quant Mode - Color %d\n",col);
      
      if (col ==0)  /* Lum */
	    {
	        h = mzte_codec.m_iHeight;
	        w = mzte_codec.m_iWidth;
	        levels = mzte_codec.m_iWvtDecmpLev;
	    }
      else { /* should depend on mzte_codec.color_format;
		not implemented yet, only support 4:2:0  */
	    h = mzte_codec.m_iHeight/2;
	    w = mzte_codec.m_iWidth/2;
	    levels = mzte_codec.m_iWvtDecmpLev-1;
      }
      
      /* decode the bitstream */
      /* initialize the PEZW codec */
      Max_Bitplane = SPlayer[col][0].SNR_scalability_levels;
      PEZW_decode_init (levels, w, h);

      if(col==0)    /* the bitplane level of Y is assumed to be larger than U,V */
          {
            PEZW_bitstream = (unsigned char ***) calloc(levels,sizeof(int **));
            for(i=0;i<levels;i++)
                PEZW_bitstream[i]= (unsigned char **)calloc(Max_Bitplane,sizeof(int *));

            splev0_bitstream = (unsigned char **)calloc(Max_Bitplane,sizeof(char *));
            splev0_bitstream_len = (int *)calloc(Max_Bitplane,sizeof(int));

            Init_Bufsize = (int **) calloc(levels,sizeof(int *));
            for(i=0;i<levels;i++)
                Init_Bufsize[i]= (int *)calloc(Max_Bitplane,sizeof(int));

            /* decoded bytes */
            decoded_bytes = (int **)calloc(tree_depth,sizeof(int *));
            for (i=0;i<tree_depth;i++)
 		        decoded_bytes[i]=(int *)calloc(Max_Bitplane,sizeof(int));

            /* bits_to_go structure */
            bits_to_go_inBuffer = (unsigned char **)calloc(tree_depth,sizeof(char *));
            for (i=0;i<tree_depth;i++)
 		        bits_to_go_inBuffer[i]=(unsigned char *)calloc(Max_Bitplane,sizeof(char));         
          }
       
     /* get the bitstreams for each color */
     /* for YUV420, the U,V color component has one less level */
     if(col==1)
         splev_start=1;
     else
         splev_start=0;

     for (splev=0;splev<levels-spatial_leveloff;splev++)   /* from coarse scale to fine scale */
          {
          for(snrlev=SPlayer[0][splev].SNR_scalability_levels-1;snrlev>=Min_Bitplane;snrlev--) /* from msb to lsb */
              {
                /* for SPlayer, snrlayer 0 means msb bits, need to invert order */
                bplane = SPlayer[col][splev].SNR_scalability_levels-snrlev-1;

                if(col==0)
                    {
                        Init_Bufsize[splev][snrlev] = SPlayer[0][splev].SNRlayer[bplane].snr_bitstream.length;  
                        PEZW_bitstream[splev][snrlev] = (unsigned char *) SPlayer[0][splev].SNRlayer[bplane].snr_bitstream.data;
                        if(splev==0){
                            splev0_bitstream[snrlev] = PEZW_bitstream[0][snrlev];
                            splev0_bitstream_len[snrlev] = Init_Bufsize[0][snrlev];
                        }
                    }
                else{
                        Init_Bufsize[splev][snrlev] = Init_Bufsize[splev+splev_start][snrlev];
                        PEZW_bitstream[splev][snrlev] = PEZW_bitstream[splev+splev_start][snrlev];
                        reach_budget[splev][snrlev] = reach_budget[splev+splev_start][snrlev];
                    }  
                
                /* get all zero and subband skipping info. */
                if(splev==0){
                    all_non_zero[col] = lshift_by_NBit (splev0_bitstream[snrlev], splev0_bitstream_len[snrlev], 1);
                    if(!all_non_zero[col]){
                        all_zero[col] = lshift_by_NBit (splev0_bitstream[snrlev], splev0_bitstream_len[snrlev], 1);
                        if(!all_zero[col]){
                            LH_zero[col] = lshift_by_NBit (splev0_bitstream[snrlev], splev0_bitstream_len[snrlev], 1);
                            HL_zero[col] = lshift_by_NBit (splev0_bitstream[snrlev], splev0_bitstream_len[snrlev], 1);
                            HH_zero[col] = lshift_by_NBit (splev0_bitstream[snrlev], splev0_bitstream_len[snrlev], 1);
                        }
                     }
                    else
                        all_zero[col]=0;
                 }

                if(all_zero[col] && (splev==0))
                    Max_Bitplane--;
                if(all_zero[col]){
                    decoded_bytes[splev][snrlev] = 0;
                    continue;
                    }
                else    /* skip the leading '1' */
                    lshift_by_NBit (PEZW_bitstream[splev][snrlev], Init_Bufsize[splev][snrlev], 1);                                     

              }  /* end of snrlev */
         }  /* end of splev */

      /* allocate wvt_coeffs */
      wvt_coeffs = (WINT **)calloc(h,sizeof(WINT *));
      wvt_coeffs[0] = (WINT *)(SPlayer[col][0].SNRlayer[0].snr_image.data);
      for(i=1;i<h;i++)
        wvt_coeffs[i] = wvt_coeffs[0]+i*w;

      if(Max_Bitplane>0){
         /* this step must follow the previous step */
         setbuffer_PEZW_decode ();

        /* decode the bitstream for this color component */
        PEZW_decode_block (wvt_coeffs,w,h);

        /* finish PEZW coding for this color component
            retrun the number of bytes decoded in Init_Bufsize */
        PEZW_decode_done ( );

        for(splev=0;splev<levels-spatial_leveloff;splev++)
            for(snrlev=Min_Bitplane;snrlev<Max_Bitplane;snrlev++)
                {
                total_decoded_bytes += decoded_bytes[splev][snrlev];

                if(col<2){
        		            /* adjust for residue bits in last byte */
                        unsigned char Bit;
                        //unsigned char *data;

                        PEZW_bitstream[splev][snrlev] += decoded_bytes[splev][snrlev];
                        Init_Bufsize[splev][snrlev] -= decoded_bytes[splev][snrlev];
                        
                        Bit = (*(PEZW_bitstream[splev][snrlev]-1)>>bits_to_go_inBuffer[splev][snrlev])&0x01;
		                if(bits_to_go_inBuffer[splev][snrlev]>0){
		                    PEZW_bitstream[splev][snrlev]--;
		                    Init_Bufsize[splev][snrlev]++;
		                    lshift_by_NBit(PEZW_bitstream[splev][snrlev],Init_Bufsize[splev][snrlev],
                                8-bits_to_go_inBuffer[splev][snrlev]);
		                }
		                /* check last bit */
		                if(!Bit)
		                    lshift_by_NBit(PEZW_bitstream[splev][snrlev],Init_Bufsize[splev][snrlev],1);

                        if((splev==0)&&(col==0)){
                            splev0_bitstream[snrlev] = PEZW_bitstream[0][snrlev];
                            splev0_bitstream_len[snrlev] = Init_Bufsize[0][snrlev];
                        }
                }
              }
      }

     /* dequantize */
     Quant[col] = SPlayer[col][0].SNRlayer[0].Quant;
     dc_w = w>>levels;
     dc_h = h>>levels;
     for(i=0;i<h;i++)
          for(j=0;j<w;j++)
              {
                if((i<dc_h) && (j<dc_w))
                    continue;

                if(wvt_coeffs[i][j]==0)
                    temp = 0;
                else
                    temp = (int)((abs(wvt_coeffs[i][j])+0.5)*Quant[col]);
                wvt_coeffs[i][j] =  (wvt_coeffs[i][j]>0)?temp:-temp;
             }

    /* copy data to structure */
    (SPlayer[col][0].SNRlayer[0].snr_image.data) = (WINT *) wvt_coeffs[0];


     /* free memory */
     free(wvt_coeffs);
    }
   
    printf("total actually decoded bits:          %ld\n", (total_decoded_bytes+bytes_decoded)*8);
    printf("total actually decoded AC band bits:  %ld", total_decoded_bytes*8);

    /* put the decoded wavelet coefficients back into the data structure */
    restore_PEZWdata (SPlayer);
   
    /* free memory */
    PEZW_freeDec (SPlayer);
    free(SPlayer);
    for(i=0;i<levels;i++)
         free(PEZW_bitstream[i]);
    free(PEZW_bitstream);
    for (i=0;i<levels;i++)
         free(Init_Bufsize[i]);
    free(Init_Bufsize);
    for (i=0;i<levels;i++)
       free(reach_budget[i]);
    free(reach_budget);
    for (i=0;i<levels+1;i++)
       free(bits_to_go_inBuffer[i]);
    free(bits_to_go_inBuffer);
    for (i=0;i<levels+1;i++)
       free(decoded_bytes[i]);
    free(decoded_bytes);
    free(splev0_bitstream);
    free(splev0_bitstream_len); 
}

/* rate control at decoding */
void CVTCDecoder::PEZW_decode_ratecontrol (PEZW_SPATIAL_LAYER **SPlayer, int bytes_decoded)
{
    int i, levels;
    long decoded_bits=bytes_decoded;
    int splayer, snrlev, bplane;
    int MaxBitplanes;
    int last_bplane = 0, last_splayer = 0, adjusted_rate=0;
    int diffrate;
    int min_snrlev;

    levels = mzte_codec.m_iWvtDecmpLev;

    /* allocate rate control structure */
    reach_budget = (unsigned char **) calloc(levels,sizeof(char *));
    for(i=0;i<levels;i++)
        reach_budget[i]= (unsigned char *)calloc(Max_Bitplane,sizeof(char));

	/* target bitplane */
	Min_Bitplane  = SPlayer[0][0].SNR_scalability_levels-PEZW_target_snr_levels;
	if ( Min_Bitplane < 0)
	    Min_Bitplane  = 0;

	/* target spatial level */
	spatial_leveloff = mzte_codec.m_iWvtDecmpLev-PEZW_target_spatial_levels;
	if (spatial_leveloff <0)
        spatial_leveloff=0;

    /* figure out the target snr and spatial layer
       and the length for the last layer for target_bitrate options */
    MaxBitplanes = SPlayer[0][0].SNR_scalability_levels;
    if ((decoded_bits < PEZW_target_bitrate/8) && (PEZW_target_bitrate >0)){
        if(mzte_codec.m_iScanOrder==1)
            {
                for (splayer=0;splayer<mzte_codec.m_iWvtDecmpLev-spatial_leveloff ;splayer++)
                    for (snrlev=MaxBitplanes-1;snrlev>=Min_Bitplane;snrlev--)              
                            {
                                /* for SPlayer, snrlayer 0 means msb bits, need to invert order */
                                bplane = SPlayer[0][splayer].SNR_scalability_levels
                                            -snrlev-1;

                                decoded_bits += SPlayer[0][splayer].SNRlayer[bplane].snr_bitstream.length;
                                if ( decoded_bits >= mzte_codec.m_iTargetBitrate/8){
                                       adjusted_rate = 1;
                                       last_bplane = bplane;
                                       last_splayer = splayer;
                                       goto outside;
                                     }
                            }
            }
        else if (mzte_codec.m_iScanOrder==0)
            {
               for (snrlev=MaxBitplanes-1;snrlev>=Min_Bitplane;snrlev--)              
                 for (splayer=0;splayer<mzte_codec.m_iWvtDecmpLev-spatial_leveloff ;splayer++)
                     {
                       /* for SPlayer, snrlayer 0 means msb bits, need to invert order */
                       bplane = SPlayer[0][splayer].SNR_scalability_levels
                                  -snrlev-1;

                       decoded_bits += SPlayer[0][splayer].SNRlayer[bplane].snr_bitstream.length;
                       if ( decoded_bits >= PEZW_target_bitrate/8){
                            adjusted_rate = 1;
                            last_bplane = bplane;
                            last_splayer = splayer;
                            goto outside;
                           }
                     }
            }
     }

outside:
     if(adjusted_rate){
        diffrate = decoded_bits - PEZW_target_bitrate/8;
        SPlayer[0][last_splayer].SNRlayer[last_bplane].snr_bitstream.length
            -= diffrate;

        /* set up target snr and spatial levels, as well as
           the rate control matrix */

        if (mzte_codec.m_iScanOrder==0){
            min_snrlev = MaxBitplanes-1-last_bplane;
            for (splayer=last_splayer;splayer<mzte_codec.m_iWvtDecmpLev-spatial_leveloff;splayer++)                
                        reach_budget[splayer][min_snrlev] = 1;
            reach_budget[last_splayer][min_snrlev]=0;
            Min_Bitplane = min_snrlev;
        }
        else if (mzte_codec.m_iScanOrder==1){
            min_snrlev = MaxBitplanes-1-last_bplane;
            for (bplane=min_snrlev;bplane>=Min_Bitplane;bplane--)               
                     reach_budget[last_splayer][bplane] = 1;
            reach_budget[last_splayer][min_snrlev]=0;
            spatial_leveloff = mzte_codec.m_iWvtDecmpLev-1-last_splayer;
        }

        decoded_bits = PEZW_target_bitrate;
     }

    return;
}

