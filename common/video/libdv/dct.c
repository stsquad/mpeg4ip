/* 
 *  dct.c
 *
 *     Copyright (C) Charles 'Buck' Krasic - April 2000
 *     Copyright (C) Erik Walthinsen - April 2000
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  libdv is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your
 *  option) any later version.
 *   
 *  libdv is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *  The libdv homepage is http://libdv.sourceforge.net/.  
 */

#include <dv_types.h>

#include <math.h>
#include <string.h>

#include "dct.h"
#include "weighting.h"

#if ARCH_X86
#include "mmx.h"
#endif /* ARCH_X86 */

typedef short var;

#if BRUTE_FORCE_DCT_248 || BRUTE_FORCE_DCT_88
static double KC248[8][4][4][8];
#endif /* BRUTE_FORCE_DCT_248 || BRUTE_FORCE_DCT_88 */

#if (!ARCH_X86) || BRUTE_FORCE_DCT_248 || BRUTE_FORCE_DCT_88
static double C[8];
static double KC88[8][8][8][8];
#endif /* (!ARCH_X86) || BRUTE_FORCE_DCT_248 || BRUTE_FORCE_DCT_88 */

#if ARCH_X86
void idct_block_mmx(gint16 *block);
void dct_88_block_mmx(gint16* block);
void dct_block_mmx_postscale_88(gint16* block, gint16* postscale_matrix);
void dct_block_mmx_postscale_248(gint16* block, gint16* postscale_matrix);
void dct_248_block_mmx(gint16* block);
void dct_248_block_mmx_post_sum(gint16* out_block);
void transpose_mmx(short * dst);
#endif /* ARCH_X86 */

extern dv_coeff_t postSC88[64] ALIGN32;
extern dv_coeff_t postSC248[64] ALIGN32;

void dct_init(void) {
#if BRUTE_FORCE_DCT_248 || BRUTE_FORCE_DCT_88
  int u, z;
#endif /* BRUTE_FORCE_DCT_248 || BRUTE_FORCE_DCT_88 */
#if (!ARCH_X86) || BRUTE_FORCE_DCT_248 || BRUTE_FORCE_DCT_88
  int x, y, h, v, i;
  for (x = 0; x < 8; x++) {
    for (y = 0; y < 8; y++) {
      for (v = 0; v < 8; v++) {
        for (h = 0; h < 8; h++) {
          KC88[x][y][h][v] =
            cos((M_PI * v * ((2.0 * y) + 1.0)) / 16.0) *
            cos((M_PI * h * ((2.0 * x) + 1.0)) / 16.0);
        }
      }
    }
  }
#endif /* (!ARCH_X86) || BRUTE_FORCE_DCT_248 || BRUTE_FORCE_DCT_88 */
#if BRUTE_FORCE_DCT_248 || BRUTE_FORCE_DCT_88
  for (x = 0; x < 8; x++) {
    for (z = 0; z < 4; z++) {
      for (u = 0; u < 4; u++) {
        for (h = 0; h < 8; h++) {
	  KC248[x][z][u][h] = 
	    cos((M_PI * u * ((2.0 * z) + 1.0)) / 8.0) *
	    cos((M_PI * h * ((2.0 * x) + 1.0)) / 16.0);
        }                       /* for h */
      }                         /* for u */
    }                           /* for z */
  }                             /* for x */
#endif /* BRUTE_FORCE_DCT_248 || BRUTE_FORCE_DCT_88 */
#if (!ARCH_X86) || BRUTE_FORCE_DCT_248 || BRUTE_FORCE_DCT_88
  for (i = 0; i < 8; i++) {
    C[i] = (i == 0 ? 0.5 / sqrt(2.0) : 0.5);
  } /* for i */
#endif /* (!ARCH_X86) || BRUTE_FORCE_DCT_248 || BRUTE_FORCE_DCT_88 */
}

