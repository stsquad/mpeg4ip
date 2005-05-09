/*************************************************************************

This software module was originally developed by 
	Stefan Rauthenberg (rauthenberg@HHI.DE), HHI
	(date: January, 1998)

in the course of development of the MPEG-4 Video (ISO/IEC 14496-2). 
This software module is an implementation of a part of one or more MPEG-4 Video tools 
as specified by the MPEG-4 Video. 
ISO/IEC gives users of the MPEG-4 Video free license to this software module or modifications 
thereof for use in hardware or software products claiming conformance to the MPEG-4 Video. 
Those intending to use this software module in hardware or software products are advised that its use may infringe existing patents. 
The original developer of this software module and his/her company, 
the subsequent editors and their companies, 
and ISO/IEC have no liability for use of this software module or modifications thereof in an implementation. 
Copyright is not released for non MPEG-4 Video conforming products. 
Microsoft retains full right to use the code for his/her own purpose, 
assign or donate the code to a third party and to inhibit third parties from using the code for non <MPEG standard> conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1998.

Module Name:

	sadct.cpp

Abstract:

	SADCT and inverse SADCT

Revision History:
  
*************************************************************************/


#include "typeapi.h"
#include "dct.hpp"
#include <math.h>
#if defined(__DEBUG_SADCT_) && !defined(NDEBUG)
#include <iostream.h>
#endif
#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

CSADCT::CSADCT():
	m_N(8),
	m_mat_tmp1(0),
	m_row_buf(0),
	m_ly(0),
	m_lx(0),
	m_mask(0)
{

	allocMatrix(&m_mat_tmp1, m_N, m_N);
	
	m_row_buf = new Float[m_N];
	m_ly = new Int[m_N];
	m_lx = new Int[m_N];
	
	allocMatrix(&m_mask, m_N, m_N);
	
	allocMatrix(&m_in, m_N, m_N);
	allocMatrix(&m_out, m_N, m_N);
	
// Schueuer HHI: added for fast_sadct
#ifdef _FAST_SADCT_
	c_buf = new Float[m_N];
	allocMatrix(&tmp_out, m_N, m_N);


	f0_2 = 0.707107;
	f0_3 = 0.577350;
	f1_3 = 0.707107;
	f2_3 = 0.408248;
	f3_3 = 0.816497;
	f0_4 = 0.500000;
	f1_4 = 0.653281;
	f2_4 = 0.270598;
	f0_5 = 0.447214;
	f1_5 = 0.601501;
	f2_5 = 0.371748;
	f3_5 = 0.511667;
	f4_5 = 0.195440;
	f5_5 = 0.632456;
	f0_6 = 0.408248;
	f1_6 = 0.557678;
	f2_6 = 0.500000; 
	f3_6 = 0.288675;
	f4_6 = 0.149429;
	f5_6 = 0.577350;
	f0_7 = 0.377964;
	f1_7 = 0.521121;
	f2_7 = 0.481588; 
	f3_7 = 0.417907;
	f4_7 = 0.333269;
	f5_7 = 0.231921;
	f6_7 = 0.118942;
	f7_7 = 0.534522;
	f0_8 = 0.7071068;
	f1_8 = 0.4903926;
	f2_8 = 0.4619398;
	f3_8 = 0.4157348;
	f4_8 = 0.3535534;
	f5_8 = 0.2777851;
	f6_8 = 0.1913417;
	f7_8 = 0.0975452;


	sq[0] = 0.00000000000;
	sq[1] = 1.00000000000;
	sq[2] =	1.41421356237;
	sq[3] =	1.73205080757;
	sq[4] =	2.00000000000;
	sq[5] =	2.23606797750;
	sq[6] =	2.44948974278;
	sq[7] =	2.64575131106;
	sq[8] =	2.82842712475;
#endif
}

CSADCT::~CSADCT()
{
	freeMatrix(m_mat_tmp1, m_N);

	delete [] m_row_buf;
	delete [] m_ly;
	delete [] m_lx;

	freeMatrix(m_mask, m_N);
	
	freeMatrix(m_in, m_N);
	freeMatrix(m_out, m_N);

// Schueuer HHI 
#ifdef _FAST_SADCT_
	delete [] c_buf;
	freeMatrix(tmp_out, m_N);
#endif

}

Void CSADCT::allocMatrix(Float ***mat, Int nr, Int nc) 
{
	Float **m = new Float* [nr];
	m[0] = new Float[nr*nc];
	
	for (Int i=1; i<nr; i++) {
		m[i] = m[i-1] + nc;
	}
	*mat = m;
}

Void CSADCT::allocMatrix(PixelC ***mat, Int nr, Int nc) 
{
	PixelC **m = new PixelC* [nr];
	m[0] = new PixelC[nr*nc];
	
	for (Int i=1; i<nr; i++) {
		m[i] = m[i-1] + nc;
	}
	*mat = m;
}

Float ***CSADCT::allocDctTable(Int n) 
{
	Float ***tbl = new Float** [n+1];
	tbl[0] = 0;		// transformation length of zero is impossible
	
	
	for (Int i=1; i<=n; i++) {
		allocMatrix(&tbl[i], n, n);
	}
	
	return (tbl);
}

Float ***CInvSADCT::allocReorderTable(Int n)
{
	Float ***tbl = new Float** [n];
	
	for (Int i=0; i<n; i++) {
		tbl[i] = new Float* [n];
		// to play it safe and get an exception if trying to access
		// an uninitialized poInter.
		memset(tbl[i], 0, sizeof(Float*)*n);
	}
	return tbl;
}

Void CInvSADCT::freeReorderTable(Float ***tbl, Int n)
{
	
	if (tbl) {
		for (Int i=0; i<n; i++)
			delete [] tbl[i];
		delete [] tbl;
	}
}



// parameter nr not used yet because the data elements are allocated in contiguous memory and 
// in separate rows.
Void CSADCT::freeMatrix(Float **mat, Int nr) 
{
	if (mat) {
		delete [] mat[0];
		delete [] mat;
	}
}

Void CSADCT::freeMatrix(PixelC **mat, Int nr) 
{
	if (mat) {
		delete [] mat[0];
		delete [] mat;
	}
}

Void CSADCT::freeDctTable(Float ***tbl, Int n) 
{
	if (tbl) {
		for (Int i=1; i<=n; i++) {
			freeMatrix(tbl[i], n);
		}
 		delete [] tbl;
	}
}

Void CSADCT::getRowLength(Int *lx, const PixelC* ppxlcMask, Int iStride)
{
	prepareMask(ppxlcMask, iStride);
	getRowLengthInternal(lx, m_mask, m_N, m_N);
}

