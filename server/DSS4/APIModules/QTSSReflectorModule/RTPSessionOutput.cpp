/*
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * Copyright (c) 1999-2001 Apple Computer, Inc.  All Rights Reserved. The
 * contents of this file constitute Original Code as defined in and are
 * subject to the Apple Public Source License Version 1.2 (the 'License').
 * You may not use this file except in compliance with the License.  Please
 * obtain a copy of the License at http://www.apple.com/publicsource and
 * read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.  Please
 * see the License for the specific language governing rights and
 * limitations under the License.
 *
 *
 * @APPLE_LICENSE_HEADER_END@
 *
 */
/*
	File:		RTSPReflectorOutput.cpp

	Contains:	Implementation of object in .h file
					

*/


#include "RTPSessionOutput.h"


#include <errno.h>


#if DEBUG 
#define RTP_SESSION_DEBUGGING 0
#else
#define RTP_SESSION_DEBUGGING 0
#endif

// ATTRIBUTES

static QTSS_AttributeID 	sNextSeqNumAttr				= qtssIllegalAttrID;
static QTSS_AttributeID 	sSeqNumOffsetAttr			= qtssIllegalAttrID;
static QTSS_AttributeID 	sLastQualityChangeAttr		= qtssIllegalAttrID;

RTPSessionOutput::RTPSessionOutput(QTSS_ClientSessionObject inClientSession, ReflectorSession* inReflectorSession,
									QTSS_Object serverPrefs, QTSS_AttributeID inCookieAddrID)
: 	fClientSession(inClientSession),
	fReflectorSession(inReflectorSession),
	fCookieAttrID(inCookieAddrID)
{
	// create a bookmark for each stream we'll reflect
	this->InititializeBookmarks( inReflectorSession->GetNumStreams() );
}

void RTPSessionOutput::Register()
{
	// Add some attributes to QTSS_RTPStream dictionary for thinning
	static char*		sNextSeqNum 			= "qtssNextSeqNum";
	static char*		sSeqNumOffset			= "qtssSeqNumOffset";
	static char*		sLastQualityChange		= "qtssLastQualityChange";

	(void)QTSS_AddStaticAttribute(qtssRTPStreamObjectType, sNextSeqNum, NULL, qtssAttrDataTypeUInt16);
	(void)QTSS_IDForAttr(qtssRTPStreamObjectType, sNextSeqNum, &sNextSeqNumAttr);

	(void)QTSS_AddStaticAttribute(qtssRTPStreamObjectType, sSeqNumOffset, NULL, qtssAttrDataTypeUInt16);
	(void)QTSS_IDForAttr(qtssRTPStreamObjectType, sSeqNumOffset, &sSeqNumOffsetAttr);

	(void)QTSS_AddStaticAttribute(qtssRTPStreamObjectType, sLastQualityChange, NULL, qtssAttrDataTypeSInt64);
	(void)QTSS_IDForAttr(qtssRTPStreamObjectType, sLastQualityChange, &sLastQualityChangeAttr);
}


