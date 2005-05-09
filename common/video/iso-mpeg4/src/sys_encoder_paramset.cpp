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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "paramset.h"


#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

////////////////////////////////////////////////////////////////////////
// CxParamSet
////////////////////////////////////////////////////////////////////////

CxParamSet::CxParamSet()
	: m_pCurrentEnum(NULL), m_pCreateListTail(NULL)
{
	Int i;
	for(i=0; i<PAR_HASHSIZE; i++)
		m_rgpHashList[i] = NULL;
}

CxParamSet::~CxParamSet()
{
	DeleteAll();
}

Int CxParamSet::GetParam(const char *pchName, Int iIndex, TxParamValue *ptValueRtn)
{
	if(pchName==NULL)
		return ERES_PARAMS;
	//if(iIndex<-1)
	//	iIndex = -1;
	if(iIndex<0)
		iIndex = 0;

	Int iHash;
	TxParamEntry *pEntry = FindHashEntry(pchName, iIndex, &iHash);
	if(pEntry==NULL)
		return ERES_NOOBJ;

	if(ptValueRtn!=NULL)
		*ptValueRtn = pEntry->tValue;
	
	return ERES_OK;
}

Int CxParamSet::SetParam(const char *pchName, Int iIndex, const TxParamValue *ptValue)
{
	if(pchName==NULL || ptValue==NULL)
		return ERES_PARAMS;
	if(ptValue->tType==PS_STRING && ptValue->pchData==NULL)
		return ERES_PARAMS;
	if(ptValue->tType==PS_ARRAY && ptValue->pdData==NULL)
		return ERES_PARAMS;
	//if(iIndex<-1)
	//	iIndex = -1;
	if(iIndex<0)
		iIndex = 0;

	Int iHash;
	TxParamEntry *pEntry = FindHashEntry(pchName, iIndex, &iHash);
	if(pEntry==NULL)
	{
		// validate new name
		Int i, iL;
		iL = strlen(pchName);
		for(i=0; i<iL; i++)
			if(isspace(pchName[i]) || pchName[i]=='/' || pchName[i]=='['
				|| pchName[i]=='=')
				return ERES_PARAMS;

		// create new entry at iHash
		pEntry = new TxParamEntry;
		if(pEntry==NULL)
			return ERES_MEMORY;
		pEntry->pchName = new char [strlen(pchName) + 1];
		if(pEntry->pchName==NULL)
		{
			delete pEntry;
			return ERES_MEMORY;
		}
		strcpy(pEntry->pchName, pchName);
		pEntry->iIndex = iIndex;

		// copy data
		pEntry->tValue = *ptValue;
		if(pEntry->tValue.tType==PS_STRING)
		{
			pEntry->tValue.pchData = new char [strlen(ptValue->pchData) + 1];
			if(pEntry->tValue.pchData==NULL)
			{
				delete pEntry->pchName;
				delete pEntry;
				return ERES_MEMORY;
			}
			strcpy(pEntry->tValue.pchData, ptValue->pchData);
		}
		else if(pEntry->tValue.tType==PS_ARRAY)
		{
			pEntry->tValue.pdData = new Double [pEntry->tValue.iSize + 1]; // add one in case size is zero
			if(pEntry->tValue.pdData==NULL)
			{
				delete pEntry->pchName;
				delete pEntry;
				return ERES_MEMORY;
			}
			memcpy(pEntry->tValue.pdData, ptValue->pdData, pEntry->tValue.iSize * sizeof(Double));
		}
		
		// add to hash list at head
		pEntry->pNextHash = m_rgpHashList[iHash];
		pEntry->pPrevHash = NULL;
		m_rgpHashList[iHash] = pEntry;
		if(pEntry->pNextHash)
			pEntry->pNextHash->pPrevHash = pEntry;

		// add to create list at tail
		pEntry->pPrevCreate = m_pCreateListTail;
		pEntry->pNextCreate = NULL;
		m_pCreateListTail = pEntry;
		if(pEntry->pPrevCreate)
			pEntry->pPrevCreate->pNextCreate = pEntry;
	}
	else // already exists
	{
		// make copy of string or array if necessary
		char *pchTmp = NULL;
		Double *pdTmp = NULL;
		if(ptValue->tType==PS_STRING)
		{
			pchTmp = new char [strlen(ptValue->pchData) + 1];
			if(pchTmp==NULL)
				return ERES_MEMORY;
			strcpy(pchTmp, ptValue->pchData);
		}
		else if(pEntry->tValue.tType==PS_ARRAY)
		{
			pdTmp = new Double [ptValue->iSize + 1];
			if(pdTmp==NULL)
				return ERES_MEMORY;
			memcpy(pdTmp, ptValue->pdData, ptValue->iSize * sizeof(Double));
		}

		// delete old string or array if present
		if(pEntry->tValue.tType==PS_STRING)
			delete pEntry->tValue.pchData;
		else if(pEntry->tValue.tType==PS_ARRAY)
			delete pEntry->tValue.pdData;

		// copy data
		pEntry->tValue = *ptValue;

		// assign string or array pointer
		if(ptValue->tType==PS_STRING)
			pEntry->tValue.pchData = pchTmp;
		else if(ptValue->tType==PS_ARRAY)
			pEntry->tValue.pdData = pdTmp;
	}

	return ERES_OK;
}