/*
 *	returns in `lx' the number of active pels per line after shifting
 *	the pels towards the upper left corner.  This function may be
 *	called by a decoder in order to find out how the pels of a partial
 *	block are arranged for transmission.  
 */
Void CSADCT::getRowLengthInternal(Int *lx, PixelC **mask, Int bky, Int bkx)
{
	Int iy_out = 0;
	Int iy, ix, l;
	
	for (ix=0; ix<bkx; ix++) {
		l = 0;
		for (iy=0; iy<bky; iy++) {
			if ( mask[iy][ix] ) 
				l++;
		}
		if ( l ) 
			m_ly[iy_out++] = l;
	}
	
	for (ix=iy_out; ix<bkx; ix++) 
		m_ly[ix] = 0;
	
	for (iy=0; iy<bky; iy++) {
		l = 0;
		for (ix=0; ix<bkx; ix++) {
			if ( m_ly[ix] > iy )
				l++;
		}
		lx[iy] = l;
	}	
}

Void CSADCT::prepareMask(const PixelC* rgchMask, Int stride)
{
	for (Int iy=0; iy<m_N; iy++) {
    	PixelC* dstPtr = m_mask[iy];
        const PixelC* srcPtr = rgchMask;
        for (Int ix=0; ix<m_N; ix++) 
        	*dstPtr++ = (*srcPtr++ != transpValue) ? 1 : 0;
                    
		rgchMask += stride;
	}
}

Void CSADCT::prepareInputBlock(Float **matDst, const Int *rgiSrc, Int stride)
{
	Float *rowDstPtr;
	const Int *rowSrcPtr;
	
	for (Int iy=0; iy<m_N; iy++) {
		rowDstPtr = matDst[iy];
		rowSrcPtr = rgiSrc + iy*stride;
		for (Int ix=0; ix<m_N; ix++) {
			*rowDstPtr++ = *rowSrcPtr++;
		}
	}
}

Void CSADCT::prepareInputBlock(Float **matDst, const PixelC *rgiSrc, Int stride)
{
	Float *rowDstPtr;
	const PixelC *rowSrcPtr;

	for (Int iy=0; iy<m_N; iy++) {
		rowDstPtr = matDst[iy];
		rowSrcPtr = rgiSrc + iy*stride;
		for (Int ix=0; ix<m_N; ix++) {
			*rowDstPtr++ = *rowSrcPtr++;
		}
	}
}


CFwdSADCT::CFwdSADCT(UInt nBits) :
	CFwdBlockDCT(nBits)
{
	m_dct_matrix = allocDctTable(m_N);
	initTrfTables();
}

CFwdSADCT::~CFwdSADCT()
{
	freeDctTable(m_dct_matrix, m_N);
}

Void CFwdSADCT::initTrfTables(Float scale)
{
	Float **mat, a, factcos;
	Int u, x;
	Int n;
	
	for (n=1; n<=m_N; n++) {
		mat = m_dct_matrix[n];
		factcos = M_PI/(2*n);
		a = scale * sqrt(2.0 / n);   
		for (u=0; u<n; u++) {
			for (x=0; x<n; x++) {
				mat[u][x] = a * cos(factcos*u*(2*x+1));
				if ( u == 0 )
					mat[u][x] /= M_SQRT2;
			}
		}
	}
}

// shape adaptive dct for inter coded blocks.
Void CFwdSADCT::apply(const Int* rgiSrc, Int nColSrc, Int* rgiDst, Int nColDst, const PixelC* rgchMask, Int nColMask, Int *lx)
{
	if (rgchMask) {
		prepareMask(rgchMask, nColMask);
		prepareInputBlock(m_in, rgiSrc, nColSrc);

// Schueuer HHI: added for fast_sadct
#ifdef _FAST_SADCT_
		fast_transform(m_out, lx, m_in, m_mask, m_N, m_N);
#else
		transform(m_out, lx, m_in, m_mask, m_N, m_N);
#endif

		copyBack(rgiDst, nColDst, m_out, lx);
	}
	else
		CBlockDCT::apply(rgiSrc, nColSrc, rgiDst, nColDst, NULL, 0, NULL);
}

// inverse shape adaptive dct for intra coded blocks.
Void CFwdSADCT::apply (const PixelC* rgchSrc, Int nColSrc, Int* rgiDst, Int nColDst, const PixelC* rgchMask, Int nColMask, Int *lx)
{
	if (rgchMask) {
		prepareMask(rgchMask, nColMask);
		prepareInputBlock(m_in, rgchSrc, nColSrc);


// HHI Schueuer: inserted for fastsadct
#ifdef _FAST_SADCT_
		fast_deltaDCTransform(m_out, lx, m_in, m_mask, m_N, m_N);	
#else
		deltaDCTransform(m_out, lx, m_in, m_mask, m_N, m_N);
#endif
		// dirty hack for the AC/DC prediction: The transparent pixels of the
        // first row and columns must be cleared acc. to the sadct proposal.
        memset(rgiDst, 0, m_N*sizeof(PixelI));
        PixelI* rgiDstPtr = rgiDst+nColDst;
        for (int i = 1; i < m_N; i++ ) {
        	*rgiDstPtr = 0;
            rgiDstPtr += nColDst;
        }       

		copyBack(rgiDst, nColDst, m_out, lx);
	}
	else 
		CBlockDCT::apply(rgchSrc, nColSrc, rgiDst, nColDst, NULL, 0, NULL);
}

Void CFwdSADCT::copyBack(PixelI *rgiDst, Int nColDst, Float **in, Int *lx)
{
	Float *rowSrcPtr;
	PixelI *rowDstPtr;
	int i, j;
	
	for (i=0; i<m_N && lx[i]; i++) {
		rowSrcPtr = in[i];
		rowDstPtr = rgiDst;
		for (j = 0; j < lx[i]; j++) { 
			*rowDstPtr = (*rowSrcPtr < 0) ? (PixelI)(*rowSrcPtr - .5) :
			(PixelI)(*rowSrcPtr + .5);
			rowDstPtr++;
			rowSrcPtr++;
		}
		rgiDst += nColDst;
	}
}

