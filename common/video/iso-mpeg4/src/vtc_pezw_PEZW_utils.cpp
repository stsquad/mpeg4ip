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
   File name:         PEZW_utils.c
   Author:            Jie Liang  (liang@ti.com)
   Functions:         utility functions for PEZW coder such as bitstream 
                      parsing and formating.
   Revisions:         v1.0 (10/04/98)
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

#include "globals.hpp"
#include "msg.hpp"
#include "bitpack.hpp"
#include "startcode.hpp"

#include "wvtPEZW.hpp"
#include "PEZW_zerotree.hpp"
#include "PEZW_mpeg4.hpp"

extern int PEZW_target_spatial_levels;
#define    MAXSNRLAYERS  20

/* this funciton is to create and initiate the data structure
   used for bilevel image coding */

PEZW_SPATIAL_LAYER * CVTCCommon::Init_PEZWdata (int color, int levels, int w, int h)
{
  PEZW_SPATIAL_LAYER *SPlayer;
  int x,y;
  int i;

  /* generate the Spatial Layer Structure */
  SPlayer = (PEZW_SPATIAL_LAYER *)calloc (levels, sizeof(PEZW_SPATIAL_LAYER));
  for (i=0; i<levels; i++)
     SPlayer[i].SNRlayer = (PEZW_SNR_LAYER *)calloc(MAXSNRLAYERS, sizeof(PEZW_SNR_LAYER));
     
  mzte_codec.m_iScanOrder = mzte_codec.m_iScanDirection;

  /* the original wavelet coefficients are in 
     SPlayer[0].SNRlayer[0].snr_image   */
  SPlayer[0].SNRlayer[0].snr_image.height = h;
  SPlayer[0].SNRlayer[0].snr_image.width = w;
  if ((SPlayer[0].SNRlayer[0].snr_image.data = calloc(h*w, sizeof(WINT)))
      == NULL){
    printf ("Can not allocate memory in Init_PEZWdata()");
    exit(-1);
  }

  /* copy the wavelet coefficients into SPlayer structure */
  for (y=0;y<h;y++)
    for (x=0;x<w;x++)
      ((WINT *)(SPlayer[0].SNRlayer[0].snr_image.data))[y*w+x] = (WINT) 
	  (mzte_codec.m_SPlayer[color].coeffinfo[y][x].wvt_coeff);

  return (SPlayer);  

}

/* put the restored wavelet coefficients back to the data structure */
void CVTCCommon::restore_PEZWdata (PEZW_SPATIAL_LAYER **SPlayer)
{
  int x,y;
  int col;
  int h,w;
  int dch, dcw;
  int levels;

  for(col=0;col<mzte_codec.m_iColors;col++){
    h = SPlayer[col][0].SNRlayer[0].snr_image.height;
    w = SPlayer[col][0].SNRlayer[0].snr_image.width;    
    
    if(col==0)
      levels=mzte_codec.m_iWvtDecmpLev;
    else
      levels=mzte_codec.m_iWvtDecmpLev-1;

    dch = h/(1<<levels);
    dcw = w/(1<<levels);

    /* copy the wavelet coefficients into data structure */
    for (y=0;y<h;y++)
      for (x=0;x<w;x++){
	if((x>=dcw) || (y>=dch))
	  mzte_codec.m_SPlayer[col].coeffinfo[y][x].rec_coeff = 	
	    ((WINT *)(SPlayer[col][0].SNRlayer[0].snr_image.data))[y*w+x]; 
      }
  }

  return;

}