Int CxParamSet::DeleteParam(const char *pchName, Int iIndex)
{
	if(pchName==NULL)
		return ERES_PARAMS;
	//if(iIndex<-1)
	//	iIndex = -1;
	if(iIndex<0)
		iIndex = 0;

	Int iHash;
	TxParamEntry *pEntry = FindHashEntry(pchName, iIndex, &iHash);
	if(pEntry==NULL)
		return ERES_NOOBJ;
	
	if(pEntry==m_pCurrentEnum)
		m_pCurrentEnum = pEntry->pNextCreate;

	DeleteHashEntry(pEntry, iHash);

	return ERES_OK;
}

void CxParamSet::StartEnum()
{
	// set current enum to head of create list
	if(m_pCreateListTail==NULL)
		return;
	m_pCurrentEnum = m_pCreateListTail;
	while(m_pCurrentEnum->pPrevCreate)
		m_pCurrentEnum = m_pCurrentEnum->pPrevCreate;
}

Bool CxParamSet::NextEnum(char **ppchNameRtn, Int *piIndexRtn, TxParamValue *ptValueRtn)
{
	if(m_pCurrentEnum==NULL)
		return FALSE;

	if(ppchNameRtn)
		*ppchNameRtn = m_pCurrentEnum->pchName;
	if(piIndexRtn)
		*piIndexRtn = m_pCurrentEnum->iIndex;
	if(ptValueRtn)
		*ptValueRtn = m_pCurrentEnum->tValue;

	m_pCurrentEnum = m_pCurrentEnum->pNextCreate;
	return TRUE;
}

void CxParamSet::DeleteAll()
{
	Int i;
	for(i=0; i<PAR_HASHSIZE; i++)
	{
		TxParamEntry *pEntry;
		while((pEntry = m_rgpHashList[i])!=NULL)
			DeleteHashEntry(pEntry, i);
	}
	m_pCurrentEnum = NULL;
}

void CxParamSet::DeleteHashEntry(TxParamEntry *pEntry, Int iHash)
{
	// unlink from hash chain
	if(pEntry->pPrevHash)
		pEntry->pPrevHash->pNextHash = pEntry->pNextHash;
	else
		m_rgpHashList[iHash] = pEntry->pNextHash;
	if(pEntry->pNextHash)
		pEntry->pNextHash->pPrevHash = pEntry->pPrevHash;

	// unlink from create chain
	if(pEntry->pPrevCreate)
		pEntry->pPrevCreate->pNextCreate = pEntry->pNextCreate;
	if(pEntry->pNextCreate)
		pEntry->pNextCreate->pPrevCreate = pEntry->pPrevCreate;
	else
		m_pCreateListTail = pEntry->pPrevCreate;

	// delete
	if(pEntry->tValue.tType==PS_STRING)
		delete pEntry->tValue.pchData;
	else if(pEntry->tValue.tType==PS_ARRAY)
		delete pEntry->tValue.pdData;

	delete pEntry->pchName;
	delete pEntry;
}

TxParamEntry *CxParamSet::FindHashEntry(const char *pchName, Int iIndex, Int *piHashRtn)
{
	const char *pchPtr;
	Int iHash = (iIndex + 2) % PAR_HASHSIZE;
	for(pchPtr = pchName; *pchPtr!='\0'; pchPtr++)
		iHash = (iHash * 17 + *pchPtr) % PAR_HASHSIZE;

	*piHashRtn = iHash;

	TxParamEntry *pEntry = m_rgpHashList[iHash];
	for(; pEntry!=NULL; pEntry = pEntry->pNextHash)
		if(iIndex==pEntry->iIndex && strcmp(pchName, pEntry->pchName)==0)
			break;

	return pEntry;
}

Int CxParamSet::GetC()
{
	Int ch = getc(m_fp);
	if(ch=='\n')
		(*m_piErrLine)++;
	return ch;
}

