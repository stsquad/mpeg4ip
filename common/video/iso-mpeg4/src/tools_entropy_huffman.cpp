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
	
	huffman.cpp

Abstract:

	Classes for Huffman encode/decode.

Revision History:

*************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <fstream.h>
#include <iostream.h>
#include "typeapi.h"
#include "entropy.hpp"
#include "huffman.hpp"
#include "iostream.h"
#include "fstream.h"
#include "math.h"
#include "bitstrm.hpp"

#include "vlc.hpp"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

#ifdef __PC_COMPILER_
#define FULLNAME(dir, filename) strcpy (pch, "\\"#dir"\\"#filename);
#else 
#define FULLNAME(dir, filename) strcpy (pch, "/"#dir"/"#filename);
#endif

class CNode
{
friend class CHuffmanTree;
friend Int huffmanNodeCompare (const Void* Element1,const Void* Element2);
private:
    Char m_cCode;
    Int m_lNodeIndex;
    Int m_lFrequency;
    Int m_lBalancer;
    CNode () {m_cCode = 0; m_lNodeIndex = -1; m_lFrequency = 0; m_lBalancer = 1;};
};

class CHuffmanDecoderNode
{
friend class CHuffmanDecoder;
    Char m_c0End;
    Char m_c1End;
    Int m_l0NextNodeOrSymbol;
    Int m_l1NextNodeOrSymbol;
    CHuffmanDecoderNode () {m_c0End=0;m_c1End=0;
            m_l0NextNodeOrSymbol=-1;m_l1NextNodeOrSymbol=-1;};
    Bool is0Valid() {if ((m_c0End==0)&&(m_l0NextNodeOrSymbol==-1))
            return FALSE;return TRUE;};
    Bool is1Valid() {if ((m_c1End==0)&&(m_l1NextNodeOrSymbol==-1))
            return FALSE;return TRUE;};
};

CHuffmanTree::CHuffmanTree (Int lNOfSymbols, Int *lpFrequencies)
{
    assert(lNOfSymbols>1);
    m_lNOfSymbols=lNOfSymbols;
    m_pNodes = new CNode[2*m_lNOfSymbols-1];
    if(lpFrequencies)
        setFrequencies (lpFrequencies);
}

CHuffmanTree::~CHuffmanTree()
{
    delete[]m_pNodes;
}

Void CHuffmanTree::writeSymbol (Int symbolNo,ostream &stream)
{
    stream<<symbolNo<<" ";
}

Void CHuffmanTree::setFrequencies (Int* lpFrequencies)
{
    for (Int lIndex=0;lIndex<m_lNOfSymbols;lIndex++)
        setFrequency(lpFrequencies[lIndex],lIndex);
}

Void CHuffmanTree::setFrequency (Int lFrequency, Int lIndex)
{
    m_pNodes[lIndex].m_lFrequency=lFrequency;
}

Int huffmanNodeCompare (const Void* Element1, const Void* Element2)
{
    if((*((CNode**)Element1))->m_lFrequency<
            (*((CNode**)Element2))->m_lFrequency)return  1;   
    if((*((CNode**)Element1))->m_lFrequency>
            (*((CNode**)Element2))->m_lFrequency)return -1;
    if((*((CNode**)Element1))->m_lBalancer<
            (*((CNode**)Element2))->m_lBalancer)return  1;   
    if((*((CNode**)Element1))->m_lBalancer>
            (*((CNode**)Element2))->m_lBalancer)return -1;
    return 0;
}

Void CHuffmanTree::buildTree ()
{
    assert(m_lNOfSymbols>1);
    Int NOfNodesForSorting=m_lNOfSymbols;
    Int NextFreeNode=m_lNOfSymbols;
    CNode **pSortingArray;
    pSortingArray=new CNode*[m_lNOfSymbols];
    for (Int i=0;i<m_lNOfSymbols;i++)
        pSortingArray [i] = &m_pNodes[i];
    
    while (NOfNodesForSorting>1)
    {
        qsort(&pSortingArray[0],NOfNodesForSorting,
                sizeof(pSortingArray[0]),huffmanNodeCompare);
        pSortingArray[NOfNodesForSorting-2]->m_lNodeIndex=NextFreeNode;
        pSortingArray[NOfNodesForSorting-1]->m_lNodeIndex=NextFreeNode;
        pSortingArray[NOfNodesForSorting-2]->m_cCode=0;
        pSortingArray[NOfNodesForSorting-1]->m_cCode=1;
        m_pNodes[NextFreeNode].m_lFrequency=
                pSortingArray[NOfNodesForSorting-2]->m_lFrequency+
                pSortingArray[NOfNodesForSorting-1]->m_lFrequency;
        m_pNodes[NextFreeNode].m_lBalancer=
                pSortingArray[NOfNodesForSorting-2]->m_lBalancer+
                pSortingArray[NOfNodesForSorting-1]->m_lBalancer;
        pSortingArray[NOfNodesForSorting-2]=&m_pNodes[NextFreeNode];
        NOfNodesForSorting--;
        NextFreeNode++;
    };
    delete pSortingArray;
}

void CHuffmanTree::writeOneTableEntry(ostream &stream, Int entryNo,
                                    Double lTotalFrequency, Double &dNOfBits)
{
    Double dP=m_pNodes[entryNo].m_lFrequency/(Double)lTotalFrequency;
    Char *pCodeArray=new Char[m_lNOfSymbols-1];
    Int lNode=entryNo;
    Int lCodeNo=0;
    while(lNode!=2*m_lNOfSymbols-2)
    {
        pCodeArray[lCodeNo++]=m_pNodes[lNode].m_cCode;
        lNode=m_pNodes[lNode].m_lNodeIndex;
    };
    writeSymbol(entryNo,stream);
    dNOfBits+=dP*lCodeNo;
    while(lCodeNo>0)
    {
        lCodeNo--;
        stream<<(Int)pCodeArray[lCodeNo];
    };
    stream<<endl;
    delete pCodeArray;
}

Void CHuffmanTree::writeTable (ostream &stream)
{
    Int lTotalFrequency=0;
    Double dEntropy=0;
    Double dNOfBits=0;
    statistics (lTotalFrequency, dEntropy);
    for (Int i = 0; i < m_lNOfSymbols; i++)
        writeOneTableEntry (stream, i, lTotalFrequency, dNOfBits);
    printStatistics (dEntropy, dNOfBits, stream);
}