#if 0
/* Optimized out using integer fixpoint */
/* for DCT */
#define A1 0.70711
#define A2 0.54120
#define A3 0.70711
#define A4 1.30658
#define A5 0.38268
#endif

static void dct88_aan_line(short* in, short* out)
{
	var v0, v1, v2, v3, v4, v5, v6, v7;
	var v00,v01,v02,v03,v04,v05,v06,v07;   
	var v10,v11,v12,v13,v14,v15,v16;   
	var v20,v21,v22;   
	var v32,v34,v35,v36;   
	var v42,v43,v45,v47;   
	var v54,v55,v56,v57,va0;   

	v0 = in[0];
	v1 = in[1];
	v2 = in[2];
	v3 = in[3];
	v4 = in[4];
	v5 = in[5];
	v6 = in[6];
	v7 = in[7];
	
	/* first butterfly stage  */
	v00 = v0+v7;	  /* 0 */
	v07 = v0-v7;	  /* 7 */
	v01 = v1+v6;	  /* 1 */
	v06 = v1-v6;	  /* 6 */
	v02 = v2+v5;	  /* 2 */
	v05 = v2-v5;	  /* 5 */
	v03 = v3+v4;	  /* 3 */
	v04 = v3-v4;	  /* 4 */
	
	/* second low butterfly */
	v10=v00+v03;	     /* 0 */
	v13=v00-v03;		 /* 3 */
	v11=v01+v02;		 /* 1 */
	v12=v01-v02;		 /* 2 */
	
	/* second high */
	v16=v06+v07;		 /* 6 */
	v15=v05+v06;		 /* 5 */
	v14=-(v04+v05);	 /* 4 */
	/* 7 v77 without change */
	
	/* third	(only 3 real new terms) */
	v20=v10+v11;			    /* 0 */
	v21=v10-v11;				/* 1 */
	v22=v12+v13;   			/* 2 */
#if 0
	va0=(v14+v16)*A5; /* temporary for A5 multiply */
#endif
	va0=((v14+v16)*25079) >> 16;	 /* temporary for A5 multiply */
	
	/* fourth */
#if 0
	v32=v22*A1;                           /* 2 */
	v34=-(v14*A2+va0);                    /* 4 ? */
	v36=v16*A4-va0;                       /* 6 ? */
	v35=v15*A3;                           /* 5 */
#endif
	
	v32=(v22*23171) >> 15;                /* 2 */
	v34=-(((v14*17734) >> 15)+va0);       /* 4 ? */
	v36=((v16*21407) >> 14)-va0;          /* 6 ? */
	v35=(v15*23171) >> 15;                /* 5 */
	
	/* fifth */
	v42=v32+v13;                    /* 2 */
	v43=v13-v32;                    /* 3 */
	v45=v07+v35;                    /* 5 */
	v47=v07-v35;                    /* 7 */
	
	/* last */
	v54=v34+v47;                         /* 4 */
	v57=v47-v34;                         /* 7 */
	v55=v45+v36;                         /* 5 */
	v56=v45-v36;                         /* 5 */
	
	/* output butterfly */
	
	out[0] = v20;
	out[1] = v55;
	out[2] = v42;
	out[3] = v57;
	out[4] = v21;
	out[5] = v54;
	out[6] = v43;
	out[7] = v56;
}


static void dct44_aan_line(short* in, short* out)
{
	var v00,v01,v02,v03;   
	var v10,v11,v12,v13;   
	var v20,v21,v22;   
	var v32;   
	var v42,v43;   

	/* first butterfly stage  */
	v00 = in[0];
	v01 = in[2];
	v02 = in[4];
	v03 = in[6];
	
	/* second low butterfly */
	v10=v00+v03;	     /* 0 */
	v13=v00-v03;		 /* 3 */
	v11=v01+v02;		 /* 1 */
	v12=v01-v02;		 /* 2 */
	
	/* third	(only 3 real new terms) */
	v20=v10+v11;			    /* 0 */
	v21=v10-v11;				/* 1 */
	v22=v12+v13;   			/* 2 */
	
	/* fourth */
	
	v32=(v22*23171) >> 15;                /* 2 */
	
	/* fifth */
	v42=v32+v13;                    /* 2 */
	v43=v13-v32;                    /* 3 */
	
	/* output butterfly */
	
	out[0] = v20;
	out[2] = v42;
	out[4] = v21;
	out[6] = v43;
}

