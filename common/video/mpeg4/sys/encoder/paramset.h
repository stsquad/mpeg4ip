/*************************************************************************

This software module was originally developed by 

	Simon Winder (swinder@microsoft.com), Microsoft Corporation

in the course of development of the MPEG-4 Video (ISO/IEC 14496-2). 
This software module is an implementation of a part of one or more MPEG-4 Video tools 
as specified by the MPEG-4 Video. 
ISO/IEC gives users of the MPEG-4 Video free license to this software module or modifications 
thereof for use in hardware or software products claiming conformance to the MPEG-4 Video. 
Those intending to use this software module in hardware or software products are advised that
its use may infringe existing patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have no liability for use of
this software module or modifications thereof in an implementation. Copyright is not released
for non MPEG-4 Video conforming products. Microsoft retains full right to use the code for
his/her own purpose, assign or donate the code to a third party and to inhibit third parties
from using the code for non MPEG-4 Video conforming products. This copyright notice must be
included in all copies or derivative works. 

Copyright (c) 1996, 1997.


*************************************************************************/

#define PAR_HASHSIZE 31
#define TOKEN_SIZE 4096

#define ERES_OK 0
#define ERES_PARAMS 1
#define ERES_NOOBJ 2
#define ERES_MEMORY 3
#define ERES_EOF 4
#define ERES_FORMAT 5

typedef int Int;
typedef double Double;
typedef int Bool;
#define FALSE 0
#define TRUE 1


////////////////////////////////////////////////////////////////////////
// CxParamSet
////////////////////////////////////////////////////////////////////////

typedef enum { PS_DOUBLE, PS_STRING, PS_ARRAY } TxParamType;

typedef struct {
	TxParamType tType;
	Int iSize;
	union {
		Double dData;
		char *pchData;
		Double *pdData;
	};
} TxParamValue;

typedef struct TagTxParamEntry {
	char *pchName;
	Int iIndex;
	TxParamValue tValue;
	TagTxParamEntry *pPrevCreate;
	TagTxParamEntry *pNextCreate;
	TagTxParamEntry *pPrevHash;
	TagTxParamEntry *pNextHash;
} TxParamEntry;

class CxParamSet
{
public:	
	CxParamSet();
	~CxParamSet();

	Int GetParam(const char *pchName, Int iIndex, TxParamValue *ptValueRtn);
	Int SetParam(const char *pchName, Int iIndex, const TxParamValue *ptValue);
	Int DeleteParam(const char *pchName, Int iIndex);
	void StartEnum();
	Bool NextEnum(char **ppchNameRtn, Int *piIndexRtn, TxParamValue *ptValueRtn = NULL);
	void DeleteAll();

	Int GetDouble(char *pchName, Int iIndex, Double *pdValue);
	Int GetString(char *pchName, Int iIndex, char **ppchVal);
	Int GetArray(char *pchName, Int iIndex, Double **pdValue, Int *piSize);

	Int Load(FILE *fp, Int *piErrLine);
	void Dump(FILE *fp);

protected:
	void DeleteHashEntry(TxParamEntry *pEntry, Int iHash);
	TxParamEntry *FindHashEntry(const char *pchName, Int iIndex, Int *piHashRtn);

	Int GetToken(char *pchBuf);
	Int SkipComment();
	Int SkipSpace();
	Int ReadValue(char *pchBuf, TxParamValue *ptVal);
	Int GetC();
	void UnGetC(Int ch);

	TxParamEntry *m_pCurrentEnum;
	TxParamEntry *m_pCreateListTail;
	TxParamEntry *m_rgpHashList[PAR_HASHSIZE];

	Int *m_piErrLine;
	FILE *m_fp;
};


#define SetPVDouble(pv, val)	do { (pv).tType = PS_DOUBLE; (pv).dData = val; } while(0)
#define SetPVString(pv, val)	do { (pv).tType = PS_STRING; (pv).pchData = val; } while(0)
#define SetPVArray(pv, val, sz)		do { (pv).tType = PS_ARRAY; (pv).pdData = val; (pv).iSize = sz; } while(0)