Void CHuffmanTree::writeTableSorted(ostream &stream)
{
    Int lTotalFrequency=0;
    Double dEntropy=0;
    Double dNOfBits=0;
    statistics(lTotalFrequency,dEntropy);
    CNode **pSortingArray;
    pSortingArray = new CNode*[m_lNOfSymbols];
	Int i;
    for (i = 0; i < m_lNOfSymbols; i++)
        pSortingArray [i] = &m_pNodes[i];
    qsort(&pSortingArray[0],m_lNOfSymbols,
            sizeof (pSortingArray [0]), huffmanNodeCompare);
    for(i=0;i<m_lNOfSymbols;i++)
        writeOneTableEntry (stream, pSortingArray [i] - m_pNodes,
                lTotalFrequency,dNOfBits);
    delete pSortingArray;
    printStatistics (dEntropy, dNOfBits, stream);
}

Void CHuffmanTree::statistics (Int& lTotalFrequency,Double &dEntropy)
{
	Int i;
    for (i=0;i<m_lNOfSymbols;i++)
        lTotalFrequency+=m_pNodes[i].m_lFrequency;
    for(i=0;i<m_lNOfSymbols;i++)
    {
        double dP=m_pNodes[i].m_lFrequency/(double)lTotalFrequency;
        if(dP!=0)
            dEntropy+=dP*log(1.0/dP)/log(2.0);    
    };
}

Void CHuffmanTree::printStatistics (Double dEntropy, Double dNOfBits, ostream &stream)
{
    stream<<endl<<endl;
    stream<<"//Entropy Per Symbol : "<<dEntropy<<endl;
    stream<<"//Bits Per Symbol    : "<<dNOfBits<<endl;
    stream<<"//Table Efficiency   : "<<dEntropy/dNOfBits<<endl;
}

Int CHuffmanCoDec::makeIndexFromSymbolInTable(istream &huffmanTable)  
{
    Int lR;
	huffmanTable >> lR;
	return lR;
}

Void CHuffmanCoDec::trashRestOfLine (istream &str)
{
    Int iC;
    do
    {
        iC = str.get ();
    }
    while ((iC!='\n') && (iC!=EOF));
}

Bool CHuffmanCoDec::processOneLine (istream &huffmanTable, Int &lSymbol,
                    Int &lCodeSize, Char *pCode)
{
	huffmanTable >> ws;
    while(huffmanTable.peek()=='/')
    {
		trashRestOfLine (huffmanTable);
		huffmanTable >> ws;
    }
    if(huffmanTable.peek()==EOF)
        return FALSE;

	lSymbol=makeIndexFromSymbolInTable(huffmanTable);      
	huffmanTable >> ws;
	int iC=huffmanTable.get();                            
	lCodeSize=0;                                       
	while((iC=='0')||(iC=='1'))                             
    {                                                       
        if(pCode)                                           
        {                                                   
            if(iC=='0')                                     
                pCode[lCodeSize]=0;                         
            else                                            
                pCode[lCodeSize]=1;                         
        };                                                  
        lCodeSize++;                                        
        iC=huffmanTable.get();                            
    };
    if((iC!='\n')&&(iC!=EOF))
        trashRestOfLine(huffmanTable);                         
    assert(lCodeSize);
    return TRUE;
}

Void CHuffmanCoDec::profileTable (istream &huffmanTable,
            Int &lNOfSymbols, Int &lMaxCodeSize)
{
    huffmanTable.clear();
    huffmanTable.seekg(0,ios::beg);
    lNOfSymbols=0;
    lMaxCodeSize=0;
    while(huffmanTable.peek()!=EOF)
    {
        Int lCodeSize;
        Int lSymbol;
        if (processOneLine(huffmanTable,lSymbol,lCodeSize,NULL))
        {
            lNOfSymbols++;
            if(lCodeSize>lMaxCodeSize)
                lMaxCodeSize=lCodeSize;
            assert(lCodeSize);
        };
    };
    assert(lNOfSymbols>1);
    assert(lMaxCodeSize);
}

CHuffmanDecoder::CHuffmanDecoder (CInBitStream *bitStream)
{
    attachStream (bitStream);
}

CHuffmanDecoder::CHuffmanDecoder (CInBitStream *bitStream, VlcTable *pVlc)
{
    attachStream (bitStream);
	loadTable (pVlc);

}

CHuffmanDecoder::~CHuffmanDecoder()
{
    delete []m_pTree;
}

Void CHuffmanDecoder::realloc(Int lOldSize,Int lNewSize)
{
    CHuffmanDecoderNode *pNewTree=new CHuffmanDecoderNode[lNewSize];
    Int i;
    for(i=0;i<lOldSize;i++)
    {
        pNewTree[i].m_c0End=m_pTree[i].m_c0End;
        pNewTree[i].m_c1End=m_pTree[i].m_c1End;
        pNewTree[i].m_l0NextNodeOrSymbol=m_pTree[i].m_l0NextNodeOrSymbol;
        pNewTree[i].m_l1NextNodeOrSymbol=m_pTree[i].m_l1NextNodeOrSymbol;
    };
    delete []m_pTree;
    m_pTree=pNewTree;
}

