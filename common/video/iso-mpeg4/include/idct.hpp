/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See theL icense for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is idct.
 * 
 * The Initial Developer of the Original Code is Philips Electronics N.V.
 * Portions created by Philips Electronics N.V. are
 * Copyright (C) Philips Electronics N.V. 2002,2003.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Richard Chen        richard.chen@philips.com
 */

#ifndef _INT_IDCT_
#define _INT_IDCT_ 1


#define IDCT_BLOCK_SIZE	8
class idct{
public:

	static void init();
	
	template<class src, class dest>
	void IDCT(const src *psrc, int srcStride, dest *pdst, int destStride){
		readin(psrc, srcStride);
		idct2d();
		writeout(pdst, destStride);
	};

	template<class src, class dest>
	void IDCTClip(const src *psrc, int srcStride, dest *pdst, int destStride){
		readin(psrc, srcStride);
		idct2d();
		clipWriteout(pdst, destStride);
	};


protected:

	void idctrow(short *);
	void idctcol(short *);
	void idct2d();

	template<class src>
	void readin(const src *psrc, int stride){
		short *p=block;
		for(int row=0; row<IDCT_BLOCK_SIZE; row++, psrc+=stride, p+=IDCT_BLOCK_SIZE)
			for(int col=0; col< IDCT_BLOCK_SIZE; col++)
				p[col]=psrc[col];
	}

	template<class dest>
	void writeout(dest *pdst, int stride){
		short *p=block;
		for(int row=0; row<IDCT_BLOCK_SIZE; row++, pdst+=stride, p+=IDCT_BLOCK_SIZE)
			for(int col=0; col<IDCT_BLOCK_SIZE; col++)
				pdst[col]=p[col];
	}

	template<class dest>
	void clipWriteout(dest *pdst, int stride){
		short *p=block;
		for(int row=0; row<IDCT_BLOCK_SIZE; row++, pdst+=stride, p+=IDCT_BLOCK_SIZE)
			for(int col=0; col<IDCT_BLOCK_SIZE; col++)
				pdst[col]=clipping[p[col]];
	};

	short block[IDCT_BLOCK_SIZE*IDCT_BLOCK_SIZE];
	static unsigned char clipTable[];
	static unsigned char *clipping;
};

#endif
