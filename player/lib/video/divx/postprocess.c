/**************************************************************************
 *                                                                        *
 * This code has been developed by John Funnell. This software is an      *
 * implementation of a part of one or more MPEG-4 Video tools as          *
 * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
 * software module in hardware or software products are advised that its  *
 * use may infringe existing patents or copyrights, and any such use      *
 * would be at such party's own risk.  The original developer of this     *
 * software module and his/her company, and subsequent editors and their  *
 * companies (including Project Mayo), will have no liability for use of  *
 * this software or modifications or derivatives thereof.                 *
 *                                                                        *
 * Project Mayo gives users of the Codec a license to this software       *
 * module or modifications thereof for use in hardware or software        *
 * products claiming conformance to the MPEG-4 Video Standard as          *
 * described in the Open DivX license.                                    *
 *                                                                        *
 * The complete Open DivX license can be found at                         *
 * http://www.projectmayo.com/opendivx/license.php                        *
 *                                                                        *
 **************************************************************************/
/**
*  Copyright (C) 2001 - Project Mayo
 *
 * John Funnell
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
**/
// postprocess.c //

/* Currently this contains only the deblocking filter.  The vertical    */
/* deblocking filter operates over eight pixel-wide columns at once.  The  */
/* horizontal deblocking filter works on four horizontals row at a time. */


#include "postprocess.h"

#ifdef PP_SELF_CHECK
#include <stdio.h>
#endif


#define ABS(a)     ( (a)>0 ? (a) : -(a) )
#define SIGN(a)    ( (a)<0 ? -1 : 1 )
#define MIN(a, b)  ( (a)<(b) ? (a) : (b) )
#define MAX(a, b)  ( (a)>(b) ? (a) : (b) )




/***** H O R I Z O N T A L   D E B L O C K I N G   F I L T E R *****/


#ifndef MPEG4IP

/* decide DC mode or default mode for the horizontal filter */
static int deblock_horiz_useDC(uint8_t *v, int stride) {
	int eq_cnt, useDC;
	int x, y;
	#ifdef PP_SELF_CHECK
	int eq_cnt2, j, k;
	#endif

	eq_cnt = 0;
	for (y=0; y<4; y++) {
		for (x=1; x<=7; x++) {
			if (ABS(v[x+y*stride]-v[1+x+y*stride]) <= 1) eq_cnt--;
		}
	}

	#ifdef PP_SELF_CHECK
	eq_cnt2 = 0;
	for (k=0; k<4; k++) {
		for (j=1; j<=7; j++) {
			if (ABS(v[j+k*stride]-v[1+j+k*stride]) <= 1) eq_cnt2--;
		}
	}
	if (eq_cnt2 != eq_cnt) printf("ERROR: C version of useDC is incorrect\n");
	#endif

	useDC = eq_cnt <= -DEBLOCK_HORIZ_USEDC_THR;

	return useDC;
}


/* decide whether the DC filter should be turned on accoding to QP */
static int deblock_horiz_DC_on(uint8_t *v, int stride, int QP) {
	/* 99% of the time, this test turns out the same as the |max-min| strategy in the standard */
	return (ABS(v[1]-v[8]) < 2*QP);
}