void CHuffmanDecoder::loadTable(istream &huffmanTable,Bool bIncompleteTree)
{
    Int lTableSize;
    Int lNOfSymbols;
    Int lMaxCodeSize;
    Int lNextFreeNode=1;
    profileTable (huffmanTable,lNOfSymbols,lMaxCodeSize);
    assert(lNOfSymbols>1);
    assert(lMaxCodeSize);
    m_pTree = new CHuffmanDecoderNode [lNOfSymbols - 1];
    lTableSize=lNOfSymbols-1;
    Char *pCode = new Char[lMaxCodeSize];
    huffmanTable.clear();
    huffmanTable.seekg(0,ios::beg);
    while(huffmanTable.peek()!=EOF)
    {
        Int lCodeSize;
        Int lSymbol;
        Int lCurrentNode=0;
        if (processOneLine (huffmanTable,lSymbol,lCodeSize,pCode))
        {
            assert((lSymbol<lNOfSymbols)||bIncompleteTree);                                              
            assert(lCodeSize);                                                        
            for(Int i=0;i<lCodeSize;i++)                                             
            {                                                                         
				assert ((lCurrentNode<lNOfSymbols-1)||bIncompleteTree);                                 
                int iBit=pCode[i];                                                    
                assert((iBit==0)||(iBit==1));                                         
                if(i==(lCodeSize-1))                                                  
                {                                                                     
                    if(iBit==0)                                                       
                    {                                                                 
                        assert(!m_pTree[lCurrentNode].is0Valid());                    
                        m_pTree[lCurrentNode].m_c0End=1;                              
                        m_pTree[lCurrentNode].m_l0NextNodeOrSymbol=lSymbol;           
                    }                                                                 
                    else                                                              
                    {                                                                 
                        assert(!m_pTree[lCurrentNode].is1Valid());                    
                        m_pTree[lCurrentNode].m_c1End=1;                              
                        m_pTree[lCurrentNode].m_l1NextNodeOrSymbol=lSymbol;           
                    }                                                                 
                }                                                                     
                else                                                                  
                {                                                                     
                    if(iBit==0)                                                       
                    {                                                                 
                        if(!m_pTree[lCurrentNode].is0Valid())                         
                        {                                                             
							if(bIncompleteTree)
                                if(lNextFreeNode>=lTableSize)
                                {
                                    realloc(lTableSize,lTableSize+10);
                                    lTableSize+=10;
                                };
                            assert((lNextFreeNode<lNOfSymbols-1)||bIncompleteTree);                    
                            m_pTree[lCurrentNode].m_c0End=0;                          
                            m_pTree[lCurrentNode].m_l0NextNodeOrSymbol=lNextFreeNode; 
                            lNextFreeNode++;                                          
                        };                                                            
                        assert(m_pTree[lCurrentNode].m_c0End==0);                     
                        lCurrentNode=m_pTree[lCurrentNode].m_l0NextNodeOrSymbol;      
                    }                                                                 
                    else                                                              
                    {                                                                 
                        if(!m_pTree[lCurrentNode].is1Valid())                         
                        {                                                             
							if(bIncompleteTree)
                                if(lNextFreeNode>=lTableSize)
                                {
                                    realloc(lTableSize,lTableSize+10);
                                    lTableSize+=10;
                                };
                            assert((lNextFreeNode<lNOfSymbols-1)||bIncompleteTree);                    
                            m_pTree[lCurrentNode].m_c1End=0;                          
                            m_pTree[lCurrentNode].m_l1NextNodeOrSymbol=lNextFreeNode; 
                            lNextFreeNode++;                                          
                        };                                                            
                        assert(m_pTree[lCurrentNode].m_c1End==0);                     
                        lCurrentNode=m_pTree[lCurrentNode].m_l1NextNodeOrSymbol;      
                    }                                                                
                }                                                                    
            }
        }
    }
    for(Int i=0;i<lTableSize;i++)
    {
        assert((m_pTree[i].is0Valid())||bIncompleteTree);
        assert((m_pTree[i].is1Valid())||bIncompleteTree);
    };
    delete pCode;
}

void CHuffmanDecoder::loadTable(VlcTable *pVlc,Bool bIncompleteTree)
{
    Int lTableSize;
    Int lNextFreeNode = 1;
    Int lNOfSymbols = 0;
    Int lMaxCodeSize = 0;
	VlcTable *pVlcTmp;
    for(pVlcTmp = pVlc; pVlcTmp->pchBits!=NULL; pVlcTmp++)
    {
        lNOfSymbols++;
		Int lCodeSize = strlen(pVlcTmp->pchBits);

		assert(pVlcTmp->lSymbol>=0 && pVlcTmp->lSymbol<1000);
		assert(lCodeSize>0);

        if(lCodeSize>lMaxCodeSize)
			lMaxCodeSize=lCodeSize;
    }
    assert(lNOfSymbols>1);
    assert(lMaxCodeSize>0);
	
    m_pTree = new CHuffmanDecoderNode [lNOfSymbols - 1];
    lTableSize=lNOfSymbols-1;

    for(pVlcTmp = pVlc; pVlcTmp->pchBits!=NULL; pVlcTmp++)
    {
        Int lCodeSize = strlen(pVlcTmp->pchBits);
        Int lSymbol = pVlcTmp->lSymbol;
        Int lCurrentNode = 0;
		//printf("%d\t%d\t%s\n",lSymbol, lCodeSize, pVlcTmp->pchBits);

        assert((lSymbol<lNOfSymbols)||bIncompleteTree);                                              
        assert(lCodeSize);                                                        
        for(Int i=0;i<lCodeSize;i++)                                             
        {                                                                         
			assert ((lCurrentNode<lNOfSymbols-1)||bIncompleteTree);
            int iBit = pVlcTmp->pchBits[i] - '0';
			assert((iBit==0)||(iBit==1));                                         
            if(i==(lCodeSize-1))                                                  
            {                                                                     
                if(iBit==0)                                                       
                {                                                                 
                    assert(!m_pTree[lCurrentNode].is0Valid());                    
                    m_pTree[lCurrentNode].m_c0End=1;                              
                    m_pTree[lCurrentNode].m_l0NextNodeOrSymbol=lSymbol;           
                }                                                                 
                else                                                              
                {                                                                 
                    assert(!m_pTree[lCurrentNode].is1Valid());                    
                    m_pTree[lCurrentNode].m_c1End=1;                              
                    m_pTree[lCurrentNode].m_l1NextNodeOrSymbol=lSymbol;           
                }                                                                 
            }                                                                     
            else                                                                  
            {                                                                     
                if(iBit==0)                                                       
                {                                                                 
                    if(!m_pTree[lCurrentNode].is0Valid())                         
                    {                                                             
						if(bIncompleteTree)
                            if(lNextFreeNode>=lTableSize)
                            {
                                realloc(lTableSize,lTableSize+10);
                                lTableSize+=10;
                            };
                        assert((lNextFreeNode<lNOfSymbols-1)||bIncompleteTree);                    
                        m_pTree[lCurrentNode].m_c0End=0;                          
                        m_pTree[lCurrentNode].m_l0NextNodeOrSymbol=lNextFreeNode; 
                        lNextFreeNode++;                                          
                    };                                                            
                    assert(m_pTree[lCurrentNode].m_c0End==0);                     
                    lCurrentNode=m_pTree[lCurrentNode].m_l0NextNodeOrSymbol;      
                }                                                                 
                else                                                              
                {                                                                 
                    if(!m_pTree[lCurrentNode].is1Valid())                         
                    {                                                             
						if(bIncompleteTree)
                            if(lNextFreeNode>=lTableSize)
                            {
                                realloc(lTableSize,lTableSize+10);
                                lTableSize+=10;
                            };
                        assert((lNextFreeNode<lNOfSymbols-1)||bIncompleteTree);                    
                        m_pTree[lCurrentNode].m_c1End=0;                          
                        m_pTree[lCurrentNode].m_l1NextNodeOrSymbol=lNextFreeNode; 
                        lNextFreeNode++;                                          
                    };                                                            
                    assert(m_pTree[lCurrentNode].m_c1End==0);                     
                    lCurrentNode=m_pTree[lCurrentNode].m_l1NextNodeOrSymbol;      
                }                                                                
            }                                                                    
        }
    }
    for(Int i=0;i<lTableSize;i++)
    {
        assert((m_pTree[i].is0Valid())||bIncompleteTree);
        assert((m_pTree[i].is1Valid())||bIncompleteTree);
    }
}

