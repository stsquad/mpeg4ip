/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)
and edited by
        Wei Wu (weiwu@stallion.risc.rockwell.com) Rockwell Science Center
and also edited by
    Mathias Wien (wien@ient.rwth-aachen.de) RWTH Aachen / Robert BOSCH GmbH

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

Copyright (c) 1996, 1997.

Module Name:

	dct.hpp

Abstract:

	DCT and inverse DCT

Revision History:

*************************************************************************/


#ifndef __DCT_HPP_
#define __DCT_HPP_

// math constants, if not already defined (HHI, 2000-05-16)
#ifndef M_PI
  #define M_PI            3.14159265358979323846
#endif
#ifndef M_SQRT2
  #define M_SQRT2         1.41421356237309504880
#endif

class CTransform;

class CBlockDCT : public CTransform
{
public:
	// Constructors
	virtual ~CBlockDCT ();
/* NBIT: change
	CBlockDCT ();	
*/
	CBlockDCT (UInt nBits); // NBIT

	// Operations
	Void apply (const PixelC* rgiSrc, Int nColSrc, Int* rgiDst, Int nColDst);
	Void apply (const Int* rgiSrc, Int nColSrc, PixelC* rgchDst, Int nColDst);
	Void apply (const Int* rgiSrc, Int nColSrc, Int* rgchDst, Int nColDst);

	// HHI 09/14/99 support for the SADCT is
	// implemented by means of polymorphismn.  
	
	// Signature for the shape adaptive (i)dct for inter coded blocks.
	// If the VOL flag says sadct is disabled or for non-boundary blocks, 
	// the following parameters are meaningless and the block DCT will be called:
	// rgchMask, nColMask, lx
	// 
	// FYI: The vector lx contains the number of transform coefficients per row and
	//      will be needed to adapt the scan to the actual shape of the block.
	//		The decoder must call getRowLength() to obtain this information as
	//		the scan information is needed before the inverse transformation can
	//	    be performed. 
	virtual Void apply (const Int* rgiSrc, Int nColSrc, Int* rgiDst, Int nColDst, const PixelC* rgchMask, Int nColMask, Int *lx/* = NULL*/)
		{ apply(rgiSrc, nColSrc, rgiDst, nColDst); }

	// Signature for the shape adaptive dct for intra coded blocks.
	 Void apply (const PixelC* rgchSrc, Int nColSrc, Int* rgiDst, Int nColDst, const PixelC* rgchMask, Int nColMask, Int *lx)
		{ apply(rgchSrc, nColSrc, rgiDst, nColDst); }

	// Signature for the inverse shape adaptive dct for intra coded blocks.
	Void apply (const Int* rgiSrc, Int nColSrc, PixelC* rgchDst, Int nColDst, const PixelC* rgchMask, Int nColMask)
		{ apply(rgiSrc, nColSrc, rgchDst, nColDst); }

   	// end HHI 09/14/99
	

   	
   	
///////////////// implementation /////////////////
protected:

	UInt m_nBits; // NBIT
	// only used for 8x8 case
	/*	Float m_c0;	//mwi										//xformation costants
	Float m_c1;
	Float m_c2;
	Float m_c3;
	Float m_c4;
	Float m_c5;
	Float m_c6;
	Float m_c7; */
      	Double m_c0;											//xformation costants
	Double m_c1;
	Double m_c2;
	Double m_c3;
	Double m_c4;
	Double m_c5;
	Double m_c6;
	Double m_c7; 
	PixelC* m_rgchClipTbl;
	/*	Float	m_rgfltBuf1[8];	//mwi							//buffers
	Float 	m_rgfltBuf2[8];
	Float 	m_rgfltAfter1dXform[8];						//results of 1dDCT
	Float 	m_rgfltAfterRowXform[8][8];					//intermediate results */
	Double	m_rgfltBuf1[8];								//buffers
	Double 	m_rgfltBuf2[8];
	Double 	m_rgfltAfter1dXform[8];						//results of 1dDCT
	Double 	m_rgfltAfterRowXform[8][8];					//intermediate results

	Void xformRow (const PixelC* ppxlcRowSrc, CoordI i);
	Void xformRow (const PixelI* ppxlfRowSrc, CoordI i);//transform a row
	Void xformColumn (PixelC* ppxlcColDst, CoordI i, Int nColDst);
	Void xformColumn (PixelI* ppxlfColDst, CoordI i, Int nColDst);	//transfrom a col
	virtual Void oneDimensionalDCT () = 0;					//1d DCT

};