/* this function pack the bitstream and write to file */
void CVTCEncoder::PEZW_bitpack (PEZW_SPATIAL_LAYER **SPlayer)
{
  int levels = mzte_codec.m_iWvtDecmpLev;
  int levels_UV;
  int Quant[NCOLOR], q;
  int texture_spatial_layer_id;

  int lev;
  int col;
  int snr_scalability_levels=0;
  int snr_lev;

  int all_zero=0, all_non_zero=0;
  int LH_zero=0, HL_zero=0, HH_zero=0;
  int i,n;

  unsigned char *data;
  long len;

  int spalev;
  int snroffset;
  int flag;
  int Bit, bits_to_go;

  levels_UV = levels-1;  /* this should depend on the color format 
			                mzte_codec.color_format */

  /* get quantization information */
  for (col=0;col<mzte_codec.m_iColors; col++)
    Quant[col] = mzte_codec.m_Qinfo[col]->Quant[0];
  
  /* output quant for each color component */
  emit_bits_checksc_init();
  for (col=0;col<mzte_codec.m_iColors; col++){
      q = Quant[col];
      flag=0;
      for(i=3;i>=0;i--)
	{
	  q = Quant[col] & (0x7F << (i*7));
	  q >>= 7*i;
	  if (q>0) flag = 1;
	  if (flag){
	    if (i>0) 
	      emit_bits_checksc(128+q,8);
	    else
	      emit_bits_checksc(q,8);
	  }
	}
  }

  /* figure out snr scalability levels */
  snr_scalability_levels = 0;
  for(lev=0;lev<mzte_codec.m_iWvtDecmpLev;lev++){
      if(snr_scalability_levels < SPlayer[0][lev].SNR_scalability_levels)
        snr_scalability_levels = SPlayer[0][lev].SNR_scalability_levels;
      if (lev>0){
	        for (col=1;col<mzte_codec.m_iColors; col++){
	            if (snr_scalability_levels < SPlayer[col][lev-1].SNR_scalability_levels)
	            snr_scalability_levels = SPlayer[col][lev-1].SNR_scalability_levels;
	        }
      }
    }
  emit_bits_checksc (snr_scalability_levels,5);

  /* for bileve mode, start code is always enabled */
  if (!mzte_codec.m_bStartCodeEnable)
      {
        fprintf(stdout,"\nFor bilevel mode, SNR_start_code must be enabled!\n");
        exit(-1);
      }

  /* formatting the AC band bitstream */
  if(mzte_codec.m_iScanOrder==1){

    for (lev=0;lev<levels;lev++){   /* spatial scalability level */
      /*------- AC: Write header info to bitstream file -------*/
	 if (mzte_codec.m_bStartCodeEnable)	
		{
	        flush_bits();
            emit_bits(TEXTURE_SPATIAL_LAYER_START_CODE >> 16, 16);
            emit_bits(TEXTURE_SPATIAL_LAYER_START_CODE, 16);
            texture_spatial_layer_id = lev;
            emit_bits(texture_spatial_layer_id, 5);
		} 
     
    for (snr_lev=0;snr_lev<snr_scalability_levels;snr_lev++){
	    /* output the SNR_START_CODE if enabled */
	    if (mzte_codec.m_bStartCodeEnable)
	    {
	        flush_bits();
	        emit_bits(texture_snr_layer_start_code >> 16, 16);
	        emit_bits(texture_snr_layer_start_code, 16);
	        emit_bits(snr_lev,5);
		    emit_bits_checksc_init();
	    }
	
	    for (col=0;col<mzte_codec.m_iColors; col++){ /* for each color compoent */
	  
	        if(col>0)  
	            spalev = lev-1;
	        else	   
	            spalev = lev;
	  
	        if((lev==0)&&(col>0))
		        snroffset = snr_scalability_levels-SPlayer[col][lev].SNR_scalability_levels;
	        else
		        snroffset = snr_scalability_levels-SPlayer[col][spalev].SNR_scalability_levels;

	        /* output SNR_ALL_ZERO flag */
            if (snr_lev < snroffset){
                all_non_zero=0;
	            all_zero = 1;
            }
            else{
                all_non_zero=1;
	            all_zero = 0;
            }

            /* send all zero and subband skipping info */
            if(lev==0){
		        emit_bits_checksc (all_non_zero,1); 
                if(!all_non_zero){
                    emit_bits_checksc (all_zero,1); 
                    if(!all_zero){
                        emit_bits_checksc (LH_zero,1); 
                        emit_bits_checksc (HL_zero,1); 
                        emit_bits_checksc (HH_zero,1); 
                    }
                }
            }

	        if((lev==0)&&(col>0))
		      continue;
	  
	        /* send the bitstream for this snr layer if not all_zero */
	        if (!all_zero){
	            data = (unsigned char *)SPlayer[col][spalev].SNRlayer[snr_lev-snroffset].snr_bitstream.data;
	            len = SPlayer[col][spalev].SNRlayer[snr_lev-snroffset].snr_bitstream.length;
	            bits_to_go = SPlayer[col][spalev].SNRlayer[snr_lev-snroffset].bits_to_go;
	    
	            /* output the bitstream */

		        /* the first bit of each bitplane is always 1
                   to avoid start code emulation */
		        emit_bits_checksc(1,1);

	            for (n=0;n<len-1;n++){
	                emit_bits_checksc (data[n],8);
	                if (DEBUG_BS_DETAIL)
		                fprintf(stdout,"%d ", data[n]);
	            }

                /* output bits in the last byte.
                   if bits_to_go ==8, we output the whole byte.
                   otherwise, we output only the bits that needed to be transmited. */
		        if(bits_to_go==8)
			        emit_bits_checksc (data[len-1],8);
	            else
			        emit_bits_checksc (data[len-1]>>bits_to_go,8-bits_to_go);
		  
		        /* if last bit is 1, output 1, otherwise, skip. */
		        if(bits_to_go==8)
			        bits_to_go = 0;
		        Bit = (data[len-1]>>bits_to_go)&0x01;
		        if(!Bit)
			        emit_bits_checksc(1,1);

	            if(DEBUG_BS)
	                fprintf(stdout, "color %d spa_lev %d snr_lev %d: %ld\n",
		                col, spalev, snr_lev-snroffset, len);
	    
	        } /* end of !all_zero */
	      } /* end of color plane */      
      }	/* end of snr_scalability */
    }  /* end of spatial scalability */
  }
  else  /* scan_order == 0 */
  {    
    /* package the bitstream */
    for (snr_lev=0;snr_lev<snr_scalability_levels;snr_lev++)
      {
	    /* output the SNR_START_CODE if enabled */
	    if (mzte_codec.m_bStartCodeEnable)
	    {
	        flush_bits();
	        emit_bits(texture_snr_layer_start_code >> 16, 16);
	        emit_bits(texture_snr_layer_start_code, 16);
	        emit_bits(snr_lev,5);
	    }	
	
	    /* output the spatial levels */
	    for (lev=0;lev<levels;lev++){   /* spatial scalability level */
		    if (mzte_codec.m_bStartCodeEnable)
	        {
	            flush_bits();
	            emit_bits(TEXTURE_SPATIAL_LAYER_START_CODE >> 16, 16);
	            emit_bits(TEXTURE_SPATIAL_LAYER_START_CODE, 16);
	            texture_spatial_layer_id = lev;
	            emit_bits(texture_spatial_layer_id, 5);
	      
	            emit_bits_checksc_init(); 
	        }
		
	        /* for each color compoent */
	        for (col=0;col<mzte_codec.m_iColors; col++){
	            if(col>0)  
	                spalev = lev-1;
	             else
	                spalev = lev;
	    
		        if((lev==0)&&(col>0))
		            snroffset = snr_scalability_levels-SPlayer[col][lev].SNR_scalability_levels;
		        else
		            snroffset = snr_scalability_levels-SPlayer[col][spalev].SNR_scalability_levels;
	    
	        /* output SNR_ALL_ZERO flag */
            if (snr_lev < snroffset){
                all_non_zero=0;
	            all_zero = 1;
            }
            else{
                all_non_zero=1;
	            all_zero = 0;
            }

            /* send all zero and subband skipping info */
            if(lev==0){
		         emit_bits_checksc (all_non_zero,1); 
                 if(!all_non_zero){
                    emit_bits_checksc (all_zero,1); 
                    if(!all_zero){
                        emit_bits_checksc (LH_zero,1); 
                        emit_bits_checksc (HL_zero,1); 
                        emit_bits_checksc (HH_zero,1); 
                    }
                }
            }

		    if((lev==0)&&(col>0))
			    continue;
	    
	        /* send the bitstream for this snr layer if not all_zero */
	        if (!all_zero){
	            data = (unsigned char *)SPlayer[col][spalev].SNRlayer[snr_lev-snroffset].snr_bitstream.data;
	            len = SPlayer[col][spalev].SNRlayer[snr_lev-snroffset].snr_bitstream.length;
	            bits_to_go = SPlayer[col][spalev].SNRlayer[snr_lev-snroffset].bits_to_go;

	            /* output the bitstream */

		        /* the first bit is 1 */
		            emit_bits_checksc(1,1);

		        /* output bitstream until second to last byte */
	            for (n=0;n<len-1;n++){
			        emit_bits_checksc (data[n],8);
			        if (DEBUG_BS_DETAIL)
			        	fprintf(stdout,"%d ", data[n]);
	            }

		        if(bits_to_go==8)
				    emit_bits_checksc (data[len-1],8);
	            else
				    emit_bits_checksc (data[len-1]>>bits_to_go,8-bits_to_go);

		        /* if last bit is 1, output 1 */
		        if(bits_to_go==8)
			        bits_to_go = 0;
		        Bit = (data[len-1]>>bits_to_go)&0x01;
		            if(!Bit)
			    emit_bits_checksc(1,1);

	            if(DEBUG_BS)
		            fprintf(stdout, "color %d spa_lev %d snr_lev %d: %ld\n",
			            col, spalev, snr_lev-snroffset, len);
	      
	    } /* end of !all_zero */
	  } /* end of color plane */     
	} /* end of spatial level */
   }
  }  /* end of else scan_order */
}