Void CHuffmanDecoder::attachStream(CInBitStream *bitStream)
{
    m_pBitStream=bitStream;
}

Int CHuffmanDecoder::decodeSymbol()
{
    Char cEnd=0;
    Int lNextNodeOrSymbol=0;
    do
    {
        Int iBit;
        iBit=m_pBitStream->getBits(1);
        if(iBit==0)
        {
            if(m_pTree[lNextNodeOrSymbol].is0Valid())
            {
                cEnd=m_pTree[lNextNodeOrSymbol].m_c0End;
                lNextNodeOrSymbol=m_pTree[lNextNodeOrSymbol].m_l0NextNodeOrSymbol;
            }
            else
            {
                cEnd=TRUE;
                lNextNodeOrSymbol=-1;
            }
        }
        else
        {
            if(m_pTree[lNextNodeOrSymbol].is1Valid())
            {
                cEnd=m_pTree[lNextNodeOrSymbol].m_c1End;
                lNextNodeOrSymbol=m_pTree[lNextNodeOrSymbol].m_l1NextNodeOrSymbol;
            }
            else
            {
                cEnd=TRUE;
                lNextNodeOrSymbol=-1;
            }
        };
    }
    while(cEnd==0);
    return lNextNodeOrSymbol;
}

CHuffmanEncoder::~CHuffmanEncoder ()
{
	delete m_pCodeTable; 
	delete m_pSizeTable;
}

CHuffmanEncoder::CHuffmanEncoder(COutBitStream &bitStream)
{
    attachStream(bitStream);
}

CHuffmanEncoder::CHuffmanEncoder(COutBitStream &bitStream, VlcTable *pVlc)
{
	attachStream(bitStream);

	loadTable(pVlc);
}

Void CHuffmanEncoder::loadTable(istream &huffmanTable)
{
    Int lNOfSymbols;
    Int lMaxCodeSize;
    profileTable(huffmanTable,lNOfSymbols,lMaxCodeSize);
    assert(lNOfSymbols>1);
    assert(lMaxCodeSize);
    m_lCodeTableEntrySize=lMaxCodeSize/8;
    if(lMaxCodeSize%8)
        m_lCodeTableEntrySize++;
    m_pSizeTable = new Int [lNOfSymbols];
    m_pCodeTable = new Int [lNOfSymbols];
    Char *pCode=new Char[lMaxCodeSize];
    huffmanTable.clear();
    huffmanTable.seekg(0,ios::beg);
    while(huffmanTable.peek()!=EOF)
    {
        Int lCodeSize;
        Int lSymbol;
        Int iBitPosition=0;
        if(processOneLine(huffmanTable,lSymbol,lCodeSize,pCode))
        {
            assert(lSymbol<lNOfSymbols);                                                 
            assert(lCodeSize >=0 && lCodeSize <= (Int) sizeof (Int) * 8);
            m_pSizeTable[lSymbol]=lCodeSize;                                             
            Int* pCodeTableEntry=&m_pCodeTable[lSymbol]; 
            for(Int i=0;i<lCodeSize;i++)                                                
            {                                                                            
                if(iBitPosition==0)                                                      
                    *pCodeTableEntry=0;                                                  
                assert((pCode[lCodeSize - i - 1]==0)||(pCode[lCodeSize - i - 1]==1));                                    
                if(pCode[lCodeSize - i - 1]==0)                                                          
                    *pCodeTableEntry&=~(0x01<<iBitPosition);                             
                else                                                                     
                    *pCodeTableEntry|=0x01<<iBitPosition;                                
                iBitPosition++;                                                          
				/*
                if(iBitPosition>=8)                                                      
                {                                                                        
                    iBitPosition=0;                                                      
                    pCodeTableEntry++;                                                   
                } 
				*/
            }                                                                           
        }
    }
    delete pCode;
}

Void CHuffmanEncoder::loadTable(VlcTable *pVlc)
{
    Int lNOfSymbols = 0;
    Int lMaxCodeSize = 0;

	VlcTable *pVlcTmp;

	for(pVlcTmp = pVlc; pVlcTmp->pchBits!=NULL; pVlcTmp++)
	{
        lNOfSymbols++;
		Int lCodeSize = strlen(pVlcTmp->pchBits);

		assert(pVlcTmp->lSymbol>=0 && pVlcTmp->lSymbol<1000);
		assert(lCodeSize>0);

        if(lCodeSize>lMaxCodeSize)
			lMaxCodeSize=lCodeSize;
    }
    assert(lNOfSymbols>1);
    assert(lMaxCodeSize>0);

    m_lCodeTableEntrySize=lMaxCodeSize/8;
    if(lMaxCodeSize%8)
        m_lCodeTableEntrySize++;
    m_pSizeTable = new Int [lNOfSymbols];
    m_pCodeTable = new Int [lNOfSymbols];
	
	for(pVlcTmp = pVlc; pVlcTmp->pchBits!=NULL; pVlcTmp++)
    {
        Int lCodeSize = strlen(pVlcTmp->pchBits);
        Int lSymbol = pVlcTmp->lSymbol;
        Int iBitPosition=0;

        assert(lSymbol<lNOfSymbols);                                                 
        assert(lCodeSize >=0 && lCodeSize <= (Int) sizeof (Int) * 8);
        m_pSizeTable[lSymbol]=lCodeSize;                                             
        Int* pCodeTableEntry=&m_pCodeTable[lSymbol]; 
        for(Int i=0;i<lCodeSize;i++)                                                
        {                                                                            
	        if(iBitPosition==0)                                                      
		        *pCodeTableEntry=0;                                                  
			Int iBitC = pVlcTmp->pchBits[lCodeSize - i - 1];
            assert(iBitC=='0' || iBitC=='1');
            if(iBitC=='0')                                                          
				*pCodeTableEntry&=~(0x01<<iBitPosition);                             
            else                                                                     
                *pCodeTableEntry|=0x01<<iBitPosition;                                
            iBitPosition++;                                                          
			/*
                if(iBitPosition>=8)                                                      
                {                                                                        
                    iBitPosition=0;                                                      
                    pCodeTableEntry++;                                                   
                } 
			*/                                                                         
        }
    }
}