/* The 9-tap low pass filter used in "DC" regions */
static void deblock_horiz_lpf9(uint8_t *v, int stride, int QP) {
	int x, y, p1, p2, psum;
	uint8_t *vv, vnew[9];
	#ifdef PP_SELF_CHECK
	uint8_t selfcheck[9];
	int psum2;
	uint8_t *vv2; 
	#endif

	for (y=0; y<4; y++) {
		p1 = (ABS(v[0+y*stride]-v[1+y*stride]) < QP ) ?  v[0+y*stride] : v[1+y*stride];
		p2 = (ABS(v[8+y*stride]-v[9+y*stride]) < QP ) ?  v[9+y*stride] : v[8+y*stride];


		#ifdef PP_SELF_CHECK
		/* generate a self-check version of the filter result in selfcheck[9] */
		/* low pass filtering (LPF9: 1 1 2 2 4 2 2 1 1) */
		vv2 = &(v[y*stride]);
		psum2 = p1 + p1 + p1 + vv2[1] + vv2[2] + vv2[3] + vv2[4] + 4;
		selfcheck[1] = (((psum2 + vv2[1]) << 1) - (vv2[4] - vv2[5])) >> 4;
		psum2 += vv2[5] - p1; 
		selfcheck[2] = (((psum2 + vv2[2]) << 1) - (vv2[5] - vv2[6])) >> 4;
		psum2 += vv2[6] - p1; 
		selfcheck[3] = (((psum2 + vv2[3]) << 1) - (vv2[6] - vv2[7])) >> 4;
		psum2 += vv2[7] - p1; 
		selfcheck[4] = (((psum2 + vv2[4]) << 1) + p1 - vv2[1] - (vv2[7] - vv2[8])) >> 4;
		psum2 += vv2[8] - vv2[1]; 
		selfcheck[5] = (((psum2 + vv2[5]) << 1) + (vv2[1] - vv2[2]) - vv2[8] + p2) >> 4;
		psum2 += p2 - vv2[2]; 
		selfcheck[6] = (((psum2 + vv2[6]) << 1) + (vv2[2] - vv2[3])) >> 4;
		psum2 += p2 - vv2[3]; 
		selfcheck[7] = (((psum2 + vv2[7]) << 1) + (vv2[3] - vv2[4])) >> 4;
		psum2 += p2 - vv2[4]; 
		selfcheck[8] = (((psum2 + vv2[8]) << 1) + (vv2[4] - vv2[5])) >> 4;
		#endif

		/* C implementation of horizontal LPF */
		vv = &(v[y*stride]);
		psum = p1 + p1 + p1 + vv[1] + vv[2] + vv[3] + vv[4] + 4;
		vnew[1] = (((psum + vv[1]) << 1) - (vv[4] - vv[5])) >> 4;
		psum += vv[5] - p1; 
		vnew[2] = (((psum + vv[2]) << 1) - (vv[5] - vv[6])) >> 4;
		psum += vv[6] - p1; 
		vnew[3] = (((psum + vv[3]) << 1) - (vv[6] - vv[7])) >> 4;
		psum += vv[7] - p1; 
		vnew[4] = (((psum + vv[4]) << 1) + p1 - vv[1] - (vv[7] - vv[8])) >> 4;
		psum += vv[8] - vv[1]; 
		vnew[5] = (((psum + vv[5]) << 1) + (vv[1] - vv[2]) - vv[8] + p2) >> 4;
		psum += p2 - vv[2]; 
		vnew[6] = (((psum + vv[6]) << 1) + (vv[2] - vv[3])) >> 4;
		psum += p2 - vv[3]; 
		vnew[7] = (((psum + vv[7]) << 1) + (vv[3] - vv[4])) >> 4;
		psum += p2 - vv[4]; 
		vnew[8] = (((psum + vv[8]) << 1) + (vv[4] - vv[5])) >> 4;
		for (x=1; x<=8; x++) {
			vv[x] = vnew[x];
		}
	
		#ifdef PP_SELF_CHECK
		for (x=1; x<=8; x++) {
			if (selfcheck[x] != v[x+y*stride]) printf("ERROR: C version of horiz lpf9 is incorrect\n");
		}
		#endif

	}

}


#endif /* !MPEG4IP */