class CFwdBlockDCT: public CBlockDCT								//forward DCT
{
public:
	// Constructors
 	// HHI 09/14/99 declared the dtor virtual to have a polymorphic relationship
	// between SADCT and BlockDCT.
	virtual ~CFwdBlockDCT () {}	
	// ~CFwdBlockDCT () {}	
/* NBIT: change
	CFwdBlockDCT ();	
*/
	CFwdBlockDCT (UInt nBits=8);	
   	
///////////////// implementation /////////////////
protected:
	Void oneDimensionalDCT ();
};

class CInvBlockDCT: public CBlockDCT								//inverse DCT
{
public:
	// Constructors
	// HHI 09/14/99 declared the dtor virtual to have a polymorphic relationship
	// between SADCT and BlockDCT.	
	virtual ~CInvBlockDCT () {}
	// end HHI 09/14/99 
	// ~CInvBlockDCT () {}		
/* NBIT: change
	CInvBlockDCT ();	
*/
	CInvBlockDCT (UInt nBits=8);	
   	

///////////////// implementation /////////////////
protected:
	Void oneDimensionalDCT ();
};

// HHI 09/14/99 SADCT
// The following 
class CSADCT 
{
public:
	CSADCT();
	virtual ~CSADCT();
	
	/*
	 *	returns in `lx' the number of active pels per line after shifting
	 *	the pels towards the upper left corner.  This function may be
	 *	called by a decoder in order to find out how the pels of a partial
	 *	block are arranged for transmission.  
	 */
	Void getRowLength(Int *lx, const PixelC* ppxlcMask, Int stride);

protected:
	Int m_N;		// maximum block length (=8 for mpeg vm)
	
	// buffers to hold temporary results for the saidct/sadct

	Float **m_mat_tmp1;
	Float *m_row_buf;
	Int *m_ly, *m_lx;

	// The following buffers are part of an interface layer which
	// converts the MSVM blocks into 2d matrices as used by the 
	// (original) sadct code. 
	// 
	PixelC **m_mask;	
	Float **m_in;
	Float **m_out;

// Schueuer HHI: added for fast_sadct
#ifdef _FAST_SADCT_
	Float *c_buf;
	Float **tmp_out;

	Float f0_2;
	Float f0_3, f1_3, f2_3, f3_3;
	Float f0_4, f1_4, f2_4;
	Float f0_5, f1_5, f2_5, f3_5, f4_5, f5_5;
	Float f0_6, f1_6, f2_6, f3_6, f4_6, f5_6;
	Float f0_7, f1_7, f2_7, f3_7, f4_7, f5_7, f6_7, f7_7;
	Float f0_8, f1_8, f2_8, f3_8, f4_8, f5_8, f6_8, f7_8;

	Float sq[9];
#endif

	Void allocMatrix(Float ***mat, Int nr, Int nc);
    Void freeMatrix(Float **mat, Int nr); 
	Void allocMatrix(PixelC ***mat, Int nr, Int nc);
    Void freeMatrix(PixelC **mat, Int nr); 

	Float ***allocDctTable(Int n);
    Void freeDctTable(Float ***tbl, Int n); 
	
	
	// interface stuff to convert from VM representation to internal 
	// representation and vice versa. 
	Void prepareMask(const PixelC* rgchMask, Int stride);

	Void prepareInputBlock(Float **matDst, const Int *rgiSrc, Int stride);
	Void prepareInputBlock(Float **matDst, const PixelC *rgiSrc, Int stride);


private:
	Void getRowLengthInternal(Int *lx, PixelC **mask, int bky, int bkx);


};

class CFwdSADCT: public CSADCT, public CFwdBlockDCT
{
public:
	CFwdSADCT(UInt nBits=8);
	virtual ~CFwdSADCT();

	// shape adaptive dct for inter coded blocks.
	virtual Void apply (const Int* rgiSrc, Int nColSrc, Int* rgiDst, Int nColDst, const PixelC* rgchMask, Int nColMask, Int *lx);

	// shape adative dct for intra coded blocks.
	virtual Void apply (const PixelC* rgchSrc, Int nColSrc, Int* rgiDst, Int nColDst, const PixelC* rgchMask, Int nColMask, Int *lx);

protected:
	/* Indexing:	dct_matrix[n] is a pointer to a transformation matrix of
 	 *              blocksize `n'.  The sensible range of the first index is 
 	 *		        1..N AND not 0 .. N-1 as one might presume.
 	 */
	Float ***m_dct_matrix;
		
