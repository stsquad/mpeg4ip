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
   File name:         wvtpezw_tree_decode.c
   Author:            Jie Liang  (liang@ti.com)
   Functions:         Core functions for decoding the tree blocks of coeffs.
   Revisions:         v1.0 (10/04/98)
*****************************************************************************/

#include "wvtPEZW.hpp"
#include "PEZW_zerotree.hpp"
#include "wvtpezw_tree_codec.hpp"
#include "PEZW_functions.hpp"

/* decode a block of wavelet coefficients */

void PEZW_decode_block (WINT **coeffsBlk, Int width, Int height)
{
    Int dc_hsize, dc_vsize;
    Int i,j,k,m,n,band, hpos, vpos;
    int pos;
	int hpos_start, hpos_end;
	int vpos_start, vpos_end;
    Int lev, npix;
    Int hstart, vstart,start, end;
    Int x,y;
    Char SignBit;
    Int levels;

#ifdef DEBUG_FILE
    Int NumTree=0;
#endif

    levels = tree_depth;
    dc_hsize = width>>levels;
    dc_vsize = height>>levels;

	/* decode the AC coefficients:
       loop over all zerotrees */

	for(i=0;i<dc_vsize;i++)
		for (j=0;j<dc_hsize;j++)
			for(band=0;band<3;band++)
			{
				if(band==0)  { /* HL band */
					hpos = j+dc_hsize;
					vpos = i;
				}
				else if (band==1) { /* LH band */
					hpos = j;
					vpos = i+dc_vsize;
				}
				else{
					hpos = j+dc_hsize;
					vpos = i+dc_vsize;
				}

				y=vpos;x=hpos;

				/* determin weighting */
				for (lev=0;lev<levels;lev++)
					snr_weight[lev] = 0;

				/* initialization */
				num_Sig=0;
				the_wvt_tree[0]=0;
				prev_label[0]=ZTRZ;
				for(m=1;m<len_tree_struct;m++){
					the_wvt_tree[m]=0;
					prev_label[m]=DZ;
				}

#ifdef DEBUG_FILE
				NumTree++;
				if(DEBUG_SYMBOL)
					fprintf(fp_debug,"\n***NEW TREE %d***", NumTree);
#endif
				/* encoder one wavelet tree. this function demonstrates that 
				the requirement for memory is very low for this entropy coder.
				if combined with a localized wavelet transform, the total
				memory requirement will be extremely low. */
				PEZW_tree_decode (band,the_wvt_tree,snr_weight);

				/* dequantize the coefficients */
				for(k=0;k<num_Sig;k++){
					SignBit=sign_bit[k];
					n=sig_pos[k];
					m=the_wvt_tree[n];
					the_wvt_tree[n]=(SignBit>0)?m:-m;
				}

#ifdef DEBUG_FILE
				if(DEBUG_VALUE){
					fprintf(fp_debug,"\n***NEW TREE %d *****\n",NumTree);
					fprintf(fp_debug,"%d ",the_wvt_tree[0]);
				}
#endif

				/* copy the data from wvt_tree to output */
				coeffsBlk[y][x]=the_wvt_tree[0];
				for (lev=1;lev<levels;lev++)
					{
						npix=1<<(2*(lev-1));
						pos = level_pos[lev];
						hstart=hpos*(1<<(lev-1));
						vstart=vpos*(1<<(lev-1));
						start=level_pos[lev-1];
						end=level_pos[lev];

						for(k=start;k<end;k++)
							{
								hpos_start = (hstart+hloc_map[k])*2;
								hpos_end = hpos_start+2;
								vpos_start = (vstart+vloc_map[k])*2;
								vpos_end = vpos_start+2;
								for(y=vpos_start;y<vpos_end;y++)
									for(x=hpos_start;x<hpos_end;x++){
										coeffsBlk[y][x]=the_wvt_tree[pos++];
#ifdef DEBUG_FILE
										if(DEBUG_VALUE){
											fprintf(fp_debug,"%d ",coeffsBlk[y][x]);
											fflush(fp_debug);
										}
#endif
									}
								} /* end of k */
					} /* end of for(lev) */				
			} /* end of band */

}