Void CFwdSADCT::deltaDCTransform(Float **out, Int *lx, Float **in, PixelC **mask, Int bky, Int bkx)
{
	Int i, j;
	Float mean_value;
	Int active_pels;

	out[0][0] = 0.0;
	
	/* compute meanvalue */
	mean_value = 0.0;
	active_pels  = 0;
	for (i = 0; i < bky; i++) {
		for (j = 0; j < bkx; j++) {
			active_pels+= mask[i][j];
			mean_value += in[i][j] * mask[i][j];
		}
	}
	
	if (active_pels)
		mean_value = mean_value / (Float)active_pels;
	mean_value = mean_value + 0.5;
	mean_value = (int)mean_value;
	
	for (i = 0; i < bky; i++) {
		for (j = 0; j < bkx; j++) {
			in[i][j] -= mean_value;
		}
	}
	
 	transform(out, lx, in, mask, bky, bkx);
	
	/* copy meanvalue to DC-coefficient */
	out[0][0] = mean_value * 8.0;
	
}

// HHI Schueuer: inserted for fast sadct
#ifdef _FAST_SADCT_
Void CFwdSADCT::fast_deltaDCTransform(Float **out, Int *lx, Float **in, PixelC **mask, Int bky, Int bkx)
{
	Int i, j, jmax, k;
	Float mean_value;
	Float *row, *row_coeff;
	Int active_pels;

	out[0][0] = 0.0;
	
	/* compute meanvalue */
	// mean_value = 0.0;
	// active_pels  = 0;

	fastshiftupTranspose(m_mat_tmp1, m_ly, in, mask, &mean_value, &active_pels, bky, bkx);

	if (active_pels) {
		mean_value /= (Float) active_pels;
		if (mean_value > 0)
			mean_value = (Int) (mean_value + 0.5);
		else
			mean_value = (Int) (mean_value - 0.5);
	}

	memset(lx, 0, sizeof(Int)*bky);
	
	for (i=0; i<bkx && m_ly[i]; i++) {
		jmax = m_ly[i];
		row = m_mat_tmp1[i];
		for (j = 0; j < jmax; j++)
			row[j] -= mean_value;
		switch (jmax) {
		case 1:
			c_buf[0] = row[0];
			break;      
		case 2:
			dct_vec2 (row, c_buf);
			break;
		case 3:
			dct_vec3 (row, c_buf);
			break;
		case 4:
			dct_vec4 (row, c_buf);
			break;      
		case 5:
			dct_vec5 (row, c_buf);
			break;
		case 6:
			dct_vec6 (row, c_buf);
			break;
		case 7:
			dct_vec7 (row, c_buf);
			break;
		case 8:
			dct_vec8 (row, c_buf);
			break;
	}      
	for (k=0; k<jmax; k++) {
      tmp_out[k][lx[k]] = c_buf[k];
      lx[k]++;
    }
  }
	
	/* and finally the horizontal transformation */
	for (i=0; i<bky && lx[i]; i++) {
		jmax = lx[i];
		row = out[i];
		row_coeff = tmp_out[i];
		switch (jmax) {
		case 1:
			*row = row_coeff[0];
			break;      
		case 2:
			dct_vec2 (row_coeff, row);
		 break;
		case 3:
			dct_vec3 (row_coeff, row);
			break;
		case 4:
			dct_vec4 (row_coeff, row);
			break;      
		case 5:
			dct_vec5 (row_coeff, row);
			break;
		case 6:
			dct_vec6 (row_coeff, row);
			break;
		case 7:
			dct_vec7 (row_coeff, row);
			break;
		case 8:
			dct_vec8 (row_coeff, row);
			break;
		}
	}
 
	

	
	/* copy meanvalue to DC-coefficient */
	out[0][0] = mean_value * 8.0;
	
}
#endif

Void CFwdSADCT::transform(Float **out, Int *lx, Float **in, PixelC **mask, Int bky, Int bkx)
{
	Int i, j, jmax, k;
	Float **trf_mat, *row;
	Float c;
  
	shiftupTranspose(m_mat_tmp1, m_ly, in, mask, bky, bkx);
	
	memset(lx, 0, sizeof(Int)*bky);
	
	for (i=0; i<bkx && m_ly[i]; i++) {
		jmax = m_ly[i];
		trf_mat = m_dct_matrix[jmax];
		row = m_mat_tmp1[i];
		for (k=0; k<jmax; k++) {
			for (c=0,j=0; j<jmax; j++) 
				c += trf_mat[k][j] * row[j];
			out[k][lx[k]] = c;
			lx[k]++;
		}
	}
	
	/* and finally the horizontal transformation */
	for (i=0; i<bky && lx[i]; i++) {
		jmax = lx[i];
		trf_mat = m_dct_matrix[jmax];
		memcpy(m_row_buf, out[i], jmax*sizeof(Float));
		row = out[i];
		for (k=0; k<jmax; k++) {
			for (c=0,j=0; j<jmax; j++) 
				c += trf_mat[k][j] * m_row_buf[j];
			*row++ = c;
		}    
	}

}

// Schueuer HHI : added for fast_sadct
#ifdef _FAST_SADCT_
Void CFwdSADCT::fast_transform(Float **out, Int *lx, Float **in, PixelC **mask, Int bky, Int bkx)
{
	Int i, jmax, k;
	Double *row, *row_coeff;
  
	shiftupTranspose(m_mat_tmp1, m_ly, in, mask, bky, bkx);
	
	memset(lx, 0, sizeof(Int)*bky);
	
	for (i=0; i<bkx && m_ly[i]; i++) {
		jmax = m_ly[i];
		row = m_mat_tmp1[i];
	 switch (jmax) {
	    case 1:
			c_buf[0] = row[0];
			break;      
		case 2:
			dct_vec2 (row, c_buf);
			break;
		case 3:
			dct_vec3 (row, c_buf);
			break;
		case 4:
			dct_vec4 (row, c_buf);
			break;      
		case 5:
			dct_vec5 (row, c_buf);
			break;
		case 6:
			dct_vec6 (row, c_buf);
			break;
		case 7:
			dct_vec7 (row, c_buf);
			break;
		case 8:
			dct_vec8 (row, c_buf);
			break;
	}      
	for (k=0; k<jmax; k++) {
      tmp_out[k][lx[k]] = c_buf[k];
      lx[k]++;
    }
  }
	
	/* and finally the horizontal transformation */
	for (i=0; i<bky && lx[i]; i++) {
		jmax = lx[i];
		row = out[i];
		row_coeff = tmp_out[i];
		switch (jmax) {
		case 1:
			*row = row_coeff[0];
			break;      
		case 2:
			dct_vec2 (row_coeff, row);
		 break;
		case 3:
			dct_vec3 (row_coeff, row);
			break;
		case 4:
			dct_vec4 (row_coeff, row);
			break;      
		case 5:
			dct_vec5 (row_coeff, row);
			break;
		case 6:
			dct_vec6 (row_coeff, row);
			break;
		case 7:
			dct_vec7 (row_coeff, row);
			break;
		case 8:
			dct_vec8 (row_coeff, row);
			break;
		}
	}

}
#endif