/* horizontal deblocking filter used in default (non-DC) mode */
static void deblock_horiz_default_filter(uint8_t *v, int stride, int QP) {
	int a3_0, a3_1, a3_2, d;
	int q1, q;
	int y;

	for (y=0; y<4; y++) {

		q1 = v[4] - v[5];
		q = q1 / 2;
		if (q) {
			
			a3_0 = 2*(v[3]-v[6]) - 5*q1;
			
			/* apply the 'delta' function first and check there is a difference to avoid wasting time */
			if (ABS(a3_0) < 8*QP) {
		
				a3_1 = 2*(v[1]-v[4]) + 5*(v[3]-v[2]);
				a3_2 = 2*(v[5]-v[8]) + 5*(v[7]-v[8]);
				d = ABS(a3_0) - MIN(ABS(a3_1), ABS(a3_2));
		
				if (d > 0) { /* energy across boundary is greater than in one or both of the blocks */
					d += d<<2;
					d = (d + 32) >> 6; 
	
					if (d > 0) {
	
						d *= SIGN(-a3_0);
					
						/* clip d in the range 0 ... q */
						if (q > 0) {
							d = d<0 ? 0 : d;
							d = d>q ? q : d;
						} else {
							d = d>0 ? 0 : d;
							d = d<q ? q : d;
						}
						
						v[4] -= d;
						v[5] += d;
		
					}
				}
			}
		}

		#ifdef PP_SELF_CHECK
		/* no selfcheck written for this yet */
		#endif

		v += stride;
	}


}




/* this is a horizontal deblocking filter - i.e. it will smooth _vertical_ block edges */
static void deblock_horiz(uint8_t *image, int width, int height, int stride, QP_STORE_T *QP_store, int QP_stride, int chroma) {
	int x, y;
	int QP;
	uint8_t *v;
#ifndef MPEG4IP
	int useDC, DC_on;
#endif

	/* loop over image's pixel rows , four at a time */
	for (y=0; y<height; y+=4) {	
		
		/* loop over every block boundary in that row */
		for (x=8; x<width; x+=8) {
		
			/* extract QP from the decoder's array of QP values */
			QP = chroma ? QP_store[y/8*QP_stride+x/8]
			                : QP_store[y/16*QP_stride+x/16];	

			/* v points to pixel v0, in the left-hand block */
			v = &(image[y*stride + x]) - 5;

#ifdef MPEG4IP
			/* just use default mode */
			deblock_horiz_default_filter(v, stride, QP);
#else
			/* first decide whether to use default or DC offet mode */ 
			useDC = deblock_horiz_useDC(v, stride);

			if (useDC) { /* use DC offset mode */
				
				DC_on = deblock_horiz_DC_on(v, stride, QP);

				if (DC_on) {

					deblock_horiz_lpf9(v, stride, QP); 

					#ifdef SHOWDECISIONS_H
					if (!chromaFlag) {
						v[0*stride + 4] = 
						v[1*stride + 4] = 
						v[2*stride + 4] = 
						v[3*stride + 4] = 255;  
					}
					#endif
				}

			} else {     /* use default mode */
			
				deblock_horiz_default_filter(v, stride, QP);

				#ifdef SHOWDECISIONS_H
				if (!chromaFlag) {
					v[0*stride + 4] = 
					v[1*stride + 4] = 
					v[2*stride + 4] = 
					v[3*stride + 4] = 0;  
				}
				#endif

			}
#endif /* MPEG4IP */
		}
	}
}





/***** V E R T I C A L   D E B L O C K I N G   F I L T E R *****/

#ifndef MPEG4IP

/* decide DC mode or default mode in assembler */
static int deblock_vert_useDC(uint8_t *v, int stride) {
	int eq_cnt, useDC, x, y;
	#ifdef PP_SELF_CHECK
	int useDC2, j, i;
	#endif

	#ifdef PP_SELF_CHECK
	/* C-code version for testing */
	eq_cnt = 0;
	for (j=1; j<8; j++) {
		for (i=0; i<8; i++) {
			if (ABS(v[j*stride+i] - v[(j+1)*stride+i]) <= 1) eq_cnt++;
		}
	}
	useDC2 = (eq_cnt > DEBLOCK_VERT_USEDC_THR); 
	#endif
			
	/* C-code imlementation of vertial useDC */
	eq_cnt = 0;
	for (y=1; y<8; y++) {
		for (x=0; x<8; x++) {
			if (ABS(v[y*stride+x] - v[(y+1)*stride+x]) <= 1) eq_cnt++;
		}
	}
	useDC = (eq_cnt  > DEBLOCK_VERT_USEDC_THR);			
			
	#ifdef PP_SELF_CHECK
	if (useDC != useDC2) printf("ERROR: C version of useDC is incorrect\n");
	#endif
	
	return useDC;
}