QTSS_Error	RTPSessionOutput::WritePacket(StrPtrLen* inPacket, void* inStreamCookie, UInt32 inFlags, SInt64 packetLatenessInMSec, SInt64* timeToSendThisPacketAgain)
{
	QTSS_RTPSessionState* 	theState = NULL;
	UInt32 					theLen = 0;
	QTSS_Error	 			writeErr = QTSS_NoErr;
	
	(void)QTSS_GetValuePtr(fClientSession, qtssCliSesState, 0, (void**)&theState, &theLen);
	if (*theState != qtssPlayingState)
		return QTSS_NoErr;
	
	//¥TODO -- currently the code bails out when it gets a write err on the first stream
	// that wants a given packet.  It -is- possible that  a client could have multiple
	// streams for the same track.  In that case, they might see duplicate packets on 
	// a retrasnmission caused by one stream or the other.

	//make sure all RTP streams with this ID see this packet
	QTSS_RTPStreamObject* theStream = NULL;
						
	for (int z = 0; QTSS_GetValuePtr(fClientSession, qtssCliSesStreamObjects, z, (void**)&theStream, &theLen) == QTSS_NoErr; z++)
	{
		void** theStreamCookie = NULL;
		QTSS_RTPPayloadType* thePayloadType = NULL;
		(void)QTSS_GetValue(*theStream, fCookieAttrID, 0, (void**)&theStreamCookie, &theLen);
		if ((theStreamCookie != NULL) && (*theStreamCookie == inStreamCookie))
		{
			(void)QTSS_GetValuePtr(*theStream, qtssRTPStrPayloadType, 0, (void**)&thePayloadType, &theLen);
			//We may have thinned this client in the past. If that's the case,
			//the sequence number of this packet needs to be adjusted, but only
			//for this client, so we need to make sure we can restore the old sequence
			//number after we mess around with it.
			UInt16 theSeqNumber = this->GetPacketSeqNumber(inPacket);
			
			if ((thePayloadType == NULL) ||
				(*thePayloadType != qtssVideoPayloadType) ||
				(inFlags == qtssWriteFlagsIsRTCP) ||
				(!this->PacketShouldBeThinned(*theStream, inPacket)))
			{
				QTSS_PacketStruct thePacket;
				thePacket.packetData = inPacket->Ptr;
				thePacket.packetTransmitTime = OS::Milliseconds() - packetLatenessInMSec;
				writeErr = QTSS_Write(*theStream, &thePacket, inPacket->Len, NULL, inFlags | qtssWriteFlagsWriteBurstBegin);
				
				if (writeErr == QTSS_WouldBlock)
				{
					//
					// We are flow controlled. See if we know when flow control will be lifted and report that
					*timeToSendThisPacketAgain = thePacket.suggestedWakeupTime;
				}
				#if RTP_SESSION_DEBUGGING
				if ( writeErr != QTSS_WouldBlock )
					::printf( "rtp::WritePacket: sess 0x%lx payload %li, sequence %li\n", (long)this, (long)*thePayloadType, (long)theSeqNumber );
				#endif
			}

			//Reset the old sequence number
			this->SetPacketSeqNumber(inPacket, theSeqNumber);

		}

		if ( writeErr != QTSS_NoErr )
			break;
	}
	
	return writeErr;
}


UInt16 RTPSessionOutput::GetPacketSeqNumber(StrPtrLen* inPacket)
{
	if (inPacket->Len < 4)
		return 0;
	
	//The RTP seq number is the second short of the packet
	UInt16* seqNumPtr = (UInt16*)inPacket->Ptr;
	return ntohs(seqNumPtr[1]);
}

void RTPSessionOutput::SetPacketSeqNumber(StrPtrLen* inPacket, UInt16 inSeqNumber)
{
	if (inPacket->Len < 4)
		return;

	//The RTP seq number is the second short of the packet
	UInt16* seqNumPtr = (UInt16*)inPacket->Ptr;
	seqNumPtr[1] = htons(inSeqNumber);
}