/* unpacke the bitstream into Spatial and SNR layer bitstreams 
   SPlayer data structure will be created here  */

void CVTCDecoder::PEZW_bit_unpack (PEZW_SPATIAL_LAYER **SPlayer)
{
   int splev, snrlev;
   int spatial_id, snr_id;
   int color;
   int Quant[3]={0,0,0}, q;
   int snr_scalability_levels;
   int texture_spatial_layer_start_code;
   int snr_layer_start_code;
   unsigned char buffer[MAXSIZE];
   long len;
   int h,w;
   int n;
   int Snrlevels;
//   int i, j;

   /* initialize data structure */
   h=mzte_codec.m_iHeight;
   w=mzte_codec.m_iWidth;
   for (color=0;color<mzte_codec.m_iColors;color++)
     SPlayer[color] = (PEZW_SPATIAL_LAYER *)calloc (mzte_codec.m_iWvtDecmpLev,
						    sizeof(PEZW_SPATIAL_LAYER));
   
   	 
   /* get the quantization step sizes */
   get_X_bits_checksc_init();
   for (color=0;color<mzte_codec.m_iColors;color++){
	   do {
	     q = get_X_bits_checksc(8);
	     Quant[color] <<= 7;
	     Quant[color] += q%128;
	   } while (q>=128);
	}

   /* get the snr scalability levels 
	    this is the maximum level for all three color components,
	    it will be adjusted later on according to all_zero flag */
   snr_scalability_levels = get_X_bits(5);
   
   	 
   /* get the snr layer */
   if (mzte_codec.m_bStartCodeEnable)
	 Snrlevels=snr_scalability_levels;
   else
	 Snrlevels=1;
       
   for (color=0;color<mzte_codec.m_iColors;color++){
	 /* allocate SNR data structure */
	 for (splev=0;splev<mzte_codec.m_iWvtDecmpLev;splev++)
	   {
	     SPlayer[color][splev].SNR_scalability_levels = snr_scalability_levels;
	     
	     SPlayer[color][splev].SNRlayer = (PEZW_SNR_LAYER *)
	       calloc(snr_scalability_levels, sizeof(PEZW_SNR_LAYER));
	     SPlayer[color][splev].SNRlayer[0].Quant = Quant[color];
	     
	     if (color==0){
	       SPlayer[color][0].SNRlayer[0].snr_image.height = h;
	       SPlayer[color][0].SNRlayer[0].snr_image.width =  w;
	     }
	     else{
	       SPlayer[color][0].SNRlayer[0].snr_image.height = h/2;
	       SPlayer[color][0].SNRlayer[0].snr_image.width =  w/2;
	     }
	   }
  } /* end of color */

  /* for bileve mode, start code is always enabled */
  if (!mzte_codec.m_bStartCodeEnable)
      {
        fprintf(stdout,"\nFor bilevel mode, SNR_start_code must be enabled!\n");
        exit(-1);
      }
        
  /* parse the bitstream */
  if (mzte_codec.m_bStartCodeEnable){
	 /* align byte. Note the difference between
      align_byte and align_byte1. */
	 align_byte();

    if(mzte_codec.m_iScanOrder==1){
     for (splev=0;splev<mzte_codec.m_iWvtDecmpLev;splev++)
       {
	     /* check start code */
	    if (mzte_codec.m_bStartCodeEnable){
		     align_byte1();
		     texture_spatial_layer_start_code = get_X_bits(32);
		     if (texture_spatial_layer_start_code != 
			    TEXTURE_SPATIAL_LAYER_START_CODE)
		        printf("Wrong texture_spatial_layer_start_code.");
		     spatial_id = get_X_bits(5);
		} 

 	    for (snrlev=0;snrlev<Snrlevels;snrlev++)
	    {
	    if (mzte_codec.m_bStartCodeEnable)
	     {
	       align_byte1();
	       snr_layer_start_code = get_X_bits(32);
	       if (snr_layer_start_code != texture_snr_layer_start_code)
		        printf("Wrong texture_snr_layer_start_code.");
	       snr_id = get_X_bits(5);

		   get_X_bits_checksc_init();
	     }
	   
	    len=0;
	      
	   /* first put all the data in SPlayer[0] */
	   /* the separation of the color components bitstreams will
	      be done by the decoding process */
	   
	   if ((splev==mzte_codec.m_iWvtDecmpLev-1)&&
	       (snrlev == Snrlevels-1))
	     len=get_allbits_checksc(buffer);
	   
	   else{
	     if ((!mzte_codec.m_bStartCodeEnable) /* only one bitstream */
		 || (snrlev==Snrlevels-1))
	       {
		 /* look for the next spatial layer start code */
		 while(1){
		   /* chekc for the next start code*/
		   if(!Is_startcode(TEXTURE_SPATIAL_LAYER_START_CODE)){
		     buffer[len] = get_X_bits_checksc(8);
		     len++;
		   }
		   else{
		     buffer[len++]=align_byte_checksc();
		     break; /* start code found */
		   }
		 } /* end of while */
	       }
	     else{
	       while(1){
		 /* chekc for the next start code*/
		 if(!Is_startcode(texture_snr_layer_start_code)){
		   buffer[len] = get_X_bits_checksc(8);
		   len++;
		 }
		 else{
		   buffer[len++]=align_byte_checksc();
		   break; /* start code found */
		 }
	       } /* end of while(1) */
	     } /* end of if(!mzte_...) */
	   } /* end of if(splev== ...) */
	   
	   SPlayer[0][splev].SNRlayer[snrlev].snr_bitstream.length = len;
	   SPlayer[0][splev].SNRlayer[snrlev].snr_bitstream.data =
	     (char *)calloc(len+2,sizeof(char));
	   
	   memcpy(SPlayer[0][splev].SNRlayer[snrlev].snr_bitstream.data,
		  buffer,len);
	   
	   if(DEBUG_BS_DETAIL)
	     for(n=0;n<len;n++)
	       fprintf(stdout,"%d ", 
		       ((unsigned char *)SPlayer[0][splev].SNRlayer[snrlev].snr_bitstream.data)[n]);
	   
	   if (DEBUG_BS)
	     fprintf(stdout,"spa_lev %d snr_lev %d: %ld\n", splev, snrlev, len);
	   
	    }  /* snr levels */       
     }  /* spatial levels */
   }
   else /* scan_order == 0 */
     { 
       for (snrlev=0;snrlev<Snrlevels;snrlev++) 
	    {
	     /* check start code */      
	    if (mzte_codec.m_bStartCodeEnable){
		    align_byte1();
		    snr_layer_start_code = get_X_bits(32);
		    if (snr_layer_start_code != texture_snr_layer_start_code)
			 printf("Wrong texture_snr_layer_start_code.");
		    snr_id = get_X_bits(5);
		}
	   
	   for (splev=0;splev<mzte_codec.m_iWvtDecmpLev;splev++)
	     {
	       if (mzte_codec.m_bStartCodeEnable)
		    {
		     align_byte1();
		     texture_spatial_layer_start_code = get_X_bits(32);
		     if (texture_spatial_layer_start_code != 
		       TEXTURE_SPATIAL_LAYER_START_CODE)
		        printf("Wrong texture_spatial_layer_start_code.");
		     spatial_id = get_X_bits(5);

		     get_X_bits_checksc_init();
		    }
	        len=0;
	       
	       /* first put all the data in SPlayer[0] */
	       /* the separation of the color components bitstreams will
		      be done by the decoding process */
	       
	       if ((splev==mzte_codec.m_iWvtDecmpLev-1)&&
		        (snrlev == Snrlevels-1))
		            len=get_allbits_checksc(buffer);
	       else{
		        if ((!mzte_codec.m_bStartCodeEnable) /* only one bitstream */
		            || (splev==mzte_codec.m_iWvtDecmpLev-1))
		        {
		             /* look for the next spatial layer start code */
		             while(1){
		                /* chekc for the next start code*/
		                if(!Is_startcode(texture_snr_layer_start_code)){
			                buffer[len] = get_X_bits_checksc(8);
			                len++;
		                }
		                else{
			                buffer[len++]=align_byte_checksc();
			                break; /* start code found */
		                }
		            } /* end of while */
		        }
		        else{
		            while(1){
		                /* chekc for the next start code*/
		                if(!Is_startcode(TEXTURE_SPATIAL_LAYER_START_CODE)){
		                    buffer[len] = get_X_bits_checksc(8);
		                    len++;
		                }
		                else{
		                    buffer[len++]=align_byte_checksc();
		                    break; /* start code found */
		                }
		            } /* end of while(1) */
		        } /* end of if(!mzte_...) */
	       } /* end of if(splev== ...) */
	       
	       SPlayer[0][splev].SNRlayer[snrlev].snr_bitstream.length = len;
	       
	       SPlayer[0][splev].SNRlayer[snrlev].snr_bitstream.data =
		        (char *)calloc(len+2,sizeof(char));
	       
	       memcpy(SPlayer[0][splev].SNRlayer[snrlev].snr_bitstream.data,
		      buffer,len);
	       
	       if(DEBUG_BS_DETAIL)
		        for(n=0;n<len;n++)
		            fprintf(stdout,"%d ", 
			             ((unsigned char *)SPlayer[0][splev].SNRlayer[snrlev].snr_bitstream.data)[n]);
	       
	       if (DEBUG_BS)
		        fprintf(stdout,"spa_lev %d snr_lev %d: %ld\n", splev, snrlev, len);
	     }  /* spatial levels */       
	 }  /* snr levels */
    } /* end of else scan_order */
   }
   else { /* no start code enabled */
        fprintf(stdout,"\nFor bilevel mode, SNR_start_code must be enabled!\n");
        exit(-1);
	}

    for(color=0;color<mzte_codec.m_iColors;color++) {
       h = SPlayer[color][0].SNRlayer[0].snr_image.height;
       w = SPlayer[color][0].SNRlayer[0].snr_image.width;    
       
       if ((SPlayer[color][0].SNRlayer[0].snr_image.data = calloc(h*w, sizeof(WINT)))
	      == NULL){
	        printf ("Can not allocate memory in Init_PEZWdata()");
	        exit(-1);
        }
    }
}