void CHuffmanEncoder::attachStream(COutBitStream &bitStream)
{
    m_pBitStream=&bitStream;
}

UInt CHuffmanEncoder::encodeSymbol (Int lSymbol, Char* rgchSymbolName, Bool bDontSendBits)
{
    Int lSize=m_pSizeTable[lSymbol];
	if (bDontSendBits == TRUE)					//for counting bits
		return (lSize);
// Added for Data Partitioning by Toshiba (98-1-16)
	if(m_pBitStream -> GetDontSendBits()) 
		return (lSize);
// End Toshiba(1998-1-16)

    Int iCode=m_pCodeTable[lSymbol];
	Int lPrevCounter = m_pBitStream -> getCounter ();
    m_pBitStream -> putBits (iCode,lSize, rgchSymbolName);
	return (m_pBitStream -> getCounter () - lPrevCounter);
}


CEntropyEncoderSet::CEntropyEncoderSet (COutBitStream &bitStream)
{
	m_pentrencDCT = new CHuffmanEncoder (bitStream, g_rgVlcDCT);
	m_pentrencDCTIntra = new CHuffmanEncoder (bitStream, g_rgVlcDCTIntra);
 	//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
 	m_pentrencDCTRVLC = new CHuffmanEncoder (bitStream, g_rgVlcDCTRVLC);
 	m_pentrencDCTIntraRVLC = new CHuffmanEncoder (bitStream, g_rgVlcDCTIntraRVLC);
 	//	End Toshiba(1998-1-16:DP+RVLC)
	m_pentrencMV = new CHuffmanEncoder (bitStream, g_rgVlcMV);
	m_pentrencMCBPCintra = new CHuffmanEncoder (bitStream, g_rgVlcMCBPCintra);
	m_pentrencMCBPCinter = new CHuffmanEncoder (bitStream, g_rgVlcMCBPCinter);
	m_pentrencCBPY = new CHuffmanEncoder (bitStream, g_rgVlcCBPY);
	m_pentrencCBPY1 = new CHuffmanEncoder (bitStream, g_rgVlcCBPY1);
	m_pentrencCBPY2 = new CHuffmanEncoder (bitStream, g_rgVlcCBPY2);
	m_pentrencCBPY3 = new CHuffmanEncoder (bitStream, g_rgVlcCBPY3);
	m_pentrencIntraDCy = new CHuffmanEncoder (bitStream, g_rgVlcIntraDCy);
	m_pentrencIntraDCc = new CHuffmanEncoder (bitStream, g_rgVlcIntraDCc);
	m_pentrencMbTypeBVOP = new CHuffmanEncoder (bitStream, g_rgVlcMbTypeBVOP);
 	m_pentrencWrpPnt = new CHuffmanEncoder (bitStream, g_rgVlcWrpPnt);
	m_ppentrencShapeMode [0] = new CHuffmanEncoder (bitStream, g_rgVlcShapeMode0);
	m_ppentrencShapeMode [1] = new CHuffmanEncoder (bitStream, g_rgVlcShapeMode1);
	m_ppentrencShapeMode [2] = new CHuffmanEncoder (bitStream, g_rgVlcShapeMode2);
	m_ppentrencShapeMode [3] = new CHuffmanEncoder (bitStream, g_rgVlcShapeMode3);
	m_ppentrencShapeMode [4] = new CHuffmanEncoder (bitStream, g_rgVlcShapeMode4);
	m_ppentrencShapeMode [5] = new CHuffmanEncoder (bitStream, g_rgVlcShapeMode5);
	m_ppentrencShapeMode [6] = new CHuffmanEncoder (bitStream, g_rgVlcShapeMode6);
//OBSS_SAIT_991015
	m_ppentrencShapeSSModeInter [0] = new CHuffmanEncoder (bitStream, g_rgVlcShapeSSModeInter0);	
	m_ppentrencShapeSSModeInter [1] = new CHuffmanEncoder (bitStream, g_rgVlcShapeSSModeInter1);	
	m_ppentrencShapeSSModeInter [2] = new CHuffmanEncoder (bitStream, g_rgVlcShapeSSModeInter2);	
	m_ppentrencShapeSSModeInter [3] = new CHuffmanEncoder (bitStream, g_rgVlcShapeSSModeInter3);	
	m_ppentrencShapeSSModeIntra [0] = new CHuffmanEncoder (bitStream, g_rgVlcShapeSSModeIntra0);	
	m_ppentrencShapeSSModeIntra [1] = new CHuffmanEncoder (bitStream, g_rgVlcShapeSSModeIntra1);	
//~OBSS_SAIT_991015
	m_pentrencShapeMV1 = new CHuffmanEncoder (bitStream, g_rgVlcShapeMV1);
	m_pentrencShapeMV2 = new CHuffmanEncoder (bitStream, g_rgVlcShapeMV2);
	
	/*ifstream istrmTableDCT;
	ifstream istrmTableDCTintra;
	ifstream istrmTableMV;
	ifstream istrmTableMCBPCintra;
	ifstream istrmTableMCBPCinter;
	ifstream istrmTableCBPY;
	ifstream istrmTableCBPY2;
	ifstream istrmTableCBPY3;
//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
	ifstream istrmTableCBPY1DP;
	ifstream istrmTableCBPY2DP;
	ifstream istrmTableCBPY3DP;
// End Toshiba(1998-1-16:DP+RVLC)
	ifstream istrmTableIntraDCy;
	ifstream istrmTableIntraDCc;
	ifstream istrmMbTypeBVOP;
	ifstream istrmWrpPnt;
	ifstream istrmShapeMode [7];
	ifstream istrmShapeMv1;
	ifstream istrmShapeMv2;
 	//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
 	ifstream istrmTableDCTRVLC;
 	ifstream istrmTableDCTIntraRVLC;
 	//	End Toshiba(1998-1-16:DP+RVLC)

	// get the vm_home value
	Char *rgchVmHome = getenv ( "VM_HOME" );
	assert (rgchVmHome != NULL );
	Char rgchFile [100];
	strcpy (rgchFile, rgchVmHome);
	Int iLength = strlen (rgchFile);
	Char* pch = rgchFile + iLength;

	FULLNAME (sys, vlcdct.txt);
	istrmTableDCT.open (rgchFile);
	FULLNAME (sys, dctin.txt);
	istrmTableDCTintra.open (rgchFile);
	FULLNAME (sys, vlcmvd.txt);
	istrmTableMV.open (rgchFile);
	FULLNAME (sys, mcbpc1.txt);
	istrmTableMCBPCintra.open (rgchFile);
	FULLNAME (sys, mcbpc2.txt);
	istrmTableMCBPCinter.open (rgchFile);
	FULLNAME (sys, cbpy.txt);
	istrmTableCBPY.open (rgchFile);
	FULLNAME (sys, cbpy2.txt);
	istrmTableCBPY2.open (rgchFile);
	FULLNAME (sys, cbpy3.txt);
	istrmTableCBPY3.open (rgchFile);
//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
	FULLNAME (sys, cbpy1dp.txt);
	istrmTableCBPY1DP.open (rgchFile);
	FULLNAME (sys, cbpy2dp.txt);
	istrmTableCBPY2DP.open (rgchFile);
	FULLNAME (sys, cbpy3dp.txt);
	istrmTableCBPY3DP.open (rgchFile);
// End Toshiba(1998-1-16:DP+RVLC)
	FULLNAME (sys, dcszy.txt);
	istrmTableIntraDCy.open (rgchFile);
	FULLNAME (sys, dcszc.txt);
	istrmTableIntraDCc.open (rgchFile);
	FULLNAME (sys, mbtyp.txt);
	istrmMbTypeBVOP.open (rgchFile);
	FULLNAME (sys, wrppnt.txt);
	istrmWrpPnt.open (rgchFile);
	FULLNAME (sys, shpmd0.txt);
	istrmShapeMode[0].open (rgchFile);
	FULLNAME (sys, shpmd1.txt);
	istrmShapeMode[1].open (rgchFile);
	FULLNAME (sys, shpmd2.txt);
	istrmShapeMode[2].open (rgchFile);
	FULLNAME (sys, shpmd3.txt);
	istrmShapeMode[3].open (rgchFile);
	FULLNAME (sys, shpmd4.txt);
	istrmShapeMode[4].open (rgchFile);
	FULLNAME (sys, shpmd5.txt);
	istrmShapeMode[5].open (rgchFile);
	FULLNAME (sys, shpmd6.txt);
	istrmShapeMode[6].open (rgchFile);
	FULLNAME (sys, shpmv1.txt);
	istrmShapeMv1.open (rgchFile);
	FULLNAME (sys, shpmv2.txt);
	istrmShapeMv2.open (rgchFile);
 	//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
 	FULLNAME (sys, rvlcdct.txt);
 	istrmTableDCTRVLC.open (rgchFile);	
 	FULLNAME (sys, rvlcin.txt);
 	istrmTableDCTIntraRVLC.open (rgchFile);
 	//	End Toshiba(1998-1-16:DP+RVLC)

	m_pentrencDCT -> loadTable (istrmTableDCT);
	m_pentrencDCTIntra -> loadTable (istrmTableDCTintra);
    m_pentrencMV -> loadTable (istrmTableMV);
    m_pentrencMCBPCintra -> loadTable (istrmTableMCBPCintra);
    m_pentrencMCBPCinter -> loadTable (istrmTableMCBPCinter);
    m_pentrencCBPY -> loadTable (istrmTableCBPY);
    m_pentrencCBPY2 -> loadTable (istrmTableCBPY2);
    m_pentrencCBPY3 -> loadTable (istrmTableCBPY3);	
//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
    m_pentrencCBPY1DP -> loadTable (istrmTableCBPY1DP);	
    m_pentrencCBPY2DP -> loadTable (istrmTableCBPY2DP);
    m_pentrencCBPY3DP -> loadTable (istrmTableCBPY3DP);	
// End Toshiba(1998-1-16:DP+RVLC)
    m_pentrencIntraDCy -> loadTable (istrmTableIntraDCy);
    m_pentrencIntraDCc -> loadTable (istrmTableIntraDCc);
    m_pentrencMbTypeBVOP->loadTable (istrmMbTypeBVOP);
    m_pentrencWrpPnt->loadTable (istrmWrpPnt);
	for (iShapeMd = 0; iShapeMd < 7; iShapeMd++)
		m_ppentrencShapeMode [iShapeMd]->loadTable (istrmShapeMode [iShapeMd]);
	m_pentrencShapeMV1->loadTable (istrmShapeMv1);
	m_pentrencShapeMV2->loadTable (istrmShapeMv2);
 	//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
 	m_pentrencDCTRVLC -> loadTable (istrmTableDCTRVLC);
 	m_pentrencDCTIntraRVLC -> loadTable (istrmTableDCTIntraRVLC);
 	//	End Toshiba(1998-1-16:DP+RVLC)
	*/
}