/* decide whether the DC filter should be turned on accoding to QP */
static int deblock_vert_DC_on(uint8_t *v, int stride, int QP) {
	int DC_on, x;
	#ifdef PP_SELF_CHECK
	int i, DC_on2;
	#endif

	#ifdef PP_SELF_CHECK
	DC_on2 = 1;
	for (i=0; i<8; i++) {
		if (ABS(v[i+1*stride]-v[i+8*stride]) > 2 *QP) DC_on2 = 0;
	}
	#endif
	
	/* C implementation of vertical DC_on */
	DC_on = 1;
	for (x=0; x<8; x++) {
		if (ABS(v[x+1*stride]-v[x+8*stride]) > 2 *QP) DC_on = 0;
	}
				
	#ifdef PP_SELF_CHECK
	if (DC_on != DC_on2) printf("ERROR: C version of DC_on is incorrect\n");
	#endif

	return DC_on;
}







/* Vertical 9-tap low-pass filter for use in "DC" regions of the picture */
void deblock_vert_lpf9(uint64_t *v_local, uint64_t *p1p2, uint8_t *v, int stride, int QP) {
	int x, y;
	int p1, p2, psum;
	uint8_t  *vv, vnew[9];
	/* define semi-constants to enable us to move up and down the picture easily... */
	int l1 = 1 * stride;
	int l2 = 2 * stride;
	int l3 = 3 * stride;
	int l4 = 4 * stride;
	int l5 = 5 * stride;
	int l6 = 6 * stride;
	int l7 = 7 * stride;
	int l8 = 8 * stride;
	#ifdef PP_SELF_CHECK
	int j, k;
	uint8_t selfcheck[64];
	#endif


	#ifdef PP_SELF_CHECK
	/* generate a self-check version of the filter result in selfcheck[64] */
	for (j=0; j<8; j++) { /* loop left->right */
		vv = &(v[j]);
		p1 = (ABS(vv[0*stride]-vv[1*stride]) < QP ) ?  vv[0*stride] : vv[1*stride];
		p2 = (ABS(vv[8*stride]-vv[9*stride]) < QP ) ?  vv[9*stride] : vv[8*stride];
		/* the above may well be endian-fussy */
		psum = p1 + p1 + p1 + vv[l1] + vv[l2] + vv[l3] + vv[l4] + 4; 
		selfcheck[j+8*0] = (((psum + vv[l1]) << 1) - (vv[l4] - vv[l5])) >> 4; 
		psum += vv[l5] - p1; 
		selfcheck[j+8*1] = (((psum + vv[l2]) << 1) - (vv[l5] - vv[l6])) >> 4; 
		psum += vv[l6] - p1; 
		selfcheck[j+8*2] = (((psum + vv[l3]) << 1) - (vv[l6] - vv[l7])) >> 4; 
		psum += vv[l7] - p1; 
		selfcheck[j+8*3] = (((psum + vv[l4]) << 1) + p1 - vv[l1] - (vv[l7] - vv[l8])) >> 4; 
		psum += vv[l8] - vv[l1];  
		selfcheck[j+8*4] = (((psum + vv[l5]) << 1) + (vv[l1] - vv[l2]) - vv[l8] + p2) >> 4; 
		psum += p2 - vv[l2];  
		selfcheck[j+8*5] = (((psum + vv[l6]) << 1) + (vv[l2] - vv[l3])) >> 4; 
		psum += p2 - vv[l3]; 
		selfcheck[j+8*6] = (((psum + vv[l7]) << 1) + (vv[l3] - vv[l4])) >> 4; 
		psum += p2 - vv[l4]; 
		selfcheck[j+8*7] = (((psum + vv[l8]) << 1) + (vv[l4] - vv[l5])) >> 4; 
	}
	#endif
	
	/* simple C implementation of vertical default filter */
	for (x=0; x<8; x++) { /* loop left->right */
		vv = &(v[x]);
		p1 = (ABS(vv[0*stride]-vv[1*stride]) < QP ) ?  vv[0*stride] : vv[1*stride];
		p2 = (ABS(vv[8*stride]-vv[9*stride]) < QP ) ?  vv[9*stride] : vv[8*stride];
		/* the above may well be endian-fussy */
		psum = p1 + p1 + p1 + vv[l1] + vv[l2] + vv[l3] + vv[l4] + 4; 
		vnew[1] = (((psum + vv[l1]) << 1) - (vv[l4] - vv[l5])) >> 4; 
		psum += vv[l5] - p1; 
		vnew[2] = (((psum + vv[l2]) << 1) - (vv[l5] - vv[l6])) >> 4; 
		psum += vv[l6] - p1; 
		vnew[3] = (((psum + vv[l3]) << 1) - (vv[l6] - vv[l7])) >> 4; 
		psum += vv[l7] - p1; 
		vnew[4] = (((psum + vv[l4]) << 1) + p1 - vv[l1] - (vv[l7] - vv[l8])) >> 4; 
		psum += vv[l8] - vv[l1];  
		vnew[5] = (((psum + vv[l5]) << 1) + (vv[l1] - vv[l2]) - vv[l8] + p2) >> 4; 
		psum += p2 - vv[l2];  
		vnew[6] = (((psum + vv[l6]) << 1) + (vv[l2] - vv[l3])) >> 4; 
		psum += p2 - vv[l3]; 
		vnew[7] = (((psum + vv[l7]) << 1) + (vv[l3] - vv[l4])) >> 4; 
		psum += p2 - vv[l4]; 
		vnew[8] = (((psum + vv[l8]) << 1) + (vv[l4] - vv[l5])) >> 4;
		for (y=1; y<=8; y++) {
			vv[y*stride] = vnew[y];
		}  
	}
	
	#ifdef PP_SELF_CHECK
	/* use the self-check version of the filter result in selfcheck[64] to verify the filter output */
	for (k=0; k<8; k++) { /* loop top->bottom */
		for (j=0; j<8; j++) { /* loop left->right */
			if (v[(k+1)*stride + j] != selfcheck[j+8*k]) printf("ERROR: problem with C filter in row %d\n", k+1);
		}
	}
	#endif

}