void CxParamSet::UnGetC(Int ch)
{
	ungetc(ch, m_fp);
	if(ch=='\n')
		(*m_piErrLine)--;
}

Int CxParamSet::GetToken(char *pchBuf)
{
	SkipSpace();
	Int ch = GetC();
	if(ch==EOF)
		return ERES_EOF;
	if(ch=='/')
	{
		SkipComment();
		ch = GetC();
		if(ch==EOF)
			return ERES_EOF;
	}
	if(ch=='[' || ch==']' || ch=='=' || ch=='{' || ch=='}' || ch==','
		|| ch=='\"')
	{
		pchBuf[0] = (char)ch;
		pchBuf[1] = '\0';
		return ERES_OK;
	}
	pchBuf[0] = (char)ch;
	Int i;
	for(i=1;i<TOKEN_SIZE - 1;i++)
	{
		ch = GetC();
		if(ch==EOF)
		{
			pchBuf[i] = '\0';
			return ERES_EOF;
		}
		if(isspace(ch) || ch=='[' || ch==']' || ch=='=' || ch=='{'
			|| ch=='}' || ch==',' || ch=='\"')
		{
			pchBuf[i] = '\0';
			UnGetC(ch);
			return ERES_OK;
		}
		if(ch=='/' && SkipComment()!=ERES_NOOBJ)
		{
			pchBuf[i] = '\0';
			return ERES_OK;
		}
		pchBuf[i] = (char)ch;
	}
	pchBuf[i] = '\0';
	return ERES_OK;
}

Int CxParamSet::SkipSpace()
{
	Int ch;

	do {
		ch = GetC();
		if(ch==EOF)
			return ERES_EOF;
	} while(isspace(ch));

	UnGetC(ch);
	return ERES_OK;	
}

Int CxParamSet::SkipComment()
{
	// assumes we already read a '/'
	// space is skipped at end
	Int ch;
	do {
		ch = GetC();
		if(ch==EOF)
			return ERES_EOF;
		if(ch!='/' && ch!='*')
		{
			UnGetC(ch);
			return ERES_NOOBJ;
		}
		if(ch=='/')
		{
			do {
				ch = GetC();
				if(ch==EOF)
					return ERES_EOF;
			} while(ch!='\n');
		}
		else if(ch=='*')
		{
			Int iState = 0;
			do {
				ch = GetC();
				if(ch==EOF)
					return ERES_EOF;
				if(iState==0 && ch=='*')
					iState = 1;
				else if(iState==1)
				{
					if(ch=='/')
						iState = 2;
					else if(ch!='*')
						iState = 0;
				}
			} while(iState!=2);
		}
		if(SkipSpace()==ERES_EOF)
			return ERES_EOF;
		ch = GetC();
		if(ch==EOF)
			return ERES_EOF;
	} while(ch=='/');
	
	UnGetC(ch);
	return ERES_OK;
}

Int CxParamSet::ReadValue(char *pchBuf, TxParamValue *ptVal)
{
	if(pchBuf[0]=='\"')
	{
		// string value
		Int i, ch;
		for(i=0; i<TOKEN_SIZE - 2; i++)
		{
			ch = GetC();
			if(ch=='\"')
			{
				pchBuf[i] = '\0';
				//printf("\"%s\"\n", pchBuf);
				SetPVString(*ptVal, pchBuf);
				return ERES_OK;
			}
			if(ch==EOF)
				return ERES_FORMAT;
			pchBuf[i] = (char)ch;
		}
		return ERES_FORMAT;
	}
	else
	{
		char *pchEnd;
		Double dVal = strtod(pchBuf, &pchEnd);
		if(*pchEnd != '\0')
			return ERES_FORMAT;
		//printf("%g\n", dVal);
		SetPVDouble(*ptVal, dVal);
		return ERES_OK;
	}
}