void postscale88(var v[64])
{
	int i;
	int factor = pow(2, 16 + DCT_YUV_PRECISION);
	for( i = 0; i < 64; i++ ) {
		v[i] = ( v[i] * postSC88[i] ) / factor;
	}
}

/* Input has to be transposed ! */

static inline void dct88_aan(short *s_in)
{
	int i,j,r,c;
	short s_out[64];

	for(c = 0; c < 64; c += 8 ) /* columns */
		dct88_aan_line(s_in + c, s_in + c);

	for(i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			s_out[i * 8 + j] = s_in[j * 8 + i];
		}
	}
	
	for(r = 0; r < 64; r += 8 ) /* then rows */
		dct88_aan_line(s_out + r, s_out + r);

	memcpy(s_in, s_out, 64 * sizeof(short));
}

/* Input has to be transposed ! */

static inline void dct248_aan(short *s_in)
{
	int i,j,r,c;
	short s_out[64];

	for(c = 0; c < 64; c += 8 ) { /* columns */
		dct44_aan_line(s_in + c, s_in + c);
		dct44_aan_line(s_in + c + 1, s_in + c + 1);
	}

	for(i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			s_out[i * 8 + j] = s_in[j * 8 + i];
		}
	}

	for(r = 0; r < 64; r += 8 ) { /* then rows */
		dct88_aan_line(s_out + r, s_out + r);
	}

	for(i = 0; i < 4; i++) {
		for (j = 0; j < 8; j++) {
			s_in[i * 8 + j] = s_out[i * 16 + j]
				+ s_out[i * 16 + 8 + j];
			s_in[i * 8 + 32 + j] = s_out[i * 16 + j]
				- s_out[i * 16 + 8 + j];
		}
	}
}

void postscale248(var v[64])
{
	int i;
	int factor = pow(2, 16 + DCT_YUV_PRECISION);
	for( i=0; i<64; i++ ) {
		v[i] = ( v[i] * postSC248[i] ) / factor;
	}
}

/* Input has to be transposed !!! */

void dct_88(dv_coeff_t *block) {
#if !ARCH_X86
#if BRUTE_FORCE_DCT_88
  int v,h,y,x,i;
  double temp[64];
  int factor = pow(2, DCT_YUV_PRECISION);

  memset(temp,0,sizeof(temp));
  for (v = 0; v < 8; v++) {
    for (h = 0; h < 8; h++) {
      for (y = 0;y < 8; y++) {
	for (x = 0;x < 8; x++) {
	  temp[v * 8 + h] += block[x * 8 + y] 
	    * KC88[x][y][h][v];
	}
      }
      temp[v * 8 + h] *= (C[h] * C[v]);
    }
  }

  for (i = 0; i < 64; i++) {
    block[i] = temp[i] / factor;
  }
#else /* BRUTE_FORCE_DCT_88 */
  dct88_aan(block);
  postscale88(block);
#endif /* BRUTE_FORCE_DCT_88 */
#else /* ARCH_X86 */
  dct_88_block_mmx(block);
  transpose_mmx(block);
  dct_88_block_mmx(block);
  dct_block_mmx_postscale_88(block, postSC88);
  emms(); 
#endif /* ARCH_X86 */
}

/* Input has to be transposed !!! */