#endif /* !MPEG4IP */


/* Vertical deblocking filter for use in non-flat picture regions */
static void deblock_vert_default_filter(uint8_t *v, int stride, int QP) {
	int x, a3_0, a3_1, a3_2, d, q;
	/* define semi-constants to enable us to move up and down the picture easily... */
	int l1 = 1 * stride;
	int l2 = 2 * stride;
	int l3 = 3 * stride;
	int l4 = 4 * stride;
	int l5 = 5 * stride;
	int l6 = 6 * stride;
	int l7 = 7 * stride;
	int l8 = 8 * stride;
	
	#ifdef PP_SELF_CHECK
	int j, k, a3_0_SC, a3_1_SC, a3_2_SC, d_SC, q_SC;
	uint8_t selfcheck[8][2];
	#endif

	#ifdef PP_SELF_CHECK
	/* compute selfcheck matrix for later comparison */
	for (j=0; j<8; j++) {
		a3_0_SC = 2*v[l3+j] - 5*v[l4+j] + 5*v[l5+j] - 2*v[l6+j];	
		a3_1_SC = 2*v[l1+j] - 5*v[l2+j] + 5*v[l3+j] - 2*v[l4+j];	
		a3_2_SC = 2*v[l5+j] - 5*v[l6+j] + 5*v[l7+j] - 2*v[l8+j];	
		q_SC    = (v[l4+j] - v[l5+j]) / 2;

		if (ABS(a3_0_SC) < 8*QP) {

			d_SC = ABS(a3_0_SC) - MIN(ABS(a3_1_SC), ABS(a3_2_SC));
			if (d_SC < 0) d_SC=0;
				
			d_SC = (5*d_SC + 32) >> 6; 
			d_SC *= SIGN(-a3_0_SC);
							
			//printf("d_SC[%d] preclip=%d\n", j, d_SC);
			/* clip d in the range 0 ... q */
			if (q_SC > 0) {
				d_SC = d_SC<0    ? 0    : d_SC;
				d_SC = d_SC>q_SC ? q_SC : d_SC;
			} else {
				d_SC = d_SC>0    ? 0    : d_SC;
				d_SC = d_SC<q_SC ? q_SC : d_SC;
			}
						
		} else {
			d_SC = 0;		
		}
		selfcheck[j][0] = v[l4+j] - d_SC;
		selfcheck[j][1] = v[l5+j] + d_SC;
	}
	#endif

	/* simple C implementation of vertical default filter */
	for (x=0; x<8; x++) {
		a3_0 = 2*v[l3+x] - 5*v[l4+x] + 5*v[l5+x] - 2*v[l6+x];	
		a3_1 = 2*v[l1+x] - 5*v[l2+x] + 5*v[l3+x] - 2*v[l4+x];	
		a3_2 = 2*v[l5+x] - 5*v[l6+x] + 5*v[l7+x] - 2*v[l8+x];	
		q    = (v[l4+x] - v[l5+x]) / 2;

		if (ABS(a3_0) < 8*QP) {

			d = ABS(a3_0) - MIN(ABS(a3_1), ABS(a3_2));
			if (d < 0) d=0;
				
			d = (5*d + 32) >> 6; 
			d *= SIGN(-a3_0);
							
			//printf("d[%d] preclip=%d\n", x, d);
			/* clip d in the range 0 ... q */
			if (q > 0) {
				d = d<0    ? 0    : d;
				d = d>q ? q : d;
			} else {
				d = d>0    ? 0    : d;
				d = d<q ? q : d;
			}
						
		} else {
			d = 0;		
		}
		v[l4+x] -= d;
		v[l5+x] += d;
	}
	
	
	


	#ifdef PP_SELF_CHECK
	/* do selfcheck */
	for (j=0; j<8; j++) {
		for (k=0; k<2; k++) {
			if (selfcheck[j][k] != v[l4+j+k*stride]) {
				printf("ERROR: problem with vertical default filter in col %d, row %d\n", j, k);	
				printf("%d should be %d\n", v[l4+j+k*stride], selfcheck[j][k]);	
				
			}
		}
	}
	#endif

	


}










