/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)

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

	svd.cpp

Abstract:

	Solution of Linear Algebraic Equations 

Revision History:

*************************************************************************/

#include <stdlib.h>
#include <math.h>
#include "basic.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

#define irowNull		(-1)

__inline static void SwapRow(Double *rgcoeff, Double *rgrhs, Int crow,
	Int irow1, Int irow2);
__inline static void EliminateColumn(Double *rgcoeff, Double *rgrhs, Int crow,
	Int irowPiv);
__inline static void BackSub(Double *rgcoeff, Double *rgrhs, Int crow);
__inline static Int RowPivot(Double *rgcoeff, Int crow, Int irowBeg);

Int FSolveLinEq(Double *rgcoeff, Double *rgrhs, Int crow)
	{
	Int irow;

	for (irow = 0; irow < crow; irow++)
		{
		Int irowPivot = RowPivot(rgcoeff, crow, irow);

		if (irowPivot == irowNull)
			return FALSE;
		
		SwapRow(rgcoeff, rgrhs, crow, irow, irowPivot);
		EliminateColumn(rgcoeff, rgrhs, crow, irow);
		}
	BackSub(rgcoeff, rgrhs, crow);
	return TRUE;
	}

// Assumes that columns till column irow1 have been eliminated from the 
// rows irow1 & irow2
__inline static void SwapRow(Double *rgcoeff, Double *rgrhs, Int crow,
	Int irow1, Int irow2)
	{
	Int icol;
	Double coeffT, rhsT;
	Double *pcoeffRow1 = &rgcoeff[crow * irow1];
	Double *pcoeffRow2 = &rgcoeff[crow * irow2];

	for (icol = irow1; icol < crow; icol++)
		{
		coeffT = pcoeffRow1[icol];

		pcoeffRow1[icol] = pcoeffRow2[icol];
		pcoeffRow2[icol] = coeffT;
		}

	rhsT = rgrhs[irow1];
	rgrhs[irow1] = rgrhs[irow2];
	rgrhs[irow2] = rhsT;
	}

__inline static void EliminateColumn(Double *rgcoeff, Double *rgrhs, Int crow,
	Int irowPiv)
	{
	Double *rgcoeffRowPiv = &rgcoeff[irowPiv * crow];
	Int irow;

	for (irow = irowPiv + 1; irow < crow; irow++)
		{
		Int icol;
		Double *rgcoeffRowCur = &rgcoeff[irow * crow];
		Double coeffMult;

		coeffMult = - (rgcoeffRowCur[irowPiv] / rgcoeffRowPiv[irowPiv]);
		for (icol = irowPiv + 1; icol < crow; icol++)
			rgcoeffRowCur[icol] += coeffMult * rgcoeffRowPiv[icol];
		rgrhs[irow] += coeffMult * rgrhs[irowPiv];
		}
	}

__inline static void BackSub(Double *rgcoeff, Double *rgrhs, Int crow)
	{
	Int irow;

	for (irow = crow - 1; irow >= 0; irow--)
		{
		Double *rgcoeffRow = &rgcoeff[irow * crow];
		Double rhsRow = rgrhs[irow];
		Int icol;

		for (icol = irow + 1; icol < crow; icol++)
			rhsRow -= rgcoeffRow[icol] * rgrhs[icol];

		rgrhs[irow] = rhsRow / rgcoeffRow[irow];
		}
	}

__inline static Int RowPivot(Double *rgcoeff, Int crow, Int irowBeg)
	{
	Int irow;
	Int irowPivot = irowBeg;
	Double coeffPivot = rgcoeff[irowBeg * crow + irowBeg];

	if (coeffPivot < 0.0f)
		coeffPivot = -coeffPivot;

	for (irow = irowBeg + 1; irow < crow; irow++)
		{
		Double coeffRow = rgcoeff[irow * crow + irowBeg];

		if (coeffRow < 0.0f)
			coeffRow = -coeffRow;
		if (coeffRow > coeffPivot)
			{
			coeffPivot = coeffRow;
			irowPivot = irow;
			}
		}
	if (coeffPivot == 0.0f)
		irowPivot = irowNull;
	return irowPivot;
	}

Double* linearLS (Double** Ain, Double* b, UInt n_row, UInt n_col)
{
	assert (n_row == n_col); // make sure of overdeterminancy

	Double* x = new Double [n_row + 1];
	Double* A = new Double [n_row * n_col];
	UInt count = 0;
	UInt i;
	for (i = 0; i < n_row; i++)
		for (UInt j = 0; j < n_col; j++)
			A[count++] = Ain[i][j];
	
	FSolveLinEq (A, b, n_row);
	for (i = 0; i < n_row; i++)	{
		x[i] = b[i];
	}
	delete [] A;
	x [n_row] = 1.0;
	return x;
}