//  HHI Schueuer: inserted for fast sadct
#ifdef _FAST_SADCT_
Void CFwdSADCT::fastshiftupTranspose(Float **out, Int *ly, Float **in, PixelC **mask, 
							Float *mean, Int *active_pels, Int bky, Int bkx)
{
  Int iy_out = 0, ix_out;
  Int iy, ix, l;

  *mean = 0.0;
  *active_pels = 0;

  for (ix = 0; ix < bkx; ix++) {
    ix_out = l = 0;
    for (iy = 0; iy < bky; iy++) {
      if ( mask[iy][ix] ) {
		out[iy_out][ix_out++] = in[iy][ix];
		*mean += in[iy][ix];
		l++;
      }
    }
    if ( l ) { 
      ly[iy_out++] = l;
      *active_pels += l;
    }
  }	
  /* initialize the length of the unoccupied columns to zero. The term 
     column refers to the pel positions in `in'.  In `out' columns are
     saved as rows (transposition) to speed up calculation. */
  for (ix=iy_out; ix<bkx; ix++) 
    ly[ix] = 0;

}
#endif

/*
 *	`sadct_shiftup_transpose' shifts upwards pels marked in `mask' towards
 *	the top margin of the rectangular block.  If a column consists of more
 *	than one stretch of pels, the gaps between these stretches will be
 *	eleminated.  The resulting columns are transposed and type converted
 *	into `out' and are occupying the upper part of the block without
 *	any gaps in vertical direction.
 *
 */
Void CFwdSADCT::shiftupTranspose(Float **out, Int *ly, Float **in, PixelC **mask, Int bky, Int bkx)
{
	Int iy_out = 0, ix_out;
	Int iy, ix, l;
	
	for (ix=0; ix<bkx; ix++) {
		ix_out = l = 0;
		for (iy=0; iy<bky; iy++) {
			if ( mask[iy][ix] ) {
				out[iy_out][ix_out++] = in[iy][ix];
				l++;
			}
		}
		if ( l ) 
			ly[iy_out++] = l;
	}
	
	/* initialize the length of the unoccupied columns to zero. The term 
	column refers to the pel positions in `in'.  In `out' columns are
	saved as rows (transposition) to speed up calculation. */
	for (ix=iy_out; ix<bkx; ix++) 
		ly[ix] = 0;
	
}

// Schueuer HHI : added for fast_sadct
#ifdef _FAST_SADCT_
Int CFwdSADCT::dct_vec2 (Double *vec, Double *coeff)
{

  coeff[0] = (vec[0] + vec[1]) * f0_2;
  coeff[1] = (vec[0] - vec[1]) * f0_2;

  return 0;
}

Int CFwdSADCT::dct_vec3 (Double *vec, Double *coeff)
{
  Double b;

  b = vec[0] + vec[2];

  coeff[0] = (vec[1] + b) * f0_3;
  coeff[1] = (vec[0] - vec[2]) * f1_3;
  coeff[2] = b * f2_3 -  vec[1] * f3_3;

  return 0;
}

Int CFwdSADCT::dct_vec4 (Double *vec, Double *coeff)
{
  Double b[4];

  /* stage 1 */
  b[0] = vec[0] + vec[3];
  b[1] = vec[1] + vec[2];
  b[2] = vec[1] - vec[2];
  b[3] = vec[0] - vec[3];

  /* stage 2 */
  coeff[0] = (b[0] + b[1]) * f0_4;
  coeff[2] = (b[0] - b[1]) * f0_4;
  coeff[1] = b[2] * f2_4 + b[3] * f1_4;
  coeff[3] = b[3] * f2_4 - b[2] * f1_4;

  return 0;
}

Int CFwdSADCT::dct_vec5 (Double *vec, Double *coeff)
{
  Double b[5];

  b[0] = vec[0] + vec[4];
  b[1] = vec[0] - vec[4];
  b[2] = vec[1] + vec[3];
  b[3] = vec[1] - vec[3];
  b[4] = vec[2] * f5_5;

  coeff[0] = (b[0] + b[2] + vec[2]) * f0_5;
  coeff[1] = b[1] * f1_5 + b[3] * f2_5;
  coeff[2] = b[0] * f3_5 - b[2] * f4_5 - b[4];
  coeff[3] = b[1] * f2_5 - b[3] * f1_5;
  coeff[4] = b[0] * f4_5 - b[2] * f3_5 + b[4];

  return 0;
}

Int CFwdSADCT::dct_vec6 (Double *vec, Double *coeff)
{
  Double b[8];

  /* stage 1 */
  b[0] = vec[0] + vec[5];
  b[1] = vec[0] - vec[5];
  b[2] = vec[1] + vec[4];
  b[3] = vec[1] - vec[4];
  b[4] = vec[2] + vec[3];
  b[5] = vec[2] - vec[3];
  b[6] = b[3] * f0_6;
  b[7] = b[0] + b[4];

  /* stage 2 */
  coeff[0] = (b[7] + b[2]) * f0_6;
  coeff[1] = b[1] * f1_6 + b[5] * f4_6 + b[6];
  coeff[2] = (b[0] - b[4]) * f2_6;
  coeff[3] = (b[1] - b[3] -b[5]) * f0_6;
  coeff[4] = b[7] * f3_6 - b[2] * f5_6;
  coeff[5] = b[1] * f4_6 + b[5] * f1_6 - b[6];

  return 0;
}

Int CFwdSADCT::dct_vec7 (Double *vec, Double *coeff)
{
  Double b[7];

  b[0] = vec[0] + vec[6];
  b[1] = vec[0] - vec[6];
  b[2] = vec[1] + vec[5];
  b[3] = vec[1] - vec[5];
  b[4] = vec[2] + vec[4];
  b[5] = vec[2] - vec[4];
  b[6] = vec[3] * f7_7;

  coeff[0] = (b[0] + b[2] + b[4] + vec[3]) * f0_7;
  coeff[1] = b[1] * f1_7 + b[3] * f3_7 + b[5] * f5_7;
  coeff[2] = b[0] * f2_7 + b[2] * f6_7 - b[4] * f4_7 - b[6];
  coeff[3] = b[1] * f3_7 - b[3] * f5_7 - b[5] * f1_7;
  coeff[4] = b[0] * f4_7 - b[2] * f2_7 - b[4] * f6_7 + b[6];
  coeff[5] = b[1] * f5_7 - b[3] * f1_7 + b[5] * f3_7;
  coeff[6] = b[0] * f6_7 - b[2] * f4_7 + b[4] * f2_7 - b[6];
  
  return 0;
}