CEntropyEncoderSet::~CEntropyEncoderSet()
{
	delete m_pentrencDCT;
	delete m_pentrencDCTIntra;
	delete m_pentrencMV;
	delete m_pentrencMCBPCintra;
	delete m_pentrencMCBPCinter;
	delete m_pentrencCBPY;
	delete m_pentrencCBPY2;
	delete m_pentrencCBPY3;
	delete m_pentrencIntraDCy;
	delete m_pentrencIntraDCc;
	delete m_pentrencMbTypeBVOP;
	delete m_pentrencWrpPnt;
	for (UInt iShapeMd = 0; iShapeMd < 7; iShapeMd++)
		delete m_ppentrencShapeMode [iShapeMd];

//OBSS_SAIT_991015
	for (UInt iShapeSSMdInter = 0; iShapeSSMdInter < 4; iShapeSSMdInter++)
		delete m_ppentrencShapeSSModeInter [iShapeSSMdInter];
	for (UInt iShapeSSMdIntra = 0; iShapeSSMdIntra < 2; iShapeSSMdIntra++)
		delete m_ppentrencShapeSSModeIntra [iShapeSSMdIntra];
//~OBSS_SAIT_991015

	delete m_pentrencShapeMV1;
	delete m_pentrencShapeMV2;
 	//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
 	delete m_pentrencDCTRVLC;
 	delete m_pentrencDCTIntraRVLC;
 	//	End Toshiba(1998-1-16:DP+RVLC)
}

