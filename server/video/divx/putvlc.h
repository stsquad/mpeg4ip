/***********************************************************HeaderBegin*******
 *                                                                         
 * File:	putvlc.h
 *
 * Description: Header file to include prototypes for vlc functions
 *
 ***********************************************************HeaderEnd*********/

/************************    INCLUDE FILES    ********************************/

#include "momusys.h"

#ifndef _PUTVLC_H_
#define _PUTVLC_H_

#define MARKER_BIT 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Int  	PutDCsize_lum _P_((	Int size,
			Image *bitstream
	));
Int  	PutDCsize_chrom _P_((	Int size,
			Image *bitstream
	));
Int  	PutMV _P_((	Int mvint,
			Image *bitstream
	));
Int  	PutMCBPC_Intra _P_((	Int cbpc,
			Int mode,
			Image *bitstream
	));
Int  	PutMCBPC_Inter _P_((	Int cbpc,
			Int mode,
			Image *bitstream
	));
Int  	PutMCBPC_Sprite _P_((	Int cbpc,
			Int mode,
			Image *bitstream
	));
Int  	PutCBPY _P_((	Int cbpy,
			Char intra,
			Int *MB_transp_pattern,
			Image *bitstream
	));
Int  	PutCoeff_Inter _P_((	Int run,
			Int level,
			Int last,
			Image *bitstream
	));
Int  	PutCoeff_Intra _P_((	Int run,
			Int level,
			Int last,
			Image *bitstream
	));
Int  	PutCoeff_Inter_RVLC _P_((	Int run,
			Int level,
			Int last,
			Image *bitstream
	));
Int  	PutCoeff_Intra_RVLC _P_((	Int run,
			Int level,
			Int last,
			Image *bitstream
	));
Int  	PutRunCoeff_Inter _P_((	Int run,
			Int level,
			Int last,
			Image *bitstream
	));
Int  	PutRunCoeff_Intra _P_((	Int run,
			Int level,
			Int last,
			Image *bitstream
	));
Int  	PutLevelCoeff_Inter _P_((	Int run,
			Int level,
			Int last,
			Image *bitstream
	));
Int  	PutLevelCoeff_Intra _P_((	Int run,
			Int level,
			Int last,
			Image *bitstream
	));

#ifdef __cplusplus
}
#endif /* __cplusplus  */ 

#endif /* _PUTVLC_H_ */