Int CFwdSADCT::dct_vec8 (Double *vec, Double *coeff)
{
  Int   j1, j;
  Double b[8];
  Double b1[8];

  /* stage 1 */
  for (j = 0; j < 4; j++)
    {
      j1 = 7 - j;
      b1[j] = vec[j] + vec[j1];
      b1[j1] = vec[j] - vec[j1];
    }

  /* stage 2 */
  b[0] = b1[0] + b1[3];
  b[1] = b1[1] + b1[2];
  b[2] = b1[1] - b1[2];
  b[3] = b1[0] - b1[3];
  b[4] = b1[4];
  b[5] = (b1[6] - b1[5]) * f0_8;
  b[6] = (b1[6] + b1[5]) * f0_8;
  b[7] = b1[7];

  /* stage 3/4 for the coeff. 0,2,4,6 */
  coeff[0] = (b[0] + b[1]) * f4_8;
  coeff[4] = (b[0] - b[1]) * f4_8;
  coeff[2] = b[2] * f6_8 + b[3] * f2_8;
  coeff[6] = b[3] * f6_8 - b[2] * f2_8;

  /* stage 3 */
  b1[4] = b[4] + b[5];
  b1[7] = b[7] + b[6];
  b1[5] = b[4] - b[5];
  b1[6] = b[7] - b[6];

  /* stage 4 for coeff. 1,3,5,7 */
  coeff[1] = b1[4] * f7_8 + b1[7] * f1_8;
  coeff[5] = b1[5] * f3_8 + b1[6] * f5_8;
  coeff[7] = b1[7] * f7_8 - b1[4] * f1_8;
  coeff[3] = b1[6] * f3_8 - b1[5] * f5_8;
    
  return 0;
}
#endif


CInvSADCT::CInvSADCT(UInt nBits) :
	CInvBlockDCT(nBits)
{
	m_reorder_h = allocReorderTable(m_N);
	m_reorder_v = allocReorderTable(m_N);
	m_idct_matrix = allocDctTable(m_N);

	initTrfTables();
}

CInvSADCT::~CInvSADCT()
{
	freeDctTable(m_idct_matrix, m_N);
	freeReorderTable(m_reorder_h, m_N);
	freeReorderTable(m_reorder_v, m_N);
}

Void CInvSADCT::initTrfTables(Float scale)
{
	Float **mat, factcos, a;
	Int u, x;
	Int n;
	
	for (n=1; n<=m_N; n++) {
		mat = m_idct_matrix[n];
		factcos = M_PI/(2*n);
		a = scale * sqrt(2.0 / n);
		for (x=0; x<n; x++) {
			for (u=0; u<n; u++) {
				mat[x][u] = a * cos(factcos*u*(2*x+1));
				if ( u == 0 )
					mat[x][u] /= M_SQRT2;
			}
		}
	}
}

// inverse shape adaptive dct for Intra coded blocks.
Void CInvSADCT::apply (const Int* rgiSrc, Int nColSrc, PixelC* rgchDst, Int nColDst, const PixelC* rgchMask, Int nColMask)
{
	if (rgchMask) { 
		prepareMask(rgchMask, nColMask);
		prepareInputBlock(m_in, rgiSrc, nColSrc);


//  HHI Schueuer: inserted for sadct
#ifdef _FAST_SADCT_
		fast_deltaDCTransform(m_out, m_in, m_mask, m_N, m_N);
#else
		deltaDCTransform(m_out, m_in, m_mask, m_N, m_N);
#endif
        
        // dirty hack for the AC/DC prediction: The transparent pixels of the
        // first row and columns must be cleared acc. to the sadct proposal.
        memset(rgchDst, 0, m_N*sizeof(PixelC));
        PixelC* rgchDstPtr = rgchDst+nColDst;
#ifdef __TRACE_DECODING_
		// This complete clean is for tracing/debugging only, because I don't want to see memory garbage in the trace file.
        for (int i=1; i<m_N; i++ ) {
        	memset(rgchDstPtr, 0, m_N*sizeof(PixelC));
            rgchDstPtr += nColDst;
        }
#else
        for (int i=1; i<m_N; i++ ) {
        	*rgchDstPtr = 0;
            rgchDstPtr += nColDst;
        }
#endif        
		copyBack(rgchDst, nColDst, m_out, m_mask);	
	}
	else 
		CBlockDCT::apply(rgiSrc, nColSrc, rgchDst, nColDst);
}

// inverse shape adative dct for inter coded blocks.  
Void CInvSADCT::apply (const PixelI* rgiSrc, Int nColSrc, PixelI* rgiDst, Int nColDst, const PixelC* rgchMask, Int nColMask)
{
	if (rgchMask) {
		// boundary block assumed, for details refer to the comment
		// inside the body of the other apply method.
		prepareMask(rgchMask, nColMask);
		prepareInputBlock(m_in, rgiSrc, nColSrc);

// Schueuer HHI: added for fast_sadct
#ifdef _FAST_SADCT_
		fast_transform(m_out, m_in, m_mask, m_N, m_N);
#else
 		transform(m_out, m_in, m_mask, m_N, m_N);
#endif

        // dirty hack for the AC/DC prediction: The transparent pixels of the
        // first row and columns must be cleared acc. to the sadct proposal.
        memset(rgiDst, 0, m_N*sizeof(PixelI));
        PixelI* rgiDstPtr = rgiDst+nColDst;
#ifdef __TRACE_DECODING_
		// This complete clean out is for tracing/debugging only, because I don't want to see memory garbage in the trace file.
        for (int i=1; i<m_N; i++ ) {
        	memset(rgiDstPtr, 0, m_N*sizeof(PixelI));
            rgiDstPtr += nColDst;
        }
#else
        for (int i=1; i<m_N; i++ ) {
        	*rgiDstPtr = 0;
            rgiDstPtr += nColDst;
        }
#endif       
		copyBack(rgiDst, nColDst, m_out, m_mask);
	}
	else 
		CBlockDCT::apply(rgiSrc, nColSrc, rgiDst, nColDst);
}


/*
 *	Inverse shape adaptive transformation of block `in'.  The spatial positions
 *	of valid pels are marked in `mask' by 1.  Please note that the
 *	dct coefficients encoding those pels are expected to be found in 
 *	the upper left corner of block `in'.
 *	  
 *
 *	The following drawing explains the relation between `in', `out'
 *	and `mask':
 *
 *	in -> I I I - - - - - 
 *	 	  I I - - - - - -
 *		  I - - - - - - -
 *		  - - ...
 *				                 out ->    - - - - O - - -         
 *	mask ->   - - - - 1 - - -         	   - - O O - - - -
 *		      - - 1 1 - - - -		       - - O O - - - -
 *		      - - 1 1 - - - -		       - - - O - - - -
 *		      - - - 1 - - - -		       - - - - - - - -
 *		      - - - - - - - -		       - - ...
 *		      - - ...
 *
 */
