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
   File name:         wvtpezw_tree_encode.c
   Author:            Jie Liang  (liang@ti.com)
   Functions:         Core functions for encoding the tree blocks of coeffs.
   Revisions:         v1.0 (10/04/98)
*****************************************************************************/

#include "wvtPEZW.hpp"
#include "PEZW_zerotree.hpp"
#include "wvtpezw_tree_codec.hpp"
#include "PEZW_functions.hpp"

/* encode a block of wavelet coefficients */

void PEZW_encode_block (WINT **coeffsBlk,int width, int height)
{
    Int dc_hsize, dc_vsize;
    Int i,j,k,band, hpos, vpos;
    int pos;
	int hpos_start, hpos_end;
	int vpos_start, vpos_end;
    Int lev, npix;
    Int hstart, vstart,start, end;
    Int x,y;
    Int levels;
#ifdef DEBUG_FILE
    Int NumTree=0;
#endif
    levels = tree_depth;
    dc_hsize = width>>levels;
    dc_vsize = height>>levels;

	/* encode the AC coefficients:
       loop over all zerotrees */

	for(i=0;i<dc_vsize;i++)
		for (j=0;j<dc_hsize;j++)
			for(band=0;band<3;band++)
			{
				/* copy the data into the wavelet tree */
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

                /* quantization and copy */
				the_wvt_tree[0]=coeffsBlk[y][x];
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
								hpos_start = (hstart+hloc_map[k])<<1;
								hpos_end = hpos_start+2;
								vpos_start = (vstart+vloc_map[k])<<1;
								vpos_end = vpos_start+2;
								for(y=vpos_start;y<vpos_end;y++)
									for(x=hpos_start;x<hpos_end;x++){


                                        /* quantization and copy */
				                        the_wvt_tree[pos]=coeffsBlk[y][x]; 								
										pos++;
										}
							} /* end of k */
					} /* end of for(lev) */

				/* determin weighting */
				for (lev=0;lev<levels;lev++)
					snr_weight[lev] = 0;

				/* initialization */
				num_Sig=0;
				prev_label[0]=ZTRZ;
				for(k=1;k<len_tree_struct;k++)
					prev_label[k]=DZ;

#ifdef DEBUG_FILE
				NumTree++;
				if(DEBUG_SYMBOL)
					fprintf(fp_debug,"\n***NEW TREE   %d***", NumTree);
#endif

				/* encoder one wavelet tree. this function demonstrates that 
				the requirement for memory is very low for this entropy coder.
				if combined with a localized wavelet transform, the total
				memory requirement will be extremely low. */
				PEZW_tree_encode (band,the_wvt_tree);

				if(abs_wvt_tree[0]>MaxValue)
					MaxValue=abs_wvt_tree[0];
				if(wvt_tree_maxval[0]>MaxValue)
					MaxValue=wvt_tree_maxval[0];
				
			} /* end of band */

}


/* encode a tree of wavelet coefficients */