CEntropyDecoderSet::CEntropyDecoderSet (CInBitStream *bitStream)
{
	m_pentrdecDCT = new CHuffmanDecoder (bitStream, g_rgVlcDCT);
	m_pentrdecDCTIntra = new CHuffmanDecoder (bitStream, g_rgVlcDCTIntra);
 	//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
 	m_pentrdecDCTRVLC = new CHuffmanDecoder (bitStream, g_rgVlcDCTRVLC);
 	m_pentrdecDCTIntraRVLC = new CHuffmanDecoder (bitStream, g_rgVlcDCTIntraRVLC);
 	//	End Toshiba(1998-1-16:DP+RVLC)
	m_pentrdecMV = new CHuffmanDecoder (bitStream, g_rgVlcMV);
	m_pentrdecMCBPCintra = new CHuffmanDecoder (bitStream, g_rgVlcMCBPCintra);
	m_pentrdecMCBPCinter = new CHuffmanDecoder (bitStream, g_rgVlcMCBPCinter);
	m_pentrdecCBPY = new CHuffmanDecoder (bitStream, g_rgVlcCBPY);
	m_pentrdecCBPY1 = new CHuffmanDecoder (bitStream, g_rgVlcCBPY1);
	m_pentrdecCBPY2 = new CHuffmanDecoder (bitStream, g_rgVlcCBPY2);
	m_pentrdecCBPY3 = new CHuffmanDecoder (bitStream, g_rgVlcCBPY3);
	m_pentrdecIntraDCy = new CHuffmanDecoder (bitStream, g_rgVlcIntraDCy);
	m_pentrdecIntraDCc = new CHuffmanDecoder (bitStream, g_rgVlcIntraDCc);
	m_pentrdecMbTypeBVOP = new CHuffmanDecoder (bitStream, g_rgVlcMbTypeBVOP);
 	m_pentrdecWrpPnt = new CHuffmanDecoder (bitStream, g_rgVlcWrpPnt);
	m_ppentrdecShapeMode [0] = new CHuffmanDecoder (bitStream, g_rgVlcShapeMode0);
	m_ppentrdecShapeMode [1] = new CHuffmanDecoder (bitStream, g_rgVlcShapeMode1);
	m_ppentrdecShapeMode [2] = new CHuffmanDecoder (bitStream, g_rgVlcShapeMode2);
	m_ppentrdecShapeMode [3] = new CHuffmanDecoder (bitStream, g_rgVlcShapeMode3);
	m_ppentrdecShapeMode [4] = new CHuffmanDecoder (bitStream, g_rgVlcShapeMode4);
	m_ppentrdecShapeMode [5] = new CHuffmanDecoder (bitStream, g_rgVlcShapeMode5);
	m_ppentrdecShapeMode [6] = new CHuffmanDecoder (bitStream, g_rgVlcShapeMode6);
//OBSS_SAIT_991015
	m_ppentrdecShapeSSModeInter [0] = new CHuffmanDecoder (bitStream, g_rgVlcShapeSSModeInter0);
	m_ppentrdecShapeSSModeInter [1] = new CHuffmanDecoder (bitStream, g_rgVlcShapeSSModeInter1);
	m_ppentrdecShapeSSModeInter [2] = new CHuffmanDecoder (bitStream, g_rgVlcShapeSSModeInter2);
	m_ppentrdecShapeSSModeInter [3] = new CHuffmanDecoder (bitStream, g_rgVlcShapeSSModeInter3);
	m_ppentrdecShapeSSModeIntra [0] = new CHuffmanDecoder (bitStream, g_rgVlcShapeSSModeIntra0);
	m_ppentrdecShapeSSModeIntra [1] = new CHuffmanDecoder (bitStream, g_rgVlcShapeSSModeIntra1);
//~OBSS_SAIT_991015
	m_pentrdecShapeMV1 = new CHuffmanDecoder (bitStream, g_rgVlcShapeMV1);
	m_pentrdecShapeMV2 = new CHuffmanDecoder (bitStream, g_rgVlcShapeMV2);

/*	ifstream istrmTableDCT;
	ifstream istrmTableDCTIntra;
	ifstream istrmTableMV;
	ifstream istrmTableMCBPCintra;
	ifstream istrmTableMCBPCinter;
	ifstream istrmTableCBPY;
	ifstream istrmTableCBPY2;
	ifstream istrmTableCBPY3;
//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
	ifstream istrmTableCBPY1DP;
	ifstream istrmTableCBPY2DP;
	ifstream istrmTableCBPY3DP;
// End Toshiba(1998-1-16:DP+RVLC)
	ifstream istrmTableIntraDCy;
	ifstream istrmTableIntraDCc;
	ifstream istrmMbTypeBVOP;
	ifstream istrmWrpPnt;
	ifstream istrmShapeMode [7];
	ifstream istrmShapeMv1;
	ifstream istrmShapeMv2;
 	//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
 	ifstream istrmTableDCTRVLC;
 	ifstream istrmTableDCTIntraRVLC;
 	//	End Toshiba(1998-1-16:DP+RVLC)

	// get the vm_home value
	Char *rgchVmHome = getenv ( "VM_HOME" );
	assert (rgchVmHome != NULL );
	Char rgchFile [100];
	strcpy (rgchFile, rgchVmHome);
	Int iLength = strlen (rgchFile);
	Char* pch = rgchFile + iLength;
	
	FULLNAME (sys, vlcdct.txt);
	istrmTableDCT.open (rgchFile);	
	FULLNAME (sys, dctin.txt);
	istrmTableDCTIntra.open (rgchFile);
	FULLNAME (sys, vlcmvd.txt);
	istrmTableMV.open (rgchFile);	
	FULLNAME (sys, mcbpc1.txt);
	istrmTableMCBPCintra.open (rgchFile);	
	FULLNAME (sys, mcbpc2.txt);
	istrmTableMCBPCinter.open (rgchFile);	
	FULLNAME (sys, cbpy.txt);
	istrmTableCBPY.open (rgchFile);	
	FULLNAME (sys, cbpy2.txt);
	istrmTableCBPY2.open (rgchFile);	
	FULLNAME (sys, cbpy3.txt);
	istrmTableCBPY3.open (rgchFile);	
//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
	FULLNAME (sys, cbpy1dp.txt);
	istrmTableCBPY1DP.open (rgchFile);
	FULLNAME (sys, cbpy2dp.txt);
	istrmTableCBPY2DP.open (rgchFile);
	FULLNAME (sys, cbpy3dp.txt);
	istrmTableCBPY3DP.open (rgchFile);
// End Toshiba(1998-1-16:DP+RVLC)
	FULLNAME (sys, dcszy.txt);
	istrmTableIntraDCy.open (rgchFile);	
	FULLNAME (sys, dcszc.txt);
	istrmTableIntraDCc.open (rgchFile);	
	FULLNAME (sys, mbtyp.txt);
	istrmMbTypeBVOP.open (rgchFile);		
	FULLNAME (sys, wrppnt.txt);
	istrmWrpPnt.open (rgchFile);	
	FULLNAME (sys, shpmd0.txt);
	istrmShapeMode[0].open (rgchFile);	
	FULLNAME (sys, shpmd1.txt);
	istrmShapeMode[1].open (rgchFile);	
	FULLNAME (sys, shpmd2.txt);
	istrmShapeMode[2].open (rgchFile);	
	FULLNAME (sys, shpmd3.txt);
	istrmShapeMode[3].open (rgchFile);	
	FULLNAME (sys, shpmd4.txt);
	istrmShapeMode[4].open (rgchFile);	
	FULLNAME (sys, shpmd5.txt);
	istrmShapeMode[5].open (rgchFile);	
	FULLNAME (sys, shpmd6.txt);
	istrmShapeMode[6].open (rgchFile);	
	FULLNAME (sys, shpmv1.txt);
	istrmShapeMv1.open (rgchFile);	
	FULLNAME (sys, shpmv2.txt);
	istrmShapeMv2.open (rgchFile);
 	//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
 	FULLNAME (sys, rvlcdct.txt);
 	istrmTableDCTRVLC.open (rgchFile);	
 	FULLNAME (sys, rvlcin.txt);
 	istrmTableDCTIntraRVLC.open (rgchFile);
 	//	End Toshiba(1998-1-16:DP+RVLC)

	m_pentrdecDCT -> loadTable (istrmTableDCT);
	m_pentrdecDCTIntra -> loadTable (istrmTableDCTIntra);
    m_pentrdecMV -> loadTable (istrmTableMV);
    m_pentrdecMCBPCintra -> loadTable (istrmTableMCBPCintra);
    m_pentrdecMCBPCinter -> loadTable (istrmTableMCBPCinter);
    m_pentrdecCBPY -> loadTable (istrmTableCBPY);
    m_pentrdecCBPY2 -> loadTable (istrmTableCBPY2);
    m_pentrdecCBPY3 -> loadTable (istrmTableCBPY3);	
//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
    m_pentrdecCBPY1DP -> loadTable (istrmTableCBPY1DP);	
    m_pentrdecCBPY2DP -> loadTable (istrmTableCBPY2DP);
    m_pentrdecCBPY3DP -> loadTable (istrmTableCBPY3DP);	
// End Toshiba(1998-1-16:DP+RVLC)
    m_pentrdecIntraDCy -> loadTable (istrmTableIntraDCy);
    m_pentrdecIntraDCc -> loadTable (istrmTableIntraDCc);
    m_pentrdecMbTypeBVOP -> loadTable (istrmMbTypeBVOP);
	m_pentrdecWrpPnt  -> loadTable (istrmWrpPnt);
	for (iShapeMd = 0; iShapeMd < 7; iShapeMd++)
		m_ppentrdecShapeMode [iShapeMd]->loadTable (istrmShapeMode[iShapeMd]);
	m_pentrdecShapeMV1->loadTable (istrmShapeMv1);
	m_pentrdecShapeMV2->loadTable (istrmShapeMv2);
 	//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
 	m_pentrdecDCTRVLC -> loadTable (istrmTableDCTRVLC);
 	m_pentrdecDCTIntraRVLC -> loadTable (istrmTableDCTIntraRVLC);
 	//	End Toshiba(1998-1-16:DP+RVLC)
	*/
}

