/*************************************************************************

This software module was originally developed by 

	Ming-Chieh Lee (mingcl@microsoft.com), Microsoft Corporation
	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	Bruce Lin (blin@microsoft.com), Microsoft Corporation
	Chuang Gu (chuanggu@microsoft.com), Microsoft Corporation
	(date: March, 1996)

and edited by
	Yoshihiro Kikuchi (TOSHIBA CORPORATION)
	Takeshi Nagai (TOSHIBA CORPORATION)
	Toshiaki Watanabe (TOSHIBA CORPORATION)
	Noboru Yamaguchi (TOSHIBA CORPORATION)
    Sehoon Son (shson@unitel.co.kr) Samsung AIT

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
	
	huffman.hpp

Abstract:

	Classes for Huffman encode/decode.

Revision History:

*************************************************************************/

#ifndef _HUFFMAN_HPP_
#define _HUFFMAN_HPP_

#define VM_HOME_DIR		"\\vm"

class istream;
class ostream;
class CInBitStream;
class COutBitStream;
class CEntropyEncoder;
class CEntropyDecoder;
class CNode;

class CHuffmanTree
{
public:
	// Constructors
    CHuffmanTree (Int lNOfSymbols, Int* lpFrequencies = NULL);//create tree and load with frequencies
    virtual ~CHuffmanTree ();//destroy tree

	// Operations
    Void setFrequencies (Int* lpFrequencies);//load frequencies of all symbols
    Void setFrequency (Int lFrequency, Int lIndex);//set frequency of one symbol
    Void buildTree ();//generate huffman table
    Void writeTable (ostream &stream);//write table into text stream
    Void writeTableSorted (ostream &stream);//write table sorted by frequencies into text stream

private:
    CNode* m_pNodes;
    Int m_lNOfSymbols;
    virtual Void writeSymbol (Int symbolNo,ostream &Stream);
    Void writeOneTableEntry (
		ostream &Stream,
		Int entryNo,
        Double lTotalFrequency,
		Double &dNOfBits
	);
    Void statistics (Int &lTotalFrequency, Double &dEntropy);
    Void printStatistics (Double dEntropy, Double dNOfBits, ostream &stream);
};

class CHuffmanCoDec
{
protected:
    virtual Int makeIndexFromSymbolInTable (istream &huffmanTable);
    Void profileTable (
		istream &huffmanTable,
		Int& lNOfSymbols, 
		Int& lMaxCodeSize
	);
    Bool processOneLine (
		istream &huffmanTable,
		Int& lSymbol,
		Int& lCodeSize,
		Char* pCode
	);
    Void trashRestOfLine (istream &str);
};

class CHuffmanDecoderNode;
class CHuffmanDecoder:public CHuffmanCoDec, public CEntropyDecoder
{
public:
	// Constructors
    CHuffmanDecoder () {};//create unattached decoder
    CHuffmanDecoder (CInBitStream *BitStream);//create huffman decoder and attach to bitStream
	CHuffmanDecoder (CInBitStream *BitStream, VlcTable *pVlc);
    ~CHuffmanDecoder ();//destroy decoder

	// Operations
    Void loadTable (istream &HuffmanTable,Bool bIncompleteTree=TRUE);//load huffman table from the text istream
	Void loadTable (VlcTable *pVlc,Bool bIncompleteTree=TRUE);//load huffman table from array
	Int decodeSymbol ();//return one decoded symbol from the attached CInBitStream
    void attachStream (CInBitStream *bitStream);//attach bitStream to decoder
	CInBitStream* bitstream () {return m_pBitStream;};//return attached CInBitSteam

private:
    Void realloc (Int lOldSize,Int lNewSize);
    CHuffmanDecoderNode* m_pTree;
    CInBitStream* m_pBitStream;
};

class CHuffmanEncoder:public CHuffmanCoDec, public CEntropyEncoder
{
public:
	// Constructors
    CHuffmanEncoder (COutBitStream &bitStream);//create and attach encoder
	CHuffmanEncoder (COutBitStream &BitStream, VlcTable *pVlc);
    ~CHuffmanEncoder ();//destroy encoder

	// Operations
    Void loadTable (istream &HuffmanTable);//load huffman table from the text istream
    Void loadTable (VlcTable *pVlc);//load huffman table from the text istream
    Void attachStream (COutBitStream &BitStream);//attach bitStream to the decoder
    UInt encodeSymbol (Int lSymbol, Char* rgchSymbolName = NULL, Bool bDontSendBits = FALSE);//encode one symbol and put into the attached COutBitStream
	COutBitStream* bitstream () {return m_pBitStream;};//return attached COutBitStream

private:
	Int m_lCodeTableEntrySize;
    Int* m_pCodeTable;
    Int* m_pSizeTable;
    COutBitStream* m_pBitStream;
};

class CEntropyEncoderSet{
public:
	// Constructors
    CEntropyEncoderSet () {;}						// default constructor; do nothing
    CEntropyEncoderSet (COutBitStream &bitStream);	// create and attach encoders
    ~CEntropyEncoderSet ();							// destroy encoders