/* this is a vertical deblocking filter - i.e. it will smooth _horizontal_ block edges */
static void deblock_vert( uint8_t *image, int width, int height, int stride, QP_STORE_T *QP_store, int QP_stride, int chroma) {
	int Bx, y;
	int QP, QPx16;
	uint8_t *v;
#ifndef MPEG4IP
	uint64_t v_local[20];
	uint64_t p1p2[4];
	int useDC, DC_on;
#endif

	/* loop over image's block boundary rows */
	for (y=8; y<height; y+=8) {	
		
		/* loop over all blocks, left to right */
		for (Bx=0; Bx<width; Bx+=8) {

			QP = chroma ? QP_store[y/8*QP_stride+Bx/8]
			            : QP_store[y/16*QP_stride+Bx/16];	
			QPx16 = 16 * QP;
			v = &(image[y*stride + Bx]) - 5*stride;

#ifdef MPEG4IP
			/* just use default mode */
			deblock_vert_default_filter(v, stride, QP);
#else
			/* decide whether to use DC mode on a block-by-block basis */
			useDC = deblock_vert_useDC(v, stride);
						
			if (useDC) {
 				/* we are in DC mode for this block.  But we only want to filter low-energy areas */
				
				/* decide whether the filter should be on or off for this block */
				DC_on = deblock_vert_DC_on(v, stride, QP);
				
				if (DC_on) { /* use DC offset mode */
				
						v = &(image[y*stride + Bx])- 5*stride;
						
						/* copy the block we're working on and unpack to 16-bit values */
						/* not needed for plain C version */
						//deblock_vert_copy_and_unpack(stride, &(v[stride]), &(v_local[2]), 8);
						//deblock_vert_choose_p1p2(v, stride, p1p2, QP);
					
						deblock_vert_lpf9(v_local, p1p2, v, stride, QP); 

						#ifdef SHOWDECISIONS_V
						if (!chromaFlag) {
							v[4*stride  ] = 
							v[4*stride+1] = 
							v[4*stride+2] = 
							v[4*stride+3] = 
							v[4*stride+4] = 
							v[4*stride+5] = 
							v[4*stride+6] = 
							v[4*stride+7] = 255;
						}  
						#endif
					}
			}

			if (!useDC) { /* use the default filter */

				v = &(image[y*stride + Bx])- 5*stride;
			
				deblock_vert_default_filter(v, stride, QP);

				#ifdef SHOWDECISIONS_V
				if (!chromaFlag) {
					v[4*stride  ] = 
					v[4*stride+1] = 
					v[4*stride+2] = 
					v[4*stride+3] = 
					v[4*stride+4] = 
					v[4*stride+5] = 
					v[4*stride+6] = 
					v[4*stride+7] = 0;
				}  
				#endif
			}
#endif /* MPEG4IP */
		} 
	}
}