/* left shift a stream by N<8 bits */

int CVTCCommon::lshift_by_NBit (unsigned char *data, int len, int N)
{
  int n;
  char mask=0;
  int output;
  
  if(len==0)
    return 1;
  
  output = data[0]>>(8-N);
  
  for(n=0;n<N;n++)
    mask = (mask<<1) | 0x01;
  
  for (n=0;n<len-1;n++)
    data[n] = (data[n]<<N) | ((data[n+1]>>(8-N)) & mask);
  
  data[len-1] <<= N;
  
  return output;
}


/* free up memory at the encoder */
void CVTCEncoder::PEZW_freeEnc (PEZW_SPATIAL_LAYER **SPlayer)
{
  int levels;
  int col, l;
  int snrlev;
  
  for (col=0; col<mzte_codec.m_iColors; col++)
    {
      /* free the image buffer */
      free(SPlayer[col][0].SNRlayer[0].snr_image.data);
      
      /* free the mask buffer */
      free(SPlayer[col]->SNRlayer[0].snr_image.mask);	
      
      if (col ==0) 
	levels = mzte_codec.m_iWvtDecmpLev;
      else
	levels = mzte_codec.m_iWvtDecmpLev-1;
      
      /* free the SNR layer */
      for(l=0;l<levels;l++){
	for (snrlev=0;snrlev<SPlayer[col][l].SNR_scalability_levels;
	     snrlev++)
	  if(SPlayer[col][l].SNRlayer[snrlev].snr_bitstream.data!=NULL)
	    free(SPlayer[col][l].SNRlayer[snrlev].snr_bitstream.data);
	
	free(SPlayer[col][l].SNRlayer);
      }
      
      /* free SPlayer */
      free(SPlayer[col]);
    }
  
  /* parameters for inverse wvt */
  mzte_codec.m_iSpatialLev = mzte_codec.m_iTargetSpatialLev;
  mzte_codec.m_iTargetSpatialLev = PEZW_target_spatial_levels;
}