	CEntropyEncoder* m_pentrencDCT;					// encoder for DCT
	CEntropyEncoder* m_pentrencDCTIntra;			// encoder for Y and A planes of Intra VOP/MB
	CEntropyEncoder* m_pentrencMV;					// encoder for MV
	CEntropyEncoder* m_pentrencMCBPCintra;			// encoder for MCBPC = intra 
	CEntropyEncoder* m_pentrencMCBPCinter;			// encoder for MCBPC = inter 
	CEntropyEncoder* m_pentrencCBPY;				// encoder for CBPY
	CEntropyEncoder* m_pentrencCBPY1;				// encoder for CBPY (1 non-transparent blk) 
	CEntropyEncoder* m_pentrencCBPY2;				// encoder for CBPY (2 non-transparent blk) 
	CEntropyEncoder* m_pentrencCBPY3;				// encoder for CBPY (3 non-transparent blk) 
	CEntropyEncoder* m_pentrencIntraDCy;			// encoder for IntraDC (mpeg2 mode only)
	CEntropyEncoder* m_pentrencIntraDCc;			// encoder for IntraDC (mpeg2 mode only)
	CEntropyEncoder* m_pentrencMbTypeBVOP;			// encoder for MBtype in B-VOP
	CEntropyEncoder* m_pentrencWrpPnt;				// encoder for sprite point coding
	CEntropyEncoder* m_ppentrencShapeMode[7];		// encoder for first MMR code
//OBSS_SAIT_991015
	CEntropyEncoder* m_ppentrencShapeSSModeInter[4];// encoder for shape SS mode  
	CEntropyEncoder* m_ppentrencShapeSSModeIntra[2];// encoder for shape SS mode  
//~OBSS_SAIT_991015
	CEntropyEncoder* m_pentrencShapeMV1;			// encoder for shape mv part 1
	CEntropyEncoder* m_pentrencShapeMV2;			// encoder for shape mv part 2
 	//	Added for error resilience mode By Toshiba(1998-1-16:DP+RVLC)
 	CEntropyEncoder* m_pentrencDCTRVLC;			
 	CEntropyEncoder* m_pentrencDCTIntraRVLC;		
 	//	End Toshiba(1998-1-16:DP+RVLC)
};

class CEntropyDecoderSet{
public:
	// Constructors
    CEntropyDecoderSet () {;}						// default constructor; do nothing
    CEntropyDecoderSet (CInBitStream *bitStream);	// create and attach encoders
    ~CEntropyDecoderSet ();							// destroy encoders

	CEntropyDecoder* m_pentrdecDCT;					// decoder for DCT
	CEntropyDecoder* m_pentrdecDCTIntra;			// decoder for Y and A planes of Intra VOP/MB
	CEntropyDecoder* m_pentrdecMV;					// decoder for MV
	CEntropyDecoder* m_pentrdecMCBPCintra;			// decoder for MCBPC = intra 
	CEntropyDecoder* m_pentrdecMCBPCinter;			// decoder for MCBPC = inter 
	CEntropyDecoder* m_pentrdecCBPY;				// decoder for CBPY 
	CEntropyDecoder* m_pentrdecCBPY1;				// decoder for CBPY (1 non-transparent blk) 
	CEntropyDecoder* m_pentrdecCBPY2;				// decoder for CBPY (2 non-transparent blk) 
	CEntropyDecoder* m_pentrdecCBPY3;				// decoder for CBPY (3 non-transparent blk)
	CEntropyDecoder* m_pentrdecIntraDCy;			// decoder for IntraDC (mpeg2 mode only)
	CEntropyDecoder* m_pentrdecIntraDCc;			// decoder for IntraDC (mpeg2 mode only)
	CEntropyDecoder* m_pentrdecMbTypeBVOP;			// decoder for MBtype in B-VOP
	CEntropyDecoder* m_pentrdecWrpPnt;
	CEntropyDecoder* m_ppentrdecShapeMode [7];		// decoder for first MMR code
//OBSS_SAIT_991015
	CEntropyDecoder* m_ppentrdecShapeSSModeInter [4];	// decoder for Shape SS mode BVOP 
	CEntropyDecoder* m_ppentrdecShapeSSModeIntra [2];	// decoder for Shape SS mode PVOP 
//~OBSS_SAIT_991015
	CEntropyDecoder* m_pentrdecShapeMV1;			// decoder for shape mv part 1
	CEntropyDecoder* m_pentrdecShapeMV2;			// decoder for shape mv part 2
 	//	Added for error resilience mode By Toshiba(1998-1-16:DP+RVLC)
 	CEntropyDecoder* m_pentrdecDCTRVLC;			
 	CEntropyDecoder* m_pentrdecDCTIntraRVLC;		
 	//	End Toshiba(1998-1-16:DP+RVLC)

};

#endif
