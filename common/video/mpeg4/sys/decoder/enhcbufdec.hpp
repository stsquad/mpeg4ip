/*************************************************************************

*************************************************************************/

// Buffers for Temporal Scalability	Added by Sharp(1998-02-10)
/******************************************
***     class CEnhcBufferDecoder        ***
******************************************/
class CEnhcBufferDecoder : public CEnhcBuffer
{
	friend class CVideoObjectDecoder;
	friend class CVideoObjectDecoderTPS;

	CEnhcBufferDecoder(Int iSessionWidth, Int iSessionHeight);

	Void copyBuf(const CEnhcBufferDecoder& srcBuf);

	Void getBuf(CVideoObjectDecoder* pvopc);		// get params from Base layer
	Void putBufToQ0(CVideoObjectDecoder* pvopc);		// store params to Enhancement layer
	Void putBufToQ1(CVideoObjectDecoder* pvopc);		// store params to Enhancement layer

	Void getBuf(CVideoObjectDecoderTPS* pvopc);		// get params from Base layer
	Void putBufToQ0(CVideoObjectDecoderTPS* pvopc);		// store params to Enhancement layer
	Void putBufToQ1(CVideoObjectDecoderTPS* pvopc);		// store params to Enhancement layer
};