void CVTCDecoder::PEZW_freeDec (PEZW_SPATIAL_LAYER **SPlayer)
{
  int levels;
  int col, l;
  int snrlev;
  
  for (col=0; col<mzte_codec.m_iColors; col++)
    {
      /* free the image buffer */
      free(SPlayer[col][0].SNRlayer[0].snr_image.data);
      
      /* free the mask buffer */
	  free(SPlayer[col][0].SNRlayer[0].snr_image.mask );
	  
	  if (col ==0) 
	    levels = mzte_codec.m_iWvtDecmpLev;
	  else
	    levels = mzte_codec.m_iWvtDecmpLev-1;
      
	  /* free the SNR layer */
	  for(l=0;l<levels;l++){
	    if (!mzte_codec.m_bStartCodeEnable)
	      free(SPlayer[col][l].SNRlayer[0].snr_bitstream.data);
	    else {
	      if (col==0){
		for (snrlev=0;snrlev<SPlayer[col][l].SNR_scalability_levels;
		     snrlev++)
		  if(SPlayer[col][l].SNRlayer[snrlev].snr_bitstream.data!=NULL)
		    free(SPlayer[col][l].SNRlayer[snrlev].snr_bitstream.data);
	      }
	    }
	  }
    }
  
  
  /* free SNRlayer */
  levels = mzte_codec.m_iWvtDecmpLev;
  for (col=0; col<mzte_codec.m_iColors; col++)
    for(l=0;l<levels;l++)
      free(SPlayer[col][l].SNRlayer);
  
  /* free SPlayer */
  for (col=0; col<mzte_codec.m_iColors; col++)
    free(SPlayer[col]);

  /* parameters for inverse wvt */
  if(PEZW_target_spatial_levels>mzte_codec.m_iWvtDecmpLev)
    PEZW_target_spatial_levels=mzte_codec.m_iWvtDecmpLev;
  mzte_codec.m_iSpatialLev = PEZW_target_spatial_levels;
  mzte_codec.m_iTargetSpatialLev = PEZW_target_spatial_levels;
}