void dct_248(dv_coeff_t *block) 
{
#if !ARCH_X86
#if BRUTE_FORCE_DCT_248
  int u,h,z,x,i;
  double temp[64];
  int factor = pow(2, DCT_YUV_PRECISION);
	
  memset(temp,0,sizeof(temp));
  for (u=0;u<4;u++) {
    for (h=0;h<8;h++) {
      for (z=0;z<4;z++) {
	for (x=0;x<8;x++) {
	  temp[u*8+h] +=     (block[x*8+2*z] + block[x*8+(2*z+1)]) *
		  KC248[x][z][u][h];
	  temp[(u+4)*8+h] += (block[x*8+2*z] - block[x*8+(2*z+1)]) *
	    KC248[x][z][u][h];
	}
      }
      temp[u*8+h] *= (C[h] * C[u]);
      temp[(u+4)*8+h] *= (C[h] * C[u]);
    }
  }

  for (i=0;i<64;i++)
	  block[i] = temp[i] / factor;
#else /* BRUTE_FORCE_DCT_248 */
  dct248_aan(block);
  postscale248(block);
#endif /* BRUTE_FORCE_DCT_248 */
#else /* ARCH_X86 */
  dct_88_block_mmx(block);
  transpose_mmx(block);
  dct_248_block_mmx(block);
  dct_248_block_mmx_post_sum(block);
  dct_block_mmx_postscale_248(block, postSC248);
  emms();
#endif /* ARCH_X86 */
}

void idct_88(dv_coeff_t *block) 
{
#if !ARCH_X86
  int v,h,y,x,i;
  double temp[64];

  memset(temp,0,sizeof(temp));
  for (v=0;v<8;v++) {
    for (h=0;h<8;h++) {
      for (y=0;y<8;y++){ 
	for (x=0;x<8;x++) {
	  temp[y*8+x] += C[v] * C[h] * block[v*8+h] * KC88[x][y][h][v];
	}
      }
    }
  }
	
  for (i=0;i<64;i++)
    block[i] = temp[i];
#else
  idct_block_mmx(block);
  emms();
#endif
}

#if BRUTE_FORCE_248

void idct_248(double *block) 
{
  int u,h,z,x,i;
  double temp[64];
  double temp2[64];
  double b,c;
  double (*in)[8][8], (*out)[8][8]; /* don't really need storage 
				       -- fixup later */

#if 0
  /* This is to identify visually where 248 blocks are... */
  for(i=0;i<64;i++) {
    block[i] = 235 - 128;
  }
  return;
#endif

  memset(temp,0,sizeof(temp));

  out = &temp;
  in = &temp2;
  
  for(z=0;z<8;z++) {
    for(x=0;x<8;x++)
      (*in)[z][x] = block[z*8+x];
  }
	
  for (x = 0; x < 8; x++) {
    for (z = 0; z < 4; z++) {
      for (u = 0; u < 4; u++) {
	for (h = 0; h < 8; h++) {
	  b = (double)(*in)[u][h];  
	  c = (double)(*in)[u+4][h];
	  (*out)[2*z][x] += C[u] * C[h] * (b + c) * KC248[x][z][u][h];
	  (*out)[2*z+1][x] += C[u] * C[h] * (b - c) * KC248[x][z][u][h];
	}                       /* for h */
      }                         /* for u */
    }                           /* for z */
  }                             /* for x */

#if 0
  for (u=0;u<4;u++) {
    for (h=0;h<8;h++) {
      for (z=0;z<4;z++) {
        for (x=0;x<8;x++) {
          b = block[u*8+h];
          c = block[(u+4)*8+h];
          temp[(2*u)*8+h] += C[h] * C[u] * (b + c) * KC248[x][z][h][u];
          temp[(2*u+1)*8+h] += C[h] * C[u] * (b - c) * KC248[x][z][h][u];
        }
      }
    }
  }
#endif

  for(z=0;z<8;z++) {
    for(x=0;x<8;x++)
      block[z*8+x] = (*out)[z][x];
  }
}


#endif /* BRUTE_FORCE_248 */