Void CInvSADCT::deltaDCTransform(Float **out, Float **in, PixelC **mask, Int bky, Int bkx)
{
	Int i, j, k, l;

	Float mean_value, check_sum = 0.0;
	Float ly_sum = 0.0, ddx = 0.0;

#if 1
	/* reconstruction of meanvalue and zero setting of ddc */
	mean_value = (Int) ((in[0][0] / 8.0) + 0.5);
	in[0][0] = 0.0;
#endif	

	// Klaas Schueuer
	for (i = 0; i < 8; i++) 
		for (j = 0; j < 8; j++) 
			out [i][j] = 0.0;
	// end 

#if defined(__DEBUG_SADCT_) && !defined(NDEBUG)
	cout << "deltaDCTransform(): mean_value=" << mean_value << endl;
#endif

 	transform(out, in, mask, bky, bkx);
	
	/* computing of checksum and ddc correction */
	check_sum = 0;
	for (i = 0; i < bky; i++) {
		for (j = 0; j < bkx; j++)
			if (mask[i][j])
				check_sum += out[i][j];
	}	
	
	for (i=0; i<bkx; i++) {
		if (m_ly[i])
			ly_sum += sqrt ((Float)m_ly[i]);
	}
#if defined(__DEBUG_SADCT_) && !defined(NDEBUG)
	cout << "chksum=" << check_sum << "  ly_sum=" << ly_sum << '\n';
#endif
#if 1	
	k = 0;
	for (i = 0; i < bkx; i++) {
		l = 0;
		for (j = 0; j < bky; j++) {
			if (mask[j][i]) {
				if (l==0) {
					k++;
					l++;
					if (check_sum>0)  
						ddx = (Int) ((1.0 / (sqrt ((Float) m_ly[k-1]) * ly_sum) * check_sum ) + 0.5);
					else
						ddx = (Int) ((1.0 / (sqrt ((Float) m_ly[k-1]) * ly_sum) * check_sum ) - 0.5);
				}
#if defined(__DEBUG_SADCT_) && !defined(NDEBUG)
				cout << "delta correction: out[" << j << "][" << i << "]=" 
					 << out[j][i] << " ddx=" << ddx << '\n';
#endif
				out[j][i] += mean_value - ddx;
			}
		}
	}
#endif	

}


// HHI Schueuer: inserted for fast sadct
#ifdef _FAST_SADCT_
Void CInvSADCT::fast_deltaDCTransform(Float **out, Float **in, PixelC **mask, Int bky, Int bkx)
{
	Int i, k, jmax;
	Float **dest;
	Float *in_ptr;


	Float mean_value;
	Float e1 = 0.0, e2 = 0.0, e_dc = 0.0;

	mean_value = in[0][0] / 8.0;
	in[0][0] = 0.0;

	build_v_reorder_tbl(m_ly, out, mask, bky, bkx);
	
	build_h_reorder_tbl(m_lx, m_ly, m_mat_tmp1, bky, bkx);

	/* inverse horizontal transformation */
	for (i=0; i<bky && m_lx[i]; i++) {
		jmax = m_lx[i];
		in_ptr = in[i];
		
		dest = m_reorder_h[i];
		switch (jmax) {
		case 1:
			c_buf[0] = in_ptr[0];
			break;
		case 2:
			 idct_vec2 (in_ptr, c_buf);
			break;
		case 3:
			idct_vec3 (in_ptr, c_buf);
			break;      
		case 4:
			idct_vec4 (in_ptr, c_buf);
			break;
		case 5:
			idct_vec5 (in_ptr, c_buf);
			break;
		case 6:
			idct_vec6 (in_ptr, c_buf);
			break;      
		case 7:
			idct_vec7 (in_ptr, c_buf);
			break;
		case 8:
			idct_vec8 (in_ptr, c_buf);
			break;
		}
		if ( i == 0) {
			for (k = 0; k < jmax; k++) {
				e1 += sq[m_ly[k]] * c_buf[k];
				e2 += sq[m_ly[k]];
			}
			e_dc = e1 / e2;
			for (k=0; k<jmax; k++) 
				*dest[k] = c_buf[k] - e_dc;
		}
		else {
			for (k=0; k<jmax; k++)
				*dest[k] = c_buf[k];
		}
	}
 
	/* inverse vertical transformation */
	for (i=0; i<bkx && m_ly[i]; i++) {
		jmax = m_ly[i];
		in_ptr = m_mat_tmp1[i];
		
		dest = m_reorder_v[i];
		switch (jmax) {
		case 1:
			c_buf[0] = in_ptr[0];
			break;
		case 2:
			idct_vec2 (in_ptr, c_buf);
			break;
		case 3:
			idct_vec3 (in_ptr, c_buf);
			break;      
		case 4:
			idct_vec4 (in_ptr, c_buf);
			break;
		case 5:
			idct_vec5 (in_ptr, c_buf);
			break;
		case 6:
			idct_vec6 (in_ptr, c_buf);
			break;      
		case 7:
			idct_vec7 (in_ptr, c_buf);
			break;
		case 8:
			idct_vec8 (in_ptr, c_buf);
			break;
		}
		for (k=0; k<jmax; k++) 
			*dest[k] = c_buf[k] + mean_value;   
	}

}
#endif


/*
 *	inverse sadct transformation of block `in'.  The spatial positions
 *	of valid pels are marked in `mask' by 1.  Please note that the
 *	dct coefficients encoding those pels are expected to be found in 
 *	the upper left corner of block `in'.
 *	  
 *
 *	The following drawing explains the relation between `in', `out'
 *	and `mask':
 *
 *	in ->     I I I - - - - - 
 *	 	      I I - - - - - -
 *		      I - - - - - - -
 *		      - - ...
 *				                out ->    - - - - O - - -         
 *	mask ->   - - - - 1 - - -         	  - - O O - - - -
 *		      - - 1 1 - - - -		      - - O O - - - -
 *		      - - 1 1 - - - -		      - - - O - - - -
 *		      - - - 1 - - - -		      - - - - - - - -
 *		      - - - - - - - -		      - - ...
 *		      - - ...
 *
 */