/* this is the deringing filter */
static void dering( uint8_t *image, int width, int height, int stride, QP_STORE_T *QP_store, int QP_stride, int chroma) {
	int x, y, h, v, i, j;
	uint8_t *b8x8, *b10x10;
	uint8_t b8x8filtered[64];
	int QP, max_diff;
	uint8_t min, max, thr, range;
	uint16_t indicesP[10];  /* bitwise array of binary indices above threshold */
	uint16_t indicesN[10];  /* bitwise array of binary indices below threshold */
	uint16_t indices3x3[8]; /* bitwise array of pixels where we should filter */
	uint16_t sr;
	
	/* loop over all the 8x8 blocks in the image... */
	/* don't process outer row of blocks for the time being. */
	for (y=8; y<height-8; y+=8) {
		for (x=8; x< width-8; x+=8) {
		
			/* QP for this block.. */
			QP = chroma ? QP_store[y/8*QP_stride+x/8]
			            : QP_store[y/16*QP_stride+x/16];	
	
			/* pointer to the top left pixel in 8x8   block */
			b8x8   = &(image[stride*y + x]);
			/* pointer to the top left pixel in 10x10 block */
			b10x10 = &(image[stride*(y-1) + (x-1)]);
			
			/* Threshold detirmination - find min and max grey levels in the block */
			min = 255; max = 0;
			for (v=0; v<8; v++) {
				for (h=0; h<8; h++) {
					min = b8x8[stride*v + h] < min ? b8x8[stride*v + h] : min;				
					max = b8x8[stride*v + h] > max ? b8x8[stride*v + h] : max;				
				}
			} 
			/* Threshold detirmination - compute threshold and dynamic range */
			thr = (max + min + 1) / 2;
			range = max - min;
			
			/* Threshold rearrangement not implemented yet */
			
			/* Index aquisition */
			for (j=0; j<10; j++) {
				indicesP[j] = 0;
				for (i=0; i<10; i++) {
					if (b10x10[j*stride+i] >= thr) indicesP[j] |= (2 << i);
				}
				indicesN[j] = ~indicesP[j];			
			}
			
			/* Adaptive filtering */
			/* need to identify 3x3 blocks of '1's in indicesP and indicesN */
			for (j=0; j<10; j++) {
				indicesP[j] = (indicesP[j]<<1) & indicesP[j] & (indicesP[j]>>1);				
				indicesN[j] = (indicesN[j]<<1) & indicesN[j] & (indicesN[j]>>1);				
			}			
			for (j=1; j<9; j++) {
				indices3x3[j-1]  = indicesP[j-1] & indicesP[j] & indicesP[j+1];				
				indices3x3[j-1] |= indicesN[j-1] & indicesN[j] & indicesN[j+1];				
			}			

			for (v=0; v<8; v++) {
				sr = 4;
				for (h=0; h<8; h++) {
					if (indices3x3[v] & sr) {
						b8x8filtered[8*v + h] = ( 8
						 + 1 * b10x10[stride*(v+0) + (h+0)] + 2 * b10x10[stride*(v+0) + (h+1)] + 1 * b10x10[stride*(v+0) + (h+2)]
						 + 2 * b10x10[stride*(v+1) + (h+0)] + 4 * b10x10[stride*(v+1) + (h+1)] + 2 * b10x10[stride*(v+1) + (h+2)]
						 + 1 * b10x10[stride*(v+2) + (h+0)] + 2 * b10x10[stride*(v+2) + (h+1)] + 1 * b10x10[stride*(v+2) + (h+2)]
						) / 16;
					}
					sr <<= 1;
				}
			}
			
			/* Clipping */
			max_diff = QP/2;
			for (v=0; v<8; v++) {
				sr = 4;
				for (h=0; h<8; h++) {
					if (indices3x3[v] & sr) {
						if        (b8x8filtered[8*v + h] - b8x8[stride*v + h] >  max_diff) {
							b8x8[stride*v + h] = b8x8[stride*v + h] + max_diff;
						} else if (b8x8filtered[8*v + h] - b8x8[stride*v + h] < -max_diff) {
							b8x8[stride*v + h] = b8x8[stride*v + h] - max_diff;	
						} else {
							b8x8[stride*v + h] = b8x8filtered[8*v + h];
						}  								
					}
					sr <<= 1;
				}
			}

		}
	}


}