void PEZW_tree_encode (int band,WINT *wvt_tree)
{
	int i,j,k,m;
	int npix_block, npix;
	int pix_pos, max_pos;
	int maxval;
	int bplane;
	int scan_tree_done;
	int context;
	unsigned char label;
	int num_ScanTree, next_numScanTree;
	int temp1;
	short *temp_ptr;
	char signBit, Bit;
	//	char IsStream=1;
	unsigned char bpos;
	char skip_flag;
	char VAL_flag;

	char treeBit, NoTree_flag;

	/* first find out the largest value of the coefficients */
	for (i=tree_depth-1;i>=1;i--){
		npix_block = 1<<(2*(i-1));
		pix_pos=level_pos[i];
		max_pos=level_pos[i-1];
		for(j=0;j<npix_block;j++){
			maxval=0;
			for(k=0;k<4;k++){
				abs_wvt_tree[pix_pos]=ABS(wvt_tree[pix_pos]);
				if(abs_wvt_tree[pix_pos]>maxval)
					maxval=abs_wvt_tree[pix_pos];
				if(i<tree_depth-1){
					if(wvt_tree_maxval[pix_pos]>maxval)
						maxval=wvt_tree_maxval[pix_pos];
				}
				pix_pos++;
			}
			wvt_tree_maxval[max_pos++]=maxval;
		}
	}
	/* last level */
	abs_wvt_tree[0]=ABS(wvt_tree[0]);

	for(bplane=Max_Bitplane-1;bplane>=Min_Bitplane;bplane--){
#ifdef DEBUG_FILE
		if(DEBUG_SYMBOL)
			fprintf(fp_debug,"\n****bitplane: %d: ",bplane);
#endif
	    /* determine bitplane */
		skip_flag=0;
	    for(i=0;i<tree_depth;i++){
			bitplane[i] = bplane+snr_weight[i];
			if(bitplane[i]>=Max_Bitplane)
				skip_flag=1;
		}
		if(skip_flag)
			continue;   /* skip this bitplane */

		/* send refinement bits */
		if(bplane<Max_Bitplane-1){
			for(i=0;i<tree_depth;i++)
				maskbit[i] = (1<<bitplane[i]);

			for(i=0;i<num_Sig;i++){
				m=sig_layer[i];
				Bit=((abs_wvt_tree[sig_pos[i]]&maskbit[m])>0);

                /* context for refinement bit */
                context = m*MAX_BITPLANE+bitplane[m];
				Ac_encode_symbol(&Encoder[m][bitplane[m]],
                    &model_sub[context],Bit);
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
		for(i=0;i<tree_depth;i++){
#ifdef DEBUG_FILE
		if(DEBUG_SYMBOL)
			fprintf(fp_debug,"\n++++tree_depth %d: ",i);
#endif
			scan_tree_done=1;
			num_ScanTree=next_numScanTree;
			next_numScanTree=0;
			temp_ptr=ScanTrees;
			ScanTrees=next_ScanTrees;
			next_ScanTrees=temp_ptr;			
			bpos=bitplane[i];

			npix=1<<(2*i);
			for(j=0;j<num_ScanTree;j++)
			  for(k=ScanTrees[2*j];k<ScanTrees[2*j+1];k++)
			  {
				VAL_flag=0;
				NoTree_flag=0;
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
				  if(i==tree_depth-1){
					NoTree_flag=1;
				    /* the last level */
				    if(abs_wvt_tree[k]>>bpos){
				      label=IVAL;
					  VAL_flag=1;
				      
				      sig_pos[num_Sig]=k;
				      sig_layer[num_Sig]=i;
					  num_Sig++;
				    }
				    else
				      label=IZER;				       
				  }
				  else if(wvt_tree_maxval[k]>>bpos)
				    {	/* not a zerotree */
					  treeBit=0;
				      if(abs_wvt_tree[k]>>bpos){
							label=IVAL;
							VAL_flag=1;

							if(prev_label[k]!=ZTRV){
								sig_pos[num_Sig]=k;
								sig_layer[num_Sig]=i;
								num_Sig++;
							}
							else
								VAL_flag=0;
						}
				      else
						label=IZER;
				      
				      /* set upt the scann tree for next level */
				      temp1=2*next_numScanTree;
				      next_ScanTrees[temp1]=level_pos[i+1]+4*(k-level_pos[i]);
				      next_ScanTrees[temp1+1]=next_ScanTrees[temp1]+4;
				      next_numScanTree++;
				      scan_tree_done=0;
				    }
				  else
				    {	/* a zerotree root */
					  treeBit=1;
				      if(abs_wvt_tree[k]>>bpos){
						label=ZTRV;
						VAL_flag=1;

						sig_pos[num_Sig]=k;
						sig_layer[num_Sig]=i;
						num_Sig++;
						}
				      else{
						label=ZTRZ;
				      }
				      
				    }
				  
				  /* encode the zerotree symbol */
#ifdef DEBUG_FILE
		if(DEBUG_SYMBOL){
			fprintf(fp_debug,"%d ", label);
			fflush(fp_debug);
		}
#endif

				  context=bpos*NumContexts*tree_depth+i*NumContexts+
				    prev_label[k]*NumBands+band;
				  Ac_encode_symbol(&Encoder[i][bpos],&context_model[context],label);

				  /* encode the sign bit */
				  if(VAL_flag){
					  signBit = (wvt_tree[k]>0);

#ifdef DEBUG_FILE
					  if(DEBUG_SYMBOL){
						if(signBit==1)
							fprintf(fp_debug,"%d+ ",signBit);
						else /* signBit == 0 */
							fprintf(fp_debug,"%d- ",signBit);
						fflush(fp_debug);
						}
#endif


                       /* context for sign model */
                      context = i*MAX_BITPLANE+bpos;
					  Ac_encode_symbol(&Encoder[i][bpos],&model_sign[context],signBit);
				  }
				  
				  /* set up prev_label */
				  prev_label[k]=label;
				  
				} /* end of if(prev_label) */
			  }  /* end of k pixel */
			
			/* no subordinate none zero descendants */
			if(scan_tree_done)
			  break;
			
		}  /* end of tree depth */
		
	} /* end of bitplane */

	return;
}