Void CInvSADCT::transform(Float **out, Float **in, PixelC **mask, Int bky, Int bkx)
{
	Int i, j, k, jmax;
	Float **trf_mat, **dest;
	Float c;
	Float *in_ptr;

	build_v_reorder_tbl(m_ly, out, mask, bky, bkx);
	
	build_h_reorder_tbl(m_lx, m_ly, m_mat_tmp1, bky, bkx);

	/* inverse horizontal transformation */
	for (i=0; i<bky && m_lx[i]; i++) {
		jmax = m_lx[i];
		trf_mat = m_idct_matrix[jmax];
		in_ptr = in[i];
		
		dest = m_reorder_h[i];
		for (k=0; k<jmax; k++) {
			for (c=0,j=0; j<jmax; j++) 
				c += trf_mat[k][j] * in_ptr[j];
			*dest[k] = c;
		}    
	}
	
	/* inverse vertical transformation */
	for (i=0; i<bkx && m_ly[i]; i++) {
		jmax = m_ly[i];
		trf_mat = m_idct_matrix[jmax];
		in_ptr = m_mat_tmp1[i];
		
		dest = m_reorder_v[i];
		for (k=0; k<jmax; k++) {
			for (c=0,j=0; j<jmax; j++) 
				c += trf_mat[k][j] * in_ptr[j];
			*dest[k] = c;
		}    
	}
}

// HHI Schueuer: added for fast_sadct
#ifdef _FAST_SADCT_
Void CInvSADCT::fast_transform(Float **out, Float **in, PixelC **mask, Int bky, Int bkx)
{
	Int i, k, jmax;
	Float **dest;
	Float *in_ptr;

	build_v_reorder_tbl(m_ly, out, mask, bky, bkx);
	
	build_h_reorder_tbl(m_lx, m_ly, m_mat_tmp1, bky, bkx);

	/* inverse horizontal transformation */
	for (i=0; i<bky && m_lx[i]; i++) {
		jmax = m_lx[i];
		in_ptr = in[i];
		
		dest = m_reorder_h[i];
		switch (jmax) {
		case 1:
			c_buf[0] = in_ptr[0];
			break;
		case 2:
			 idct_vec2 (in_ptr, c_buf);
			break;
		case 3:
			idct_vec3 (in_ptr, c_buf);
			break;      
		case 4:
			idct_vec4 (in_ptr, c_buf);
			break;
		case 5:
			idct_vec5 (in_ptr, c_buf);
			break;
		case 6:
			idct_vec6 (in_ptr, c_buf);
			break;      
		case 7:
			idct_vec7 (in_ptr, c_buf);
			break;
		case 8:
			idct_vec8 (in_ptr, c_buf);
			break;
		}
		for (k=0; k<jmax; k++)
			*dest[k] = c_buf[k];    
	}
 
	/* inverse vertical transformation */
	for (i=0; i<bkx && m_ly[i]; i++) {
		jmax = m_ly[i];
		in_ptr = m_mat_tmp1[i];
		
		dest = m_reorder_v[i];
		switch (jmax) {
		case 1:
			c_buf[0] = in_ptr[0];
			break;
		case 2:
			idct_vec2 (in_ptr, c_buf);
			break;
		case 3:
			idct_vec3 (in_ptr, c_buf);
			break;      
		case 4:
			idct_vec4 (in_ptr, c_buf);
			break;
		case 5:
			idct_vec5 (in_ptr, c_buf);
			break;
		case 6:
			idct_vec6 (in_ptr, c_buf);
			break;      
		case 7:
			idct_vec7 (in_ptr, c_buf);
			break;
		case 8:
			idct_vec8 (in_ptr, c_buf);
			break;
		}
		for (k=0; k<jmax; k++)
			*dest[k] = c_buf[k];     
	}
}
#endif

// Schueuer HHI: added fast_sadct
#ifdef _FAST_SADCT_
Int CInvSADCT::idct_vec2 (Double *in, Double *out)
{

  out[0] = (in[0] + in[1]) * f0_2;
  out[1] = (in[0] - in[1]) * f0_2;

  return 0;
}

Int CInvSADCT::idct_vec3 (Double *in, Double *out)
{
  Double b[3];

  b[0] = in[0] * f0_3;
  b[1] = b[0] + in[2] * f2_3;
  b[2] = in[1] * f1_3;

  out[0] = b[1] + b[2];
  out[2] = b[1] - b[2];
  out[1] = b[0] - in[2] * f3_3;

  return 0;
}

Int CInvSADCT::idct_vec4 (Double *in, Double *out)
{
  Double b[4];

  /* stage 1 */

  b[0] = (in[0] + in[2]) * f0_4;
  b[1] = (in[0] - in[2]) * f0_4;
  b[2] = in[1] * f2_4 - in[3] * f1_4;
  b[3] = in[3] * f2_4 + in[1] * f1_4;

  /* stage 2 */
  out[0] = b[0] + b[3];
  out[1] = b[1] + b[2];
  out[2] = b[1] - b[2];
  out[3] = b[0] - b[3];

  return 0;
}

Int CInvSADCT::idct_vec5 (Double *in, Double *out)
{
  Double b[5];

  b[0] = in[0] * f0_5;
  b[1] = b[0] + in[2] * f3_5 + in[4] * f4_5;
  b[2] = in[1] * f1_5 + in[3] * f2_5;
  b[3] = b[0] - in[4] * f3_5 - in[2] * f4_5;
  b[4] = in[1] * f2_5 - in[3] * f1_5;

  out[0] = b[1] + b[2];
  out[1] = b[3] + b[4];
  out[2] = b[0] + (in[4] - in[2]) * f5_5;
  out[3] = b[3] - b[4];
  out[4] = b[1] - b[2];

  return 0;
}

Int CInvSADCT::idct_vec6 (Double *in, Double *out)
{
  Double b[10];
  Double b1[7];

  /* stage 1 */
  b[0] = in[0] * f0_6;
  b[1] = in[1] * f1_6;
  b[2] = in[2] * f2_6;
  b[3] = in[3] * f0_6;
  b[4] = in[4] * f3_6;
  b[5] = in[5] * f4_6;
  b[6] = (in[1] - in[5]) * f0_6;
  b[7] = in[1] * f4_6;
  b[8] = in[4] * f5_6;
  b[9] = in[5] * f1_6;

  /* stage 2 */
  b1[0] = b[0] + b[4];
  b1[1] = b1[0] + b[2];
  b1[2] = b[1] + b[3] + b[5];
  b1[3] = b[0] - b[8];
  b1[4] = b[6] - b[3];
  b1[5] = b1[0] - b[2];
  b1[6] = b[7] - b[3] + b[9];

  /* stage 3 */
  out[0] = b1[1] + b1[2];
  out[1] = b1[3] + b1[4];
  out[2] = b1[5] + b1[6];
  out[3] = b1[5] - b1[6];
  out[4] = b1[3] - b1[4];
  out[5] = b1[1] - b1[2];

  return 0;
}