CEntropyDecoderSet::~CEntropyDecoderSet()
{
	delete m_pentrdecDCT;
	delete m_pentrdecDCTIntra;
	delete m_pentrdecMV;
	delete m_pentrdecMCBPCintra;
	delete m_pentrdecMCBPCinter;
	delete m_pentrdecCBPY;
	delete m_pentrdecCBPY1;
	delete m_pentrdecCBPY2;
	delete m_pentrdecCBPY3;
	delete m_pentrdecIntraDCy;
	delete m_pentrdecIntraDCc;
	delete m_pentrdecMbTypeBVOP;
	delete m_pentrdecWrpPnt ;
	for (UInt iShapeMd = 0; iShapeMd < 7; iShapeMd++)
		delete m_ppentrdecShapeMode [iShapeMd];
//OBSS_SAIT_991015
	for (UInt iShapeSSMdInter = 0; iShapeSSMdInter < 4; iShapeSSMdInter++)
		delete m_ppentrdecShapeSSModeInter [iShapeSSMdInter];
	for (UInt iShapeSSMdIntra = 0; iShapeSSMdIntra < 2; iShapeSSMdIntra++)
		delete m_ppentrdecShapeSSModeIntra [iShapeSSMdIntra];
//~OBSS_SAIT_991015
	delete m_pentrdecShapeMV1;
	delete m_pentrdecShapeMV2;
 	//	Added for data partitioning mode By Toshiba(1998-1-16:DP+RVLC)
 	delete m_pentrdecDCTRVLC;
 	delete m_pentrdecDCTIntraRVLC;
 	//	End Toshiba(1998-1-16:DP+RVLC)
}