/* decode a tree structure */
void PEZW_tree_decode (int band,WINT *wvt_tree,int *snr_weight)
{
	int i,j,k,m,n;
	int npix;
	int bplane;
	int scan_tree_done;
	int context;
	unsigned char label;
	int num_ScanTree, next_numScanTree;
	int temp1;
	short *temp_ptr;
	char Bit;
	//	char IsStream=1;
	unsigned char bpos;
	char skip_flag;

#define AC_offset 3

	for(bplane=Max_Bitplane-1;bplane>=Min_Bitplane;bplane--){
#ifdef DEBUG_FILE
		if(DEBUG_SYMBOL)
			fprintf(fp_debug,"\n****bitplane: %d",bplane);
#endif
	    /* determine bitplane */
 		skip_flag=0;
	    for(i=0;i<tree_depth-spatial_leveloff;i++){
			bitplane[i] = bplane+snr_weight[i];
			if(bitplane[i]>=Max_Bitplane)
				skip_flag=1;
		}
		if(skip_flag)
			continue;   /* skip this bitplane */

		/* receive refinement bits */
		if(bplane<Max_Bitplane-1){
			for(i=0;i<num_Sig;i++){
				m=sig_layer[i];
                n = bitplane[m];

                /* context for refinement bit */
                context = m*MAX_BITPLANE+bitplane[m];

                /* rate control */
// #define FLAG reach_budget[m][n] deleted by swinder
                if(reach_budget[m][n]==1)
                    continue;                   

				Bit=Ac_decode_symbol(&Decoder[m][bitplane[m]],
                    &model_sub[context]);
               /* rate control */
               if( (Decoder[m][n].stream - PEZW_bitstream[m][n]) >= Init_Bufsize[m][n]+AC_offset){
                       reach_budget[m][n] = 1;
                       return;
               }

			   wvt_tree[sig_pos[i]] |= (Bit<<bitplane[m]);
			}
		}

		/* form the zerotree and encode the zerotree symbols
		   and encode the sign bit for new sigifnicant coeffs
		   ScanTree: the intervals of coefficients that are 
		             not descendants of zerotree roots.
		   Use ScanTree and next_ScanTree to alternate as 
		   buffer, so that we do not need to spend the cycles
		   to copy from one buffer to the other. 

			numScanTree and next_numScanTrees are the number 
			of intervals that need to be looked at.
		*/

		next_numScanTree=1;
		next_ScanTrees[0]=0;
		next_ScanTrees[1]=1;
		for(i=0;i<tree_depth-spatial_leveloff;i++){

#ifdef DEBUG_FILE
		if(DEBUG_SYMBOL)
			fprintf(fp_debug,"\n****tree_depth %d:  ",i);
#endif
			scan_tree_done=1;
			num_ScanTree=next_numScanTree;
			next_numScanTree=0;
			temp_ptr=ScanTrees;
			ScanTrees=next_ScanTrees;
			next_ScanTrees=temp_ptr;			
			bpos=bitplane[i];

            /* rate control */
// #define FLAG reach_budget[i][bpos] deleted by swinder
            if(reach_budget[i][bpos]==1)
                    break; 

			npix=1<<(2*i);
			for(j=0;j<num_ScanTree;j++)
			  for(k=ScanTrees[2*j];k<ScanTrees[2*j+1];k++)
			  {
				/* decide the zerotree symbol */
				if((prev_label[k]==IVAL)||(prev_label[k]==ZTRV)){
#ifdef DEBUG_FILE
		            if(DEBUG_SYMBOL){
			            fprintf(fp_debug,"SKIP%d ", prev_label[k]);
			            fflush(fp_debug);
		            }
#endif

					if(i<tree_depth-1){
						/* set upt the scann tree for next level */
						temp1=2*next_numScanTree;
						next_ScanTrees[temp1]=level_pos[i+1]+4*(k-level_pos[i]);
						next_ScanTrees[temp1+1]=next_ScanTrees[temp1]+4;
						next_numScanTree++;
						scan_tree_done=0;
					}

					continue;
				}
				else{
					/* decode the zerotree symbol */
				    context=bpos*NumContexts*tree_depth+i*NumContexts+
				        prev_label[k]*NumBands+band;
				    label = Ac_decode_symbol(&Decoder[i][bpos],&context_model[context]);				  

                    /* rate control */
                    if( (Decoder[i][bpos].stream - PEZW_bitstream[i][bpos]) >= Init_Bufsize[i][bpos]+AC_offset){
                           reach_budget[i][bpos] = 1;
                           return;
                    }

#ifdef DEBUG_FILE
		if(DEBUG_SYMBOL){
			fprintf(fp_debug,"%d ", label);
			fflush(fp_debug);
		}
#endif


                    /* context for sign model */
                    context = i*MAX_BITPLANE+bpos;

					if((label==IVAL)||(label==IZER)){
						scan_tree_done=0;
						if(label==IVAL){							
							  Bit=1;
							  wvt_tree[k] |= (Bit<<bitplane[i]);

							  sig_pos[num_Sig]=k;
						      sig_layer[num_Sig]=i;

							  sign_bit[num_Sig]= Ac_decode_symbol(&Decoder[i][bpos],&model_sign[context]);	
                              if( (Decoder[i][bpos].stream - PEZW_bitstream[i][bpos]) >= Init_Bufsize[i][bpos]+AC_offset){
                                  reach_budget[i][bpos] = 1;
                                  return;
                              }


#ifdef DEBUG_FILE
							  if(DEBUG_SYMBOL){
								if(sign_bit[num_Sig]==1)
									fprintf(fp_debug,"%d+ ",sign_bit[num_Sig]);
								else
									fprintf(fp_debug,"%d- ",sign_bit[num_Sig]);
								fflush(fp_debug);
								}
#endif		  
			    				num_Sig++;
						}

						/* set upt the scann tree for next level */
						if(i<tree_depth-1){
						    temp1=2*next_numScanTree;
							next_ScanTrees[temp1]=level_pos[i+1]+4*(k-level_pos[i]);
							next_ScanTrees[temp1+1]=next_ScanTrees[temp1]+4;
							next_numScanTree++;
							scan_tree_done=0;
						}
					}
					else{
						if(label==ZTRV){
							  Bit=1;						  
							  wvt_tree[k] |= (Bit<<bitplane[i]);
						      sig_pos[num_Sig]=k;
						      sig_layer[num_Sig]=i;

							  sign_bit[num_Sig]= Ac_decode_symbol(&Decoder[i][bpos],&model_sign[context]);
                              if( (Decoder[i][bpos].stream - PEZW_bitstream[i][bpos]) >= Init_Bufsize[i][bpos]+AC_offset){
                                  reach_budget[i][bpos] = 1;
                                  return;
                              }

#ifdef DEBUG_FILE
							  if(DEBUG_SYMBOL){
								if(sign_bit[num_Sig]==1)
									fprintf(fp_debug,"%d+ ",sign_bit[num_Sig]);
								else
									fprintf(fp_debug,"%d- ",sign_bit[num_Sig]);
								fflush(fp_debug);
								}
#endif			
							  num_Sig++;
						}
					} /* end of else */			

					/* set up prev_label */
					prev_label[k]=label;
				} /* end of if(prev_label */
			} /* end of k */
			
			/* no subordinate none zero descendants */
			if(scan_tree_done)
			  break;
			
		}  /* end of tree depth */
		
	} /* end of bitplane */
	
	return;
}