Int CInvSADCT::idct_vec7 (Double *in, Double *out)
{
  Double b[20];
  Double b1[6];

  /* stage 1 */
  b[0] = in[0] * f0_7;
  b[1] = in[1] * f1_7;
  b[2] = in[1] * f3_7;
  b[3] = in[1] * f5_7;
  b[4] = in[2] * f2_7;
  b[5] = in[2] * f6_7;
  b[6] = in[2] * f4_7;
  b[7] = in[3] * f3_7;
  b[8] = in[3] * f5_7;
  b[9] = in[3] * f1_7;
  b[10] = in[4] * f4_7;
  b[11] = in[4] * f2_7;
  b[12] = in[4] * f6_7;
  b[13] = in[5] * f5_7;
  b[14] = in[5] * f1_7;
  b[15] = in[5] * f3_7;
  b[16] = in[6] * f6_7;
  b[17] = in[6] * f4_7;
  b[18] = in[6] * f2_7;
  b[19] = (in[4] - in[2] - in[6]) * f7_7;
  
  /* stage 2 */
  b1[0] = b[0] + b[4] + b[10] + b[16];
  b1[1] = b[1] + b[7] + b[13];
  b1[2] = b[0] + b[5] - b[11] - b[17];
  b1[3] = b[2] - b[8] - b[14];
  b1[4] = b[0] - b[6] - b[12] + b[18];
  b1[5] = b[3] - b[9] + b[15];

  /* stage 3 */

  out[0] = b1[0] + b1[1];
  out[1] = b1[2] + b1[3];
  out[2] = b1[4] + b1[5];
  out[3] = b[0] + b[19];
  out[4] = b1[4] - b1[5];
  out[5] = b1[2] - b1[3];
  out[6] = b1[0] - b1[1];  
  
  return 0;
}

Int CInvSADCT::idct_vec8 (Double *in, Double *out)
{

  Int j1, j;
  Double tmp[8], tmp1[8];
  Double e, f, g, h;

  /* stage 1 for k = 4,5,6,7 */
  e = in[1] * f7_8 - in[7] * f1_8;
  h = in[7] * f7_8 + in[1] * f1_8;
  f = in[5] * f3_8 - in[3] * f5_8;
  g = in[3] * f3_8 + in[5] * f5_8;

  /* stage 1+2 for k = 0,1,2,3 */
  tmp1[0] = (in[0] + in[4]) * f4_8;
  tmp1[1] = (in[0] - in[4]) * f4_8;
  tmp1[2] = in[2] * f6_8 - in[6] * f2_8;
  tmp1[3] = in[6] * f6_8 + in[2] * f2_8;

  tmp[4] = e + f;
  tmp1[5] = e - f;
  tmp1[6] = h - g;
  tmp[7] = h + g;
  
  tmp[5] = (tmp1[6] - tmp1[5]) * f0_8;
  tmp[6] = (tmp1[6] + tmp1[5]) * f0_8;
  tmp[0] = tmp1[0] + tmp1[3];
  tmp[1] = tmp1[1] + tmp1[2];
  tmp[2] = tmp1[1] - tmp1[2];
  tmp[3] = tmp1[0] - tmp1[3];

  /* stage 4 */
  for (j = 0; j < 4; j++) {
    j1 = 7 - j;
    out[j] = tmp[j] + tmp[j1];
    out[j1] = tmp[j] - tmp[j1];
  }
  
  return 0;
}
#endif

	
Void CInvSADCT::build_v_reorder_tbl(Int *l_y, Float **in, PixelC **mask, Int bky, Int bkx)
{
	Int iy_out = 0, ix_out;
	Int iy, ix, l;
	
	for (ix=0; ix<bkx; ix++) {
		ix_out = l = 0;
		for (iy=0; iy<bky; iy++) {
			if ( mask[iy][ix] ) {
				m_reorder_v[iy_out][ix_out++] = in[iy]+ix;
				l++;
			}
		}
		if ( l ) 
			l_y[iy_out++] = l;
	}
	
	/* initialize the length of the unoccupied columns resp. rows to zero. */
	for (ix=iy_out; ix<bkx; ix++) 
		l_y[ix] = 0;
	
}
 
Void CInvSADCT::build_h_reorder_tbl(Int *l_x, const Int *l_y, Float **in, Int bky, Int bkx)
{
	Int i, k, jmax;
	Int col_ind;
	Float *row;
	
	memset(l_x, 0, sizeof(Int)*bky);
	for (i=0; i<bkx && l_y[i]; i++) {
		jmax = l_y[i];
		row = in[i];
		for (k=0; k<jmax; k++) {
			col_ind = l_x[k];
			m_reorder_h[k][col_ind] = row + k;
			l_x[k]++;
		}
	}
}

Void CInvSADCT::copyBack(PixelI *rgiDst, Int nColDst, Float **in, PixelC **mask)
{
	Float *rowSrcPtr;
	PixelC *rowMaskPtr;
	PixelI *rowDstPtr;
	int i, j;
	
	for (i=0; i<m_N; i++) {
		rowSrcPtr = in[i];
		rowMaskPtr = mask[i];
		rowDstPtr = rgiDst;
		for (j = 0; j < m_N; j++) { 
			if (rowMaskPtr) 
				*rowDstPtr = (*rowSrcPtr < 0) ? (PixelI)(*rowSrcPtr - .5) :
			(PixelI)(*rowSrcPtr + .5);
			rowMaskPtr++;
			rowDstPtr++;
			rowSrcPtr++;
		}
		rgiDst += nColDst;
	}
}

Void CInvSADCT::copyBack(PixelC *rgchDst, Int nColDst, Float **in, PixelC **mask)
{
	Float *rowSrcPtr;
	PixelC *rowMaskPtr;
	PixelC *rowDstPtr;
	PixelI iValue;
	int i, j;
	
	for (i=0; i<m_N; i++) {
		rowSrcPtr = in[i];
		rowMaskPtr = mask[i];
		rowDstPtr = rgchDst;
		for (j = 0; j < m_N; j++) { 
			if (rowMaskPtr) {
				iValue = (*rowSrcPtr < 0) ? (PixelI)(*rowSrcPtr - .5) :
			(PixelI)(*rowSrcPtr + .5);
				*rowDstPtr = m_rgchClipTbl [iValue];
			}
			rowMaskPtr++;
			rowDstPtr++;
			rowSrcPtr++;
		}
		rgchDst += nColDst;
	}
}