Bool16 RTPSessionOutput::PacketShouldBeThinned(QTSS_RTPStreamObject inStream, StrPtrLen* inPacket)
{
	static UInt16 sZero = 0;
	return false;
	
	//This function determines whether the packet should be dropped.
	//It also adjusts the sequence number if necessary

	if (inPacket->Len < 4)
		return false;
	
	UInt16 curSeqNum = this->GetPacketSeqNumber(inPacket);
	UInt32* curQualityLevel = NULL;
	UInt16* nextSeqNum = NULL;
	UInt16* theSeqNumOffset = NULL;
	SInt64* lastChangeTime = NULL;
	
	UInt32 theLen = 0;
	(void)QTSS_GetValuePtr(inStream, qtssRTPStrQualityLevel, 0, (void**)&curQualityLevel, &theLen);
	if ((curQualityLevel == NULL) || (theLen != sizeof(UInt32)))
		return false;
	(void)QTSS_GetValuePtr(inStream, sNextSeqNumAttr, 0, (void**)&nextSeqNum, &theLen);
	if ((nextSeqNum == NULL) || (theLen != sizeof(UInt16)))
	{
		nextSeqNum = &sZero;
		(void)QTSS_SetValue(inStream, sNextSeqNumAttr, 0, nextSeqNum, sizeof(UInt16));
	}
	(void)QTSS_GetValuePtr(inStream, sSeqNumOffsetAttr, 0, (void**)&theSeqNumOffset, &theLen);
	if ((theSeqNumOffset == NULL) || (theLen != sizeof(UInt16)))
	{
		theSeqNumOffset = &sZero;
		(void)QTSS_SetValue(inStream, sSeqNumOffsetAttr, 0, theSeqNumOffset, sizeof(UInt16));
	}
	UInt16 newSeqNumOffset = *theSeqNumOffset;
	
	(void)QTSS_GetValuePtr(inStream, sLastQualityChangeAttr, 0, (void**)&lastChangeTime, &theLen);
	if ((lastChangeTime == NULL) || (theLen != sizeof(SInt64)))
	{	static SInt64 startTime = 0;
		lastChangeTime = &startTime;
		(void)QTSS_SetValue(inStream, sLastQualityChangeAttr, 0, lastChangeTime, sizeof(SInt64));
	}
	
	SInt64 timeNow = OS::Milliseconds();
	if (*lastChangeTime == 0 || *curQualityLevel == 0) 
		*lastChangeTime =timeNow;
	
	if (*curQualityLevel > 0 && ((*lastChangeTime + 30000) < timeNow) ) // 30 seconds between reductions
	{	*curQualityLevel -= 1; // reduce quality value.  If we quality doesn't change then we may have hit some steady state which we can't get out of without thinning or increasing the quality
		*lastChangeTime =timeNow; 
		//printf("RTPSessionOutput set quality to %lu\n",*curQualityLevel);
	}

	//Check to see if we need to drop to audio only
	if ((*curQualityLevel >= ReflectorSession::kAudioOnlyQuality) &&
		(*nextSeqNum == 0))
	{
//#if REFLECTOR_THINNING_DEBUGGING || RTP_SESSION_DEBUGGING
		printf(" *** Reflector Dropping to audio only *** \n");
//#endif
		//All we need to do in this case is mark the sequence number of the first dropped packet
		(void)QTSS_SetValue(inStream, sNextSeqNumAttr, 0, &curSeqNum, sizeof(UInt16));	
		 *lastChangeTime =timeNow;	
	}
	
	
	//Check to see if we can reinstate video
	if ((*curQualityLevel == ReflectorSession::kNormalQuality) && (*nextSeqNum != 0))
	{
		//Compute the offset amount for each subsequent sequence number. This offset will
		//alter the sequence numbers so that they increment normally (providing the illusion to the
		//client that there are no missing packets)
		newSeqNumOffset = (*theSeqNumOffset) + (curSeqNum - (*nextSeqNum));
		(void)QTSS_SetValue(inStream, sSeqNumOffsetAttr, 0, &newSeqNumOffset, sizeof(UInt16));
		(void)QTSS_SetValue(inStream, sNextSeqNumAttr, 0, &sZero, sizeof(UInt16));
#if REFLECTOR_THINNING_DEBUGGING || RTP_SESSION_DEBUGGING
		printf("Reflector Reinstating video to probe the client. Offset = %li\n",  (long)newSeqNumOffset );
#endif
	}
	
	//tell the caller whether to drop this packet or not.
	if (*curQualityLevel >= ReflectorSession::kAudioOnlyQuality)
		return true;
	else
	{
		//Adjust the sequence number of the current packet based on the offset, if any
		curSeqNum -= newSeqNumOffset;
		this->SetPacketSeqNumber(inPacket, curSeqNum);
		return false;
	}
}

void RTPSessionOutput::TearDown()
{
	QTSS_CliSesTeardownReason reason = qtssCliSesTearDownBroadcastEnded;
	(void)QTSS_SetValue(fClientSession, qtssCliTeardownReason, 0, &reason, sizeof(reason));		
	(void)QTSS_Teardown(fClientSession);
}