/* This function is more or less what Andrea wanted: */
void postprocess(unsigned char * src[], int src_stride,
                 unsigned char * dst[], int dst_stride, 
                 int horizontal_size,   int vertical_size, 
                 QP_STORE_T *QP_store,  int QP_stride,
					  int mode) {
					  
	uint8_t *Y, *U, *V;
	int x, y;

	if (!(mode & PP_DONT_COPY)) {
		/* First copy source to destination... */
		/* luma */
		for (y=0; y<vertical_size; y++) {
			for (x=0; x<horizontal_size; x++) {
				(dst[0])[y*dst_stride + x] = (src[0])[y*src_stride + x];
			}
		}
		/* chroma */
		for (y=0; y<vertical_size/2; y++) {
			for (x=0; x<horizontal_size/2; x++) {
				(dst[1])[y*dst_stride/2 + x] = (src[1])[y*src_stride/2 + x];
				(dst[2])[y*dst_stride/2 + x] = (src[2])[y*src_stride/2 + x];
			}
		}
	}
					  
	Y = dst[0];
	U = dst[1];
	V = dst[2];
	
	if (mode & PP_DEBLOCK_Y_H) {
		deblock_horiz(Y, horizontal_size,   vertical_size,   dst_stride, QP_store, QP_stride, 0);
	}
	if (mode & PP_DEBLOCK_Y_V) {
		deblock_vert( Y, horizontal_size,   vertical_size,   dst_stride, QP_store, QP_stride, 0);
	}
	if (mode & PP_DEBLOCK_C_H) {
		deblock_horiz(U, horizontal_size/2, vertical_size/2, dst_stride/2, QP_store, QP_stride, 1);
		deblock_horiz(V, horizontal_size/2, vertical_size/2, dst_stride/2, QP_store, QP_stride, 1);
	}
	if (mode & PP_DEBLOCK_C_V) {
		deblock_vert( U, horizontal_size/2, vertical_size/2, dst_stride/2, QP_store, QP_stride, 1);
		deblock_vert( V, horizontal_size/2, vertical_size/2, dst_stride/2, QP_store, QP_stride, 1);
	}
	if (mode & PP_DERING_Y) {
		dering(       Y, horizontal_size,   vertical_size,   dst_stride, QP_store, QP_stride, 0);
	}
	if (mode & PP_DERING_C) {
		dering(       U, horizontal_size/2, vertical_size/2, dst_stride/2, QP_store, QP_stride, 1);
		dering(       V, horizontal_size/2, vertical_size/2, dst_stride/2, QP_store, QP_stride, 1);
	}

}