Int CxParamSet::Load(FILE *fp, Int *piErrLine)
{
	char pchBuf[TOKEN_SIZE], pchName[TOKEN_SIZE];
	Int iInd;
	TxParamValue tVal;

	m_piErrLine = piErrLine;
	m_fp = fp;
	*m_piErrLine = 1;

	while(!(GetToken(pchBuf)==ERES_EOF))
	{
		strcpy(pchName, pchBuf);
		if(GetToken(pchBuf)==ERES_EOF)
			goto close_on_format;

		if(pchBuf[0]=='[')
		{
			if(GetToken(pchBuf)==ERES_EOF)
				goto close_on_format;
			char *pchEnd;
			iInd = strtol(pchBuf, &pchEnd, 10);
			if(*pchEnd != '\0')
				goto close_on_format;
			if(GetToken(pchBuf)==ERES_EOF)
				goto close_on_format;
			if(pchBuf[0]!=']')
				goto close_on_format;
			if(GetToken(pchBuf)==ERES_EOF)
				goto close_on_format;
		}
		else
			iInd = -1;
		if(pchBuf[0]!='=')
			goto close_on_format;
		
		if(GetToken(pchBuf)==ERES_EOF)
			goto close_on_format;
		if(pchBuf[0]=='{')
		{
			Int iCount = 0;
			Double rgdTemp[256];
			do {
				//printf("%s[%d] = (%d)", pchName, iInd, iCount);
				if(GetToken(pchBuf)==ERES_EOF)
					goto close_on_format;
				if(pchBuf[0]=='}')
					break;
				Int er = ReadValue(pchBuf, &tVal);
				if(er==ERES_OK)
				{
					if(tVal.tType==PS_DOUBLE)
						rgdTemp[iCount] = tVal.dData;
					else
						rgdTemp[iCount] = 0.0;
				}
				else
					return er;
				if(iCount<255)
					iCount++;

				if(GetToken(pchBuf)==ERES_EOF)
					goto close_on_format;
			} while(pchBuf[0]==',');

			if(pchBuf[0]!='}')
				goto close_on_format;
			
			SetPVArray(tVal, rgdTemp, iCount);
			Int er = SetParam(pchName, iInd, &tVal);
			if(er!=ERES_OK)
				return er;
		}
		else
		{
			//printf("%s[%d] = ", pchName, iInd);
			TxParamValue tVal;
			Int er = ReadValue(pchBuf, &tVal);
			if(er==ERES_OK)
				er = SetParam(pchName, iInd, &tVal);
			if(er!=ERES_OK)
				return er;
		}
	}

	return ERES_OK;

close_on_format:
	return ERES_FORMAT;
}


Int CxParamSet::GetDouble(char *pchName, Int iIndex, Double *pdValue)
{
	TxParamValue v;
	Int er = GetParam(pchName, iIndex, &v);
	if(er==ERES_OK)
	{
		if(v.tType==PS_DOUBLE)
			*pdValue = v.dData;
		else
			return ERES_PARAMS;
	}
	return er;
}

Int CxParamSet::GetString(char *pchName, Int iIndex, char **ppchVal)
{
	TxParamValue v;
	Int er = GetParam(pchName, iIndex, &v);
	if(er==ERES_OK)
	{
		if(v.tType==PS_STRING)
		{
//			char *pchBuf = new char [strlen(v.pchData)+1];
//			if(pchBuf==NULL)
//				return ERES_MEMORY;
//			strcpy(pchBuf, v.pchData);
//			*ppchVal = pchBuf;
			*ppchVal = v.pchData; // swinder 991211
		}
		else
			return ERES_PARAMS;
	}
	return er;
}

Int CxParamSet::GetArray(char *pchName, Int iIndex, Double **ppdVal, Int *piSize)
{
	TxParamValue v;
	Int er = GetParam(pchName, iIndex, &v);
	if(er==ERES_OK)
	{
		if(v.tType==PS_ARRAY)
		{
//			Double *pdBuf = NULL;
//			if(v.iSize>0)
//			{
//				pdBuf = new Double [v.iSize];
//				if(pdBuf==NULL)
//					return ERES_MEMORY;
//				memcpy(pdBuf, v.pdData, v.iSize * sizeof(Double));
//			}
//			*ppdVal = pdBuf;
			*ppdVal = v.pdData; // swinder 991211
			*piSize = v.iSize;
		}
		else
			return ERES_PARAMS;
	}
	return er;
}

void CxParamSet::Dump(FILE *fp)
{
	StartEnum();

	char *pchName;
	int iInd, iCount;
	TxParamValue tVal;

	iCount = 0;
	while(NextEnum(&pchName, &iInd, &tVal))
	{
		iCount++;
		if(iInd<0)
			fprintf(fp, "%s = ", pchName);
		else
			fprintf(fp, "%s[%d] = ", pchName, iInd);
		if(tVal.tType==PS_DOUBLE)
			fprintf(fp, "%g", tVal.dData);
		else if(tVal.tType==PS_STRING)
			fprintf(fp, "\"%s\"", tVal.pchData);
		else
		{
			fprintf(fp, "{");
			int i;
			for(i = 0; i<tVal.iSize; i++)
			{
				fprintf(fp, "%g", tVal.pdData[i]);
				if(i+1<tVal.iSize)
					fprintf(fp, ", ");
			}
			fprintf(fp, "}");
		}
		fprintf(fp, "\n");
	}

	fprintf(fp,"\n// Total: %d parameters.\n\n", iCount);
}