	Void initTrfTables(Float scale = 1.0); 

private:
	Void deltaDCTransform(Float **out, Int *lx, Float **in, PixelC **mask, Int bky, Int bkx);
	Void transform(Float **out, Int *lx, Float **in, PixelC **mask, Int bky, Int bkx);
// Schueuer HHI : added for fast_sadct
#ifdef _FAST_SADCT_
	Void fastshiftupTranspose(Float **out, Int *ly, Float **in, PixelC **mask, 
							Float *mean, Int *active_pels, Int bky, Int bkx);
	Void fast_deltaDCTransform(Float **out, Int *lx, Float **in, PixelC **mask, Int bky, Int bkx);
	Void fast_transform(Float **out, Int *lx, Float **in, PixelC **mask, Int bky, Int bkx);
	Int dct_vec2 (Double *vec, Double *coeff);
	Int dct_vec3 (Double *vec, Double *coeff);
	Int dct_vec4 (Double *vec, Double *coeff);
	Int dct_vec5 (Double *vec, Double *coeff);
	Int dct_vec6 (Double *vec, Double *coeff);
	Int dct_vec7 (Double *vec, Double *coeff);
	Int dct_vec8 (Double *vec, Double *coeff);
#endif

    Void shiftupTranspose(Float **mat, Int *ly, Float **in, PixelC **mask, Int bky, Int bkx);

	Void copyBack(PixelI *rgiDst, Int nColDst, Float **in, Int *lx);

};


class CInvSADCT: public CSADCT, public CInvBlockDCT
{
public:
	CInvSADCT(UInt nBits=8);
	virtual ~CInvSADCT();

	// inverse shape adaptive dct for intra coded blocks.
	virtual Void apply (const Int* rgiSrc, Int nColSrc, PixelC* rgchDst, Int nColDst, const PixelC* rgchMask, Int nColMask);
	
	// inverse shape adative dct for inter coded blocks.  
	virtual Void apply (const Int* rgiSrc, Int nColSrc, Int* rgiDst, Int nColDst, const PixelC* rgchMask, Int nColMask, Int *lx)
	{ apply(rgiSrc, nColSrc, rgiDst, nColDst, rgchMask, nColMask); }

	Void apply (const Int* rgiSrc, Int nColSrc, Int* rgiDst, Int nColDst, const PixelC* rgchMask, Int nColMask);

protected:
	/* Indexing:	idct_matrix[n] is a pointer to a transformation matrix of
 	 *              blocksize `n'.  The sensible range of the first index is 
 	 *		        1..N AND not 0 .. N-1 as one might presume.
 	 */
	Float ***m_idct_matrix;
	
	Float ***m_reorder_h;
	Float ***m_reorder_v;

	Float ***allocReorderTable(Int n);
	Void freeReorderTable(Float ***tbl, Int n);
		
	Void initTrfTables(Float scale = 1.0);
	

private:
	Void deltaDCTransform(Float **out, Float **in, PixelC **mask, Int bky, Int bkx);
	Void transform(Float **out, Float **in, PixelC **mask, Int bky, Int bkx);
// Schueuer HHI : added for fast_sadct
#ifdef _FAST_SADCT_
	Void fast_transform(Float **out, Float **in, PixelC **mask, Int bky, Int bkx);
	Void fast_deltaDCTransform(Float **out, Float **in, PixelC **mask, Int bky, Int bkx);
	Int idct_vec2 (Double *vec, Double *coeff);
	Int idct_vec3 (Double *vec, Double *coeff);
	Int idct_vec4 (Double *vec, Double *coeff);
	Int idct_vec5 (Double *vec, Double *coeff);
	Int idct_vec6 (Double *vec, Double *coeff);
	Int idct_vec7 (Double *vec, Double *coeff);
	Int idct_vec8 (Double *vec, Double *coeff);
#endif

	Void build_v_reorder_tbl(Int *l_y, Float **in, PixelC **mask, Int bky, Int bkx);
	Void build_h_reorder_tbl(Int *l_x, const Int *l_y, Float **in, Int bky, Int bkx);

	Void copyBack(PixelI *rgiDst, Int nColDst, Float **in, PixelC **mask);
	Void copyBack(PixelC *rgchDst, Int nColDst, Float **in, PixelC **mask);

};

// end HHI 09/14/99



#endif 
// __DCT_HPP_
