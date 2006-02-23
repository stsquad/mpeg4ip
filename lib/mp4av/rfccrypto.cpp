/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000-2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		  dmackie@cisco.com
 *		Mark Baugher		  mbaugher@cisco.com
 *              Alix Marchandise-Franquet alix@cisco.com
 *              Alex Vanzella             alexv@cisco.com
 */


#include <mp4av_common.h>
// definitions
// mjb: ismacryp declarations
#define ISMACRYP_CRYPTO_SUITE 0                 //index into settings array
#define ISMACRYP_CRYPTO_SUITE_DEFAULT 0
#define ISMACRYP_IV_LENGTH 1                    //index into settings array
#define ISMACRYP_IV_LENGTH_DEFAULT 4
#define ISMACRYP_IV_DELTA_LENGTH 2              //index into settings array
#define ISMACRYP_IV_DELTA_LENGTH_DEFAULT 0
#define ISMACRYP_SELECTIVE_ENCRYPTION 3         //index into settings array
#define ISMACRYP_SELECTIVE_ENCRYPTION_DEFAULT 0
#define ISMACRYP_KEY_INDICATOR_LENGTH 4         //index into settings array
#define ISMACRYP_KEY_INDICATOR_LENGTH_DEFAULT 0
#define ISMACRYP_KEY_INDICATOR_PER_AU 5         //index into settings array
#define ISMACRYP_KEY_INDICATOR_PER_AU_DEFAULT 0
#define ISMACRYP_SALT_LENGTH 6                  //index into settings array
#define ISMACRYP_SALT_LENGTH_DEFAULT 8          //number of bytes in salt key
#define ISMACRYP_KEY_LENGTH 7                   //index into settings array
#define ISMACRYP_KEY_LENGTH_DEFAULT 16          //number of bytes in master key
#define ISMACRYP_KEY_COUNT 8                    //index into settings array
#define ISMACRYP_MAX_KEYS 6                     //max number of salts and keys
#define ISMACRYP_MAX_KEY_LENGTH ISMACRYP_KEY_LENGTH_DEFAULT
#define ISMACRYP_MAX_SALT_LENGTH ISMACRYP_SALT_LENGTH_DEFAULT
// default scheme is "iAEC"
#define ISMACRYP_DEFAULT_SCHEME_HEX  0x69414543

#define ISMACRYP_SDP_CONFIG_MAX_LEN                256
#define ISMACRYP_SDP_ADDITIONAL_CONFIG_MAX_LEN     256
// change this if any of the following strings is longer than 64
#define ISMACRYP_SDP_CONFIG_MAX_SUBSTR_LEN 64
// if any of these strings is longer than ISMACRYP_SDP_CONFIG_MAX_SUBSTR_LEN,
// change ISMACRYP_SDP_CONFIG_MAX_SUBSTR_LEN to reflect longest string + 1 for '\0'
// pay special attention to the key string (str 7).
#define ISMACRYP_SDP_CONFIG_STR_1    " ISMACrypCryptoSuite=AES_CTR_128;"
#define ISMACRYP_SDP_CONFIG_STR_2    " ISMACrypIVLength=%u;"
#define ISMACRYP_SDP_CONFIG_STR_3    " ISMACrypIVDeltaLength=%u;"
#define ISMACRYP_SDP_CONFIG_STR_4    " ISMACrypSelectiveEncryption=%u;"
#define ISMACRYP_SDP_CONFIG_STR_5    " ISMACrypKeyIndicatorLength=%u;"
#define ISMACRYP_SDP_CONFIG_STR_6    " ISMACrypKeyIndicatorPerAU=%u;"
#define ISMACRYP_SDP_CONFIG_STR_7    " ISMACrypKey=(key)%s/%u"

// These structs hold ISMACRYP protocol variables related to an
// encryption session. Used to construct SDP signalling information
// for the session.
typedef struct ismacryp_config_table {
        u_int8_t        lifetime;
        u_int8_t        settings[9];
        u_int8_t*       salts[ISMACRYP_MAX_KEYS];
        u_int8_t*       keys[ISMACRYP_MAX_KEYS];
        char            indices[ISMACRYP_MAX_KEYS][255];
} ISMACrypConfigTable_t;
typedef struct ismaCrypSampleHdrDataInfo {
  u_int8_t hasEncField; // if selective encryption
  u_int8_t isEncrypted;
  u_int8_t hasIVField;
  u_int8_t hasKIField;
} ismaCrypSampleHdrDataInfo_t;


// forward declarations.
static bool MP4AV_RfcCryptoConcatenator(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	u_int8_t samplesThisHint, 
	MP4SampleId* pSampleIds, 
	MP4Duration hintDuration,
	u_int16_t maxPayloadSize,
	mp4av_ismacrypParams *icPp,
	bool isInterleaved);

static bool MP4AV_RfcCryptoFragmenter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	MP4SampleId sampleId, 
	u_int32_t sampleSize, 
	MP4Duration sampleDuration,
	u_int16_t maxPayloadSize,
	mp4av_ismacrypParams *icPp);

/* start sdp code */

// ismac_table will contain all the setting to create the SDP
// initialize this table with settings from the ismacryp library 
static bool InitISMACrypConfigTable (ISMACrypConfigTable_t* ismac_table,
	mp4av_ismacrypParams *icPp)
{
	u_int32_t        	fourCC;
	u_int8_t	        yes;

	ismac_table->settings[ISMACRYP_KEY_COUNT]=icPp->key_count;

	if(!ismac_table || ismac_table->salts[0] || ismac_table->keys[0])
		return false;

	if(!(ismac_table->salts[0]=(u_int8_t*)malloc(icPp->salt_len))) 
          return false;

	if(!(ismac_table->keys[0]=(u_int8_t*)malloc(icPp->key_len))){
        	free(ismac_table->salts[0]);
        	return false;
        }

        ismac_table->settings[ISMACRYP_KEY_LENGTH] = icPp->key_len;
	ismac_table->settings[ISMACRYP_SALT_LENGTH] = icPp->salt_len;
	ismac_table->lifetime = icPp->key_life;
        memcpy(ismac_table->keys[0],icPp->key,icPp->key_len);
        memcpy(ismac_table->salts[0],icPp->salt,icPp->salt_len);
        ismac_table->settings[ISMACRYP_KEY_INDICATOR_LENGTH] = icPp->key_ind_len;

        yes = icPp->key_ind_perau;
	if (!yes) ismac_table->settings[ISMACRYP_KEY_INDICATOR_PER_AU] = 0;
	else ismac_table->settings[ISMACRYP_KEY_INDICATOR_PER_AU] = 1;

	yes = icPp->selective_enc;
	if (!yes) ismac_table->settings[ISMACRYP_SELECTIVE_ENCRYPTION] = 0;
	else ismac_table->settings[ISMACRYP_SELECTIVE_ENCRYPTION] = 1;

        ismac_table->settings[ISMACRYP_IV_DELTA_LENGTH] = icPp->delta_iv_len;
	ismac_table->settings[ISMACRYP_IV_LENGTH] = icPp->delta_iv_len;

        fourCC = icPp->scheme;
	if (fourCC == ISMACRYP_DEFAULT_SCHEME_HEX)
		ismac_table->settings[ISMACRYP_CRYPTO_SUITE]=0;
	else return false;

	return true; 
}

// Checks validity of the ismacryp SDP settings returned from the ismacryp library
static bool MP4AV_RfcCryptoPolicyOk (ISMACrypConfigTable_t* ismac_table)
{
	if(ismac_table->settings[ISMACRYP_CRYPTO_SUITE] 
		!= ISMACRYP_CRYPTO_SUITE_DEFAULT) 
		return false;
	if(ismac_table->settings[ISMACRYP_IV_LENGTH] >8)  
		return false;
	if(ismac_table->settings[ISMACRYP_IV_DELTA_LENGTH] > 2) 
		return false;
	if(ismac_table->settings[ISMACRYP_SELECTIVE_ENCRYPTION] > 1)
		return false;
	if(ismac_table->settings[ISMACRYP_KEY_INDICATOR_PER_AU] > 1)
		return false;
	if((ismac_table->settings[ISMACRYP_SALT_LENGTH] 
			!= ISMACRYP_SALT_LENGTH_DEFAULT) 
		|| (ismac_table->settings[ISMACRYP_KEY_LENGTH] 
			!= ISMACRYP_KEY_LENGTH_DEFAULT)) return false;
	return true;
}

// assemble the ismacryp SDP string into sISMACrypConfig
static bool MP4AV_RfcCryptoConfigure(ISMACrypConfigTable_t* ismac_table,
	char** sISMACrypConfig)
{
        if (ismac_table == NULL )
           return false;

        *sISMACrypConfig = (char *)malloc(ISMACRYP_SDP_CONFIG_MAX_LEN);
        if (*sISMACrypConfig == NULL )
           return false;

        char temp[ISMACRYP_SDP_CONFIG_MAX_SUBSTR_LEN];
        int lenstr1, lenstr2, totlen;

        // first string is ISMACrypCryptoSuite
        totlen = strlen(ISMACRYP_SDP_CONFIG_STR_1) + 1; // add 1 for '\0'
        if ( totlen < ISMACRYP_SDP_CONFIG_MAX_LEN - 1 ) 
	  snprintf(*sISMACrypConfig, totlen, "%s", ISMACRYP_SDP_CONFIG_STR_1);
        else {
          free(*sISMACrypConfig);
          return false;
        }

        // second string is ISMACrypIVLength
        snprintf(temp, ISMACRYP_SDP_CONFIG_MAX_SUBSTR_LEN,
                       ISMACRYP_SDP_CONFIG_STR_2, 
                       ismac_table->settings[ISMACRYP_IV_LENGTH]);
        lenstr1 = strlen(*sISMACrypConfig); 
        lenstr2 = strlen(temp) + 1; // add 1 for '\0'
        totlen = lenstr1 + lenstr2;
        if ( totlen < ISMACRYP_SDP_CONFIG_MAX_LEN )
          snprintf( *sISMACrypConfig+lenstr1,lenstr2, "%s", temp);
        else {
          free(*sISMACrypConfig);
          return false;
        }

        // third string is ISMACrypIVDeltaLength
        snprintf(temp, ISMACRYP_SDP_CONFIG_MAX_SUBSTR_LEN,
                       ISMACRYP_SDP_CONFIG_STR_3, 
                       ismac_table->settings[ISMACRYP_IV_DELTA_LENGTH]);
        lenstr1 = strlen(*sISMACrypConfig); 
        lenstr2 = strlen(temp) + 1; // add 1 for '\0'
        totlen = lenstr1 + lenstr2;
        if ( totlen < ISMACRYP_SDP_CONFIG_MAX_LEN )
          snprintf( *sISMACrypConfig+lenstr1,lenstr2, "%s", temp);
        else {
          free(*sISMACrypConfig);
          return false;
        }

        // fourth string is ISMACrypSelectiveEncryption
        snprintf(temp, ISMACRYP_SDP_CONFIG_MAX_SUBSTR_LEN,
                       ISMACRYP_SDP_CONFIG_STR_4, 
                       ismac_table->settings[ISMACRYP_SELECTIVE_ENCRYPTION]);
        lenstr1 = strlen(*sISMACrypConfig); 
        lenstr2 = strlen(temp) + 1; // add 1 for '\0'
        totlen = lenstr1 + lenstr2;
        if ( totlen < ISMACRYP_SDP_CONFIG_MAX_LEN )
          snprintf( *sISMACrypConfig+lenstr1,lenstr2, "%s", temp);
        else {
          free(*sISMACrypConfig);
          return false;
        }

        // fifth string is ISMACrypKeyIndicatorLength
        snprintf(temp, ISMACRYP_SDP_CONFIG_MAX_SUBSTR_LEN,
                       ISMACRYP_SDP_CONFIG_STR_5, 
                       ismac_table->settings[ISMACRYP_KEY_INDICATOR_LENGTH]);
        lenstr1 = strlen(*sISMACrypConfig); 
        lenstr2 = strlen(temp) + 1; // add 1 for '\0'
        totlen = lenstr1 + lenstr2;
        if ( totlen < ISMACRYP_SDP_CONFIG_MAX_LEN )
          snprintf( *sISMACrypConfig+lenstr1,lenstr2, "%s", temp);
        else {
          free(*sISMACrypConfig);
          return false;
        }

        // sixth string is ISMACrypKeyIndicatorPerAU
        snprintf(temp, ISMACRYP_SDP_CONFIG_MAX_SUBSTR_LEN,
                       ISMACRYP_SDP_CONFIG_STR_6, 
                       ismac_table->settings[ISMACRYP_KEY_INDICATOR_PER_AU]);
        lenstr1 = strlen(*sISMACrypConfig); 
        lenstr2 = strlen(temp) + 1; // add 1 for '\0'
        totlen = lenstr1 + lenstr2;
        if ( totlen < ISMACRYP_SDP_CONFIG_MAX_LEN )
          snprintf( *sISMACrypConfig+lenstr1,lenstr2, "%s", temp);
        else {
          free(*sISMACrypConfig);
          return false;
        }

	// convert the concatenated, binary keys to Base 64
	u_int8_t  keymat[ISMACRYP_KEY_LENGTH_DEFAULT+ISMACRYP_SALT_LENGTH_DEFAULT]; 
        memcpy(keymat,ismac_table->salts[0],ISMACRYP_SALT_LENGTH_DEFAULT);
        memcpy(&keymat[ISMACRYP_SALT_LENGTH_DEFAULT],ismac_table->keys[0],ISMACRYP_KEY_LENGTH_DEFAULT);

	char* base64keymat = MP4BinaryToBase64((u_int8_t*)keymat, 
                                     ISMACRYP_SALT_LENGTH_DEFAULT + ISMACRYP_KEY_LENGTH_DEFAULT);

        // seventh string is ISMACrypKey
        snprintf(temp, ISMACRYP_SDP_CONFIG_MAX_SUBSTR_LEN,
                       ISMACRYP_SDP_CONFIG_STR_7, 
                       base64keymat, ismac_table->lifetime);
        lenstr1 = strlen(*sISMACrypConfig); 
        lenstr2 = strlen(temp) + 1; // add 1 for '\0'
        totlen = lenstr1 + lenstr2;
        if ( totlen < ISMACRYP_SDP_CONFIG_MAX_LEN )
          snprintf( *sISMACrypConfig+lenstr1,lenstr2, "%s", temp);
        else {
          free(*sISMACrypConfig);
	  free(base64keymat);
          return false;
        }

	free(base64keymat);
	return true;
}
/* end sdp code */

// MP4AV_CryptoAudioConsecutiveHinter: figures out how to put the samples into packets
// When a hint is defined (the sample ids of the hint are in pSampleIds)
// call MP4AV_RfcCryptoConcatenator to create it 
// Note: This is a modified clone of MP4AV_AudioConsecutiveHinter in audio_hinters.cpp
static bool MP4AV_CryptoAudioConsecutiveHinter(MP4FileHandle mp4File, 
					       MP4TrackId mediaTrackId, 
					       MP4TrackId hintTrackId,
					       MP4Duration sampleDuration, 
					       u_int8_t perPacketHeaderSize,
					       u_int8_t perSampleHeaderSize,
					       u_int8_t maxSamplesPerPacket,
					       u_int16_t maxPayloadSize,
					       MP4AV_AudioSampleSizer pSizer,
					       mp4av_ismacrypParams *icPp)
{
  bool rc;
  u_int32_t numSamples = 
    MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);

  u_int16_t bytesThisHint = perPacketHeaderSize;
  u_int16_t samplesThisHint = 0;
  MP4SampleId* pSampleIds = 
    new MP4SampleId[maxSamplesPerPacket];

  for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) {

    u_int32_t sampleSize = 
      (*pSizer)(mp4File, mediaTrackId, sampleId);

    // sample won't fit in this packet
    // or we've reached the limit on samples per packet
    if ((int16_t)(sampleSize + perSampleHeaderSize) 
	> maxPayloadSize - bytesThisHint 
	|| samplesThisHint == maxSamplesPerPacket) {

      if (samplesThisHint > 0) {
	rc = MP4AV_RfcCryptoConcatenator(mp4File, mediaTrackId, hintTrackId,
					 samplesThisHint, pSampleIds,
					 samplesThisHint * sampleDuration,
					 maxPayloadSize, icPp, false);

	if (!rc) {
          delete [] pSampleIds;
	  return false;
	}
      }

      // start a new hint 
      samplesThisHint = 0;
      bytesThisHint = perPacketHeaderSize;

      // fall thru
    }

    // sample is less than remaining payload size
    if ((int16_t)(sampleSize + perSampleHeaderSize)
	<= maxPayloadSize - bytesThisHint) {

      // add it to this hint
      bytesThisHint += (sampleSize + perSampleHeaderSize);
      pSampleIds[samplesThisHint++] = sampleId;

    } else { 
      // jumbo frame, need to fragment it
      rc = MP4AV_RfcCryptoFragmenter(mp4File, mediaTrackId, hintTrackId, sampleId, 
			  sampleSize, sampleDuration, maxPayloadSize, icPp);
	
      if (!rc) {
        delete [] pSampleIds;
	return false;
      }

      // start a new hint 
      samplesThisHint = 0;
      bytesThisHint = perPacketHeaderSize;
    }
  }

  if (samplesThisHint > 0) {
    rc = MP4AV_RfcCryptoConcatenator(mp4File, mediaTrackId, hintTrackId,
				     samplesThisHint, pSampleIds,
				     samplesThisHint * sampleDuration,
				     maxPayloadSize, icPp, false);
    if (!rc) {
      delete [] pSampleIds;
      return false;
    }
  }

  delete [] pSampleIds;

  return true;
}

// MP4AV_CryptoAudioInterleaveHinter: figures out how to put the samples into packets
// When a hint is defined (the sample ids of the hint are in pSampleIds)
// call MP4AV_RfcCryptoConcatenator to create it. The samples are interleaved. 
// Note: This is a modified clone of MP4AV_AudioInterleaveHinter in audio_hinters.cpp
static bool MP4AV_CryptoAudioInterleaveHinter(MP4FileHandle mp4File, 
					      MP4TrackId mediaTrackId, 
					      MP4TrackId hintTrackId,
					      MP4Duration sampleDuration, 
					      u_int8_t stride, 
					      u_int8_t bundle,
					      u_int16_t maxPayloadSize,
					      mp4av_ismacrypParams *icPp)
{
  bool rc;
  u_int32_t numSamples = 
    MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);
  
  MP4SampleId* pSampleIds = new MP4SampleId[bundle];

  uint32_t sampleIds = 0;
  for (u_int32_t i = 1; i <= numSamples; i += stride * bundle) {
    for (u_int32_t j = 0; j < stride; j++) {
      u_int32_t k;
      for (k = 0; k < bundle; k++) {
	
	MP4SampleId sampleId = i + j + (k * stride);

	// out of samples for this bundle
	if (sampleId > numSamples) {
	  break;
	}

	// add sample to this hint
	pSampleIds[k] = sampleId;
	sampleIds++;
      }

      if (k == 0) {
	break;
      }

      // compute hint duration
      // note this is used to control the RTP timestamps 
      // that are emitted for the packet,
      // it isn't the actual duration of the samples in the packet
      MP4Duration hintDuration;
      if (j + 1 == stride) {
	// at the end of the track
	if (i + (stride * bundle) > numSamples) {
	  hintDuration = ((numSamples - i) - j) * sampleDuration;
	  if (hintDuration == 0) {
	    hintDuration = sampleDuration;
	  }
	} else {
	  hintDuration = ((stride * bundle) - j) * sampleDuration;
	}
      } else {
	hintDuration = sampleDuration;
      }

      // write hint
      rc = MP4AV_RfcCryptoConcatenator(mp4File, mediaTrackId, hintTrackId, k, 
				       pSampleIds, hintDuration, maxPayloadSize, 
				       icPp, true);
      sampleIds = 0;
      
      if (!rc) {
        delete [] pSampleIds;
	return false;
      }
    }
  }

  delete [] pSampleIds;
  
  return true;
}

// returns the size of the ismacryp sample header:
// selective encryption flag, IV, key indicator
static u_int32_t MP4AV_GetIsmaCrypSampleHdrSize (
					ismaCrypSampleHdrDataInfo_t iCrypSampleHdrInfo, 
					u_int8_t IVlen, 
				        u_int8_t KIlen)
{
  // the ismaCryp sample header always has the IV and the KI
  u_int32_t iCrypSampleHdrSize = (iCrypSampleHdrInfo.hasEncField ? 1: 0) 
    + IVlen  + KIlen;
  return iCrypSampleHdrSize;
}

// get the key indicator length, the IV length and the selective 
// encryption flag from the iSFM atom in the file
static bool MP4AV_GetiSFMSettings(MP4FileHandle mp4File, 
				  MP4TrackId mediaTrackId,
				  u_int8_t *useSelectiveEnc, 
				  u_int8_t *KIlen, 
				  u_int8_t *IVlen,
				  bool isAudio)
{
  const char *part1    = "mdia.minf.stbl.stsd.";
  const char *part2a   = "enca";
  const char *part2v   = "encv";
  const char *part3    = ".sinf.schi.iSFM.";
  const char *part4se  = "selective-encryption";
  const char *part4kil = "key-indicator-length";
  const char *part4ivl = "IV-length";
  // add 1 for null char
  int   pathlen = strlen(part1)+strlen(part2a)+strlen(part3)+strlen(part4se) + 1;
  char  *path;

  path = (char *)malloc(pathlen);
  
  // get selective encryption setting from the file (iSFM atom)
  snprintf(path, 
           pathlen,
	   "%s%s%s%s",
           part1,
	   isAudio ? part2a : part2v,
           part3, part4se);
  uint64_t temp;
  MP4GetTrackIntegerProperty(mp4File, mediaTrackId, 
			     path, &temp);
  *useSelectiveEnc = temp;
 
  // get the key indicator length from the file (iSFM atom)
  snprintf(path, 
           pathlen,
	   "%s%s%s%s",
           part1,
	   isAudio ? part2a : part2v,
           part3, part4kil);
  MP4GetTrackIntegerProperty(mp4File, mediaTrackId, 
			     path, &temp);
  *KIlen = temp;

  // get the IV length from the file (iSFM atom)
  snprintf(path, 
           pathlen,
	   "%s%s%s%s",
           part1,
	   isAudio ? part2a : part2v,
           part3, part4ivl);
  MP4GetTrackIntegerProperty(mp4File, mediaTrackId, 
			     path, &temp);
  *IVlen = temp;

  free(path);
  return true;
}

// gets the AU hdr length, and the AU sample header data information
static bool MP4AV_ProcessIsmaCrypHdrs(MP4FileHandle mp4File, 
				      MP4TrackId mediaTrackId, 
				      u_int8_t samplesThisHint, 
				      MP4SampleId* pSampleIds,
				      u_int8_t useSelectiveEnc, 
				      u_int8_t KIlen, 
				      u_int8_t IVlen,
				      u_int8_t *deltaIVLen,
				      u_int16_t *iCrypAUHdrLen,
				      ismaCrypSampleHdrDataInfo_t **iCrypSampleHdrData,
				      mp4av_ismacrypParams *icPp)
{
  int numEncAUs = 0;
  u_int8_t i = 0;
  u_int8_t kIPerAU = false;
  /**/
  kIPerAU = icPp->key_ind_perau;
  *deltaIVLen = icPp->delta_iv_len;

  *iCrypAUHdrLen = 0;
  u_int32_t maxSampleSize = MP4GetTrackMaxSampleSize(mp4File, mediaTrackId);
  u_int8_t * pSampleBuffer = (u_int8_t *) malloc ((maxSampleSize + IVlen 
						       + KIlen + 1)
						      * sizeof(u_int8_t));
  if (pSampleBuffer == NULL)
     return false;

  for (i = 0; i < samplesThisHint; i++) {
    bool sampleIsEncrypted = false;
    if (useSelectiveEnc) {
      // add the 8 bits to signal if sample is encrypted 
      // (always present for selencryption)
      *iCrypAUHdrLen += 8;
      (*iCrypSampleHdrData)[i].hasEncField = 1;

      // need to figure out if sample is encrypted
      u_int32_t sampleSize = maxSampleSize;

      bool rc = MP4ReadSample(mp4File, mediaTrackId, pSampleIds[i], 
      		      &pSampleBuffer, &sampleSize);
      if (!rc) {
        free(pSampleBuffer);
	return false;
      }
      // check the 1st bit, if 1, then is encrypted, else is not
      if (pSampleBuffer[0] & 0x01) {
	sampleIsEncrypted = true;
	(*iCrypSampleHdrData)[i].isEncrypted = 1;
      } else {
	sampleIsEncrypted = false;
	(*iCrypSampleHdrData)[i].isEncrypted = 0;
      }
      if (pSampleBuffer != NULL) {
	free(pSampleBuffer);
      }
    } else {
      sampleIsEncrypted = true;
      (*iCrypSampleHdrData)[i].isEncrypted = 1;
      (*iCrypSampleHdrData)[i].hasEncField = 0;
    }
    
    if (sampleIsEncrypted) {
      if ((IVlen > 0) && (numEncAUs == 0)) { // first AU of pkt
	*iCrypAUHdrLen += 8 * IVlen;
	(*iCrypSampleHdrData)[i].hasIVField = 1;
      } else if ((IVlen > 0) && (numEncAUs > 0) && (*deltaIVLen > 0)) {
	// IsmaCrypDeltaIVLength is non-zero 
	*iCrypAUHdrLen += 8 * (*deltaIVLen);
	(*iCrypSampleHdrData)[i].hasIVField = 1; // is delta iv field
      } else {
	(*iCrypSampleHdrData)[i].hasIVField = 0;
      }

      // the hdr has the  key indicator if this is the first encrypted AU 
      // of the packet or if IsmaCrypKeyIndicatorPerAU is not zero
      if ((KIlen > 0) && (numEncAUs == 0 || kIPerAU)) {
	*iCrypAUHdrLen += 8 * KIlen;
	(*iCrypSampleHdrData)[i].hasKIField = 1;
      } else {
	(*iCrypSampleHdrData)[i].hasKIField = 0;
      }
      
      numEncAUs++;
     }
  }

  return true;
}

// Note: this is based on MP4AV_RfcIsmaConcatenator
static bool MP4AV_RfcCryptoConcatenator(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	u_int8_t samplesThisHint, 
	MP4SampleId* pSampleIds, 
	MP4Duration hintDuration,
	u_int16_t maxPayloadSize,
	mp4av_ismacrypParams *icPp,
	bool isInterleaved)
{

  // handle degenerate case
  if (samplesThisHint == 0) {
    return true;
  }

  u_int8_t auPayloadHdrSize = 0;

  // LATER would be more efficient if this were a parameter
  u_int8_t mpeg4AudioType =
    MP4GetTrackAudioMpeg4Type(mp4File, mediaTrackId);

  if (mpeg4AudioType == MP4_MPEG4_CELP_AUDIO_TYPE) {
    auPayloadHdrSize = 1;
  } else {
    auPayloadHdrSize = 2;
  }
  // construct the new hint
  MP4AddRtpHint(mp4File, hintTrackId);
  MP4AddRtpPacket(mp4File, hintTrackId, true);

  u_int8_t payloadHeader[2];

  u_int16_t numHdrBits = samplesThisHint * auPayloadHdrSize * 8;

  ismaCrypSampleHdrDataInfo_t *iCrypSampleHdrInfo = 
    (ismaCrypSampleHdrDataInfo_t *) 
    malloc (samplesThisHint * sizeof(ismaCrypSampleHdrDataInfo_t));
  if (iCrypSampleHdrInfo == NULL)
    return false;

  memset(iCrypSampleHdrInfo, 0, 
	 samplesThisHint * sizeof(ismaCrypSampleHdrDataInfo_t));
  u_int16_t ismaCrypAUHdrLen = 0;
  u_int8_t useSelectiveEnc = 0;
  u_int8_t KIlen = 0;
  u_int8_t IVlen = 0;
  u_int8_t deltaIVlen = 0;
  if (MP4AV_GetiSFMSettings(mp4File, mediaTrackId, &useSelectiveEnc, 
			    &KIlen, &IVlen, true) == false) {
    return false;
  }
      

  bool rc = MP4AV_ProcessIsmaCrypHdrs(mp4File, mediaTrackId, 
				      samplesThisHint, pSampleIds, 
				      useSelectiveEnc, KIlen, IVlen,
				      &deltaIVlen, &ismaCrypAUHdrLen,
				      &iCrypSampleHdrInfo, 
				      icPp);
  if (rc == false) {
    return false;
  }

  // add the AU header length AU header
  numHdrBits += ismaCrypAUHdrLen;
  payloadHeader[0] = numHdrBits >> 8;
  payloadHeader[1] = numHdrBits & 0xFF;
  MP4AddRtpImmediateData(mp4File, hintTrackId,
			 (u_int8_t*)&payloadHeader, sizeof(payloadHeader));

  u_int8_t i;
  // add the AU headers
  u_int32_t prevIV = 0;
  u_int32_t prevSize = 0;

  /*
  u_int32_t maxSampleSize = MP4GetTrackMaxSampleSize(mp4File, mediaTrackId);
  u_int8_t * pSampleBuffer = (u_int8_t *) malloc ((maxSampleSize 
						     + IVlen + KIlen + 1)
						    * sizeof(u_int8_t));
  if (pSampleBuffer == NULL)
     return false;
  */

  for (i = 0; i < samplesThisHint; i++) {
    MP4SampleId sampleId = pSampleIds[i];
    u_int32_t maxSampleSize = MP4GetTrackMaxSampleSize(mp4File, mediaTrackId);
    u_int32_t curIV = 0;
    // add the ismacryp AU header for the sample
    // first read the sample data into pSampleBuffer
    u_int32_t sampleSize = maxSampleSize;
    u_int8_t * pSampleBuffer = (u_int8_t *) malloc ((maxSampleSize 
						     + IVlen + KIlen + 1)
   						    * sizeof(u_int8_t));
    if (pSampleBuffer == NULL)
       return false;
    bool rc = MP4ReadSample(mp4File, mediaTrackId, pSampleIds[i], 
			    &pSampleBuffer, &sampleSize);
    if (!rc) {
      return false;
    }
    u_int8_t * pCurPosSampleBuffer = pSampleBuffer;

    // add the selective encryption header field if needed
    if (iCrypSampleHdrInfo[i].hasEncField == 1) {
      // if selective encryption
      // add the 8 bits to signal if the sample is encrypted
      // sample_is_encrypted bit + 7 reserved bits
      MP4AddRtpImmediateData(mp4File, hintTrackId,
			     pCurPosSampleBuffer, 1);  
      pCurPosSampleBuffer += 1;
    } 

    // add the IV/delta IV header field if needed
    if (iCrypSampleHdrInfo[i].hasIVField == 1) {
      // get the value of the current IV
      u_int32_t tmpIV = 0;
      memcpy(&tmpIV, pCurPosSampleBuffer, IVlen);
      // convert back to host byte order
      curIV = ntohl(tmpIV);
      // add the IV - from the ismacryp sample header data
      if (i == 0) { // first AU
	MP4AddRtpImmediateData(mp4File, hintTrackId,
			       pCurPosSampleBuffer, IVlen);
      } else if ((i > 0) && isInterleaved) { // subsequent AU and interleaving
	u_int8_t ivdeltalen = sizeof(int8_t) * deltaIVlen;
	// compute the delta IV
	if (deltaIVlen == 1) {
	  int8_t ivDelta = curIV - prevIV - prevSize;
	  MP4AddRtpImmediateData(mp4File, hintTrackId,  
			       (u_int8_t*)&ivDelta, ivdeltalen);
	} else if (deltaIVlen == 2) {
	  u_int16_t ivDelta = htons(curIV - prevIV - prevSize);
	  MP4AddRtpImmediateData(mp4File, hintTrackId,  
			       (u_int8_t*)&ivDelta, ivdeltalen);
	} else if (deltaIVlen > 2) { 
	  // should not happen with current ismacryp spec
	  return false;
	}
      } 
    }
   
    // there is always an IV field in the ismacryp sample header 
    // if IVlen > 0 so move the data pointer
    pCurPosSampleBuffer += IVlen; 

    // add the key indicator field if needed
    if (iCrypSampleHdrInfo[i].hasKIField == 1) {
      // add the key indicator - from the ismacryp sample header data
      MP4AddRtpImmediateData(mp4File, hintTrackId,
			     pCurPosSampleBuffer, KIlen);
    }
    // there is always a key indicator field in the ismacryp sample header 
    // if KIlen > 0 so move the data pointer
    pCurPosSampleBuffer += KIlen;

    if (pSampleBuffer != NULL) {
      free(pSampleBuffer);
    }

    // remove size of ismaCryp sample header from the sample size
    u_int32_t ismaCrypSampleHdrSize = MP4AV_GetIsmaCrypSampleHdrSize (
					iCrypSampleHdrInfo[i], IVlen, KIlen);
    sampleSize = MP4GetSampleSize(mp4File, mediaTrackId, sampleId) 
      - ismaCrypSampleHdrSize;
    
    if (auPayloadHdrSize == 1) {
      // AU payload header is 6 bits of size
      // follow by 2 bits of the difference between sampleId's - 1
      payloadHeader[0] = sampleSize << 2;

    } else { // auPayloadHdrSize == 2
      // AU payload header is 13 bits of size
      // follow by 3 bits of the difference between sampleId's - 1
      payloadHeader[0] = sampleSize >> 5;
      payloadHeader[1] = (sampleSize & 0x1F) << 3;
    }

    if (i > 0) {
      payloadHeader[auPayloadHdrSize - 1] 
	|= ((sampleId - pSampleIds[i-1]) - 1); 
    }

#if 0
    printf("sample %u size %u %02x %02x prev sample %d\n", 
            sampleId, sampleSize, payloadHeader[0],
            payloadHeader[1], pSampleIds[i-1]);
#endif


    MP4AddRtpImmediateData(mp4File, hintTrackId,
			   (u_int8_t*)&payloadHeader, auPayloadHdrSize);

    prevIV = curIV;
    prevSize = sampleSize;
  }

  // then the samples
  for (i = 0; i < samplesThisHint; i++) {
    MP4SampleId sampleId = pSampleIds[i];

    // figure out the size of the ismacryp sample header
    u_int32_t iCrypSampleHdrSize = MP4AV_GetIsmaCrypSampleHdrSize
                                   (iCrypSampleHdrInfo[i], IVlen, KIlen);
    // remove it from the sample size to get the real size of the data
    u_int32_t sampleSize = 
      MP4GetSampleSize(mp4File, mediaTrackId, sampleId) - iCrypSampleHdrSize;

    // use the offset parameter to point to the beginning of the data 
    // and skip the ismacryp sample header
    MP4AddRtpSampleData(mp4File, hintTrackId, sampleId, 
			iCrypSampleHdrSize, sampleSize);
  }

  // write the hint
  MP4WriteRtpHint(mp4File, hintTrackId, hintDuration);

  if (iCrypSampleHdrInfo != NULL) {
    free(iCrypSampleHdrInfo);
  }

  return true;
}

// Add the RTP packet with a sample fragment
static bool MP4AV_RfcCryptoFragmenter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
	MP4TrackId hintTrackId,
	MP4SampleId sampleId, 
	u_int32_t sampleSize, 
	MP4Duration sampleDuration,
	u_int16_t maxPayloadSize,
	mp4av_ismacrypParams *icPp)
{
  MP4AddRtpHint(mp4File, hintTrackId);

  MP4AddRtpPacket(mp4File, hintTrackId, false);

  // ismacryp
  u_int16_t numHdrBits = 0;
  ismaCrypSampleHdrDataInfo_t *iCrypSampleHdrInfo = 
    (ismaCrypSampleHdrDataInfo_t *) 
    malloc (sizeof(ismaCrypSampleHdrDataInfo_t));

  if (iCrypSampleHdrInfo == NULL)
    return false;

  memset(iCrypSampleHdrInfo, 0, sizeof(ismaCrypSampleHdrDataInfo_t));
  u_int16_t ismaCrypAUHdrLen = 0;
  u_int8_t useSelectiveEnc = 0;
  u_int8_t KIlen = 0;
  u_int8_t IVlen = 0;
  u_int8_t deltaIVlen = 0;
  if (MP4AV_GetiSFMSettings(mp4File, mediaTrackId, &useSelectiveEnc, 
			    &KIlen, &IVlen, true) == false) {
    return false;
  }
  bool rc = MP4AV_ProcessIsmaCrypHdrs(mp4File, mediaTrackId, 1, &sampleId, 
				      useSelectiveEnc, KIlen, IVlen,
				      &deltaIVlen, &ismaCrypAUHdrLen,
				      &iCrypSampleHdrInfo,
				      icPp);
  if (rc == false) {
    return false;
  }

  // Note: CELP is never fragmented
  // so we assume the two byte AAC-hbr payload header
  numHdrBits = 16 + ismaCrypAUHdrLen; // 2 bytes for AAC AU hdr
  // add size of AU headers
  u_int8_t payloadHeaderSize[2];
  payloadHeaderSize[0] = numHdrBits >> 8;
  payloadHeaderSize[1] = numHdrBits & 0xFF;
  MP4AddRtpImmediateData(mp4File, hintTrackId,
			 (u_int8_t*)&payloadHeaderSize, 
			 sizeof(payloadHeaderSize));

  // add ismacryp AU headers
  if (ismaCrypAUHdrLen > 0) {
    u_int32_t tmpSampleSize = sampleSize;
    u_int8_t * pSampleBuffer = (u_int8_t *) malloc ((tmpSampleSize + IVlen 
						     + KIlen + 1)
						    * sizeof(u_int8_t));
    if (pSampleBuffer == NULL)
      return false;

    u_int8_t * pCurSampleBuf = pSampleBuffer;
    bool rc2 = MP4ReadSample(mp4File, mediaTrackId, sampleId, 
			     &pSampleBuffer, &tmpSampleSize);
    if (rc2 == false) {
      return false;
    }
    if (iCrypSampleHdrInfo[0].hasEncField == 1) {
      MP4AddRtpImmediateData(mp4File, hintTrackId, pCurSampleBuf, 1);  
      pCurSampleBuf += 1;
    } 
    if (iCrypSampleHdrInfo[0].hasIVField == 1) {
      MP4AddRtpImmediateData(mp4File, hintTrackId, pCurSampleBuf, IVlen);
    }
    pCurSampleBuf += IVlen;

    if (iCrypSampleHdrInfo[0].hasKIField == 1) {
      MP4AddRtpImmediateData(mp4File, hintTrackId, pCurSampleBuf, KIlen);
    }
    pCurSampleBuf += KIlen;
    
    if (pSampleBuffer != NULL) {
      free(pSampleBuffer);
    }
  }

  // remove size of ismaCryp sample header from the sample size
  u_int32_t ismaCrypSampleHdrSize = MP4AV_GetIsmaCrypSampleHdrSize (
					 iCrypSampleHdrInfo[0], IVlen, KIlen);
  sampleSize -= ismaCrypSampleHdrSize;
  
  // add AAC AU header
  u_int8_t payloadHeader[2];
  payloadHeader[0] = sampleSize >> 5;
  payloadHeader[1] = (sampleSize & 0x1F) << 3;

  MP4AddRtpImmediateData(mp4File, hintTrackId,
			 (u_int8_t*)&payloadHeader, sizeof(payloadHeader));

  // start after the ismaCrypSampleHdr
  u_int16_t sampleOffset = ismaCrypSampleHdrSize; 
  u_int16_t fragLength = maxPayloadSize - 4;

  do {
    MP4AddRtpSampleData(mp4File, hintTrackId,
			sampleId, sampleOffset, fragLength);

    sampleOffset += fragLength;

    if (sampleSize - sampleOffset > maxPayloadSize) {
      fragLength = maxPayloadSize; 
      MP4AddRtpPacket(mp4File, hintTrackId, false);
    } else {
      fragLength = sampleSize - sampleOffset; 
      if (fragLength) {
	MP4AddRtpPacket(mp4File, hintTrackId, true);
      }
    }
  } while (sampleOffset < sampleSize);
  
  MP4WriteRtpHint(mp4File, hintTrackId, sampleDuration);

  return true;
}

// based on MP4AV_RfcIsmaHinter, with ismacryp SDP added
extern "C" bool MP4AV_RfcCryptoAudioHinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
        mp4av_ismacrypParams *icPp,
	bool interleave,
	u_int16_t maxPayloadSize,
	char* PayloadMIMEType)
{

  // gather information, and check for validity

  u_int32_t numSamples =
    MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);

  if (numSamples == 0) {
    return false;
  }

  u_int32_t timeScale =
    MP4GetTrackTimeScale(mp4File, mediaTrackId);

  if (timeScale == 0) {
    return false;
  }

  u_int8_t audioType =
    MP4GetTrackEsdsObjectTypeId(mp4File, mediaTrackId);

  if (audioType != MP4_MPEG4_AUDIO_TYPE
      && !MP4_IS_AAC_AUDIO_TYPE(audioType)) {
    return false;
  }

  u_int8_t mpeg4AudioType =
    MP4GetTrackAudioMpeg4Type(mp4File, mediaTrackId);

  if (audioType == MP4_MPEG4_AUDIO_TYPE) {
    // check that track contains either MPEG-4 AAC or CELP
    if (!MP4_IS_MPEG4_AAC_AUDIO_TYPE(mpeg4AudioType) 
	&& mpeg4AudioType != MP4_MPEG4_CELP_AUDIO_TYPE) {
      return false;
    }
  }

  MP4Duration sampleDuration = 
    MP4AV_GetAudioSampleDuration(mp4File, mediaTrackId);

  if (sampleDuration == MP4_INVALID_DURATION) {
    return false;
  }

  /* get the ES configuration */
  u_int8_t* pConfig = NULL;
  u_int32_t configSize;
  uint8_t channels;

  MP4GetTrackESConfiguration(mp4File, mediaTrackId, &pConfig, 
			     &configSize);

  if (!pConfig) {
    return false;
  }
     
  channels = MP4AV_AacConfigGetChannels(pConfig);

  /* convert ES Config into ASCII form */
  char* sConfig = 
    MP4BinaryToBase16(pConfig, configSize);

  free(pConfig);

  if (!sConfig) {
    return false;
  }


  // now add the hint track
  MP4TrackId hintTrackId =
    MP4AddHintTrack(mp4File, mediaTrackId);

  if (hintTrackId == MP4_INVALID_TRACK_ID) {
    free(sConfig);
    return false;
  }

  u_int8_t payloadNumber = MP4_SET_DYNAMIC_PAYLOAD;
  char buffer[10];
  if (channels != 1) {
    snprintf(buffer, sizeof(buffer), "%u", channels);
  }
  MP4SetHintTrackRtpPayload(mp4File, hintTrackId, 
			    (const char*)PayloadMIMEType, 
			    &payloadNumber, 0, 
			    channels != 1 ? buffer : NULL);

  // moved this interleave check to here.
  u_int32_t samplesPerPacket = 0;
  if (interleave) {  
    u_int32_t maxSampleSize =
      MP4GetTrackMaxSampleSize(mp4File, mediaTrackId);

    // compute how many maximum size samples would fit in a packet
    samplesPerPacket = 
      (maxPayloadSize - 2) / (maxSampleSize + 2);

    // can't interleave if this number is 0 or 1
    if (samplesPerPacket < 2) {
      interleave = false;
      // Since interleave is now false, reset delta IV length
      // for the session to zero or headers will be wrong...AWV
      // This needs to be done before the SDP is created. ...AWV
      icPp->delta_iv_len = 0;
    }
   else 
      // set the delta IV length parameter to 2
      // per the ismacryp specification (setting to 1 is too small)
      icPp->delta_iv_len = 2;
  }

  // compute the ISMACryp configuration parameters  mjb
  ISMACrypConfigTable_t ismac_table;
  u_int16_t i;
  memset(&ismac_table, 0, sizeof(ISMACrypConfigTable_t));
  if(!InitISMACrypConfigTable(&ismac_table, icPp)){
    free(sConfig);
    return false;
	}
  char* sISMACrypConfig = NULL;

  if (!MP4AV_RfcCryptoPolicyOk(&ismac_table)) {
    free(sConfig);
    for(i = 0; i < ismac_table.settings[ISMACRYP_KEY_COUNT];i++) {
      free(ismac_table.keys[i]);		
      free(ismac_table.salts[i]);	
    }
    return false;
  }

  if (!MP4AV_RfcCryptoConfigure(&ismac_table, &sISMACrypConfig)) {
     free(sConfig);
     return false;
  }

  int sdpBufLen = strlen(sConfig) + strlen(sISMACrypConfig) + ISMACRYP_SDP_ADDITIONAL_CONFIG_MAX_LEN;
  char* sdpBuf = (char*)malloc(sdpBufLen);

  if (!sdpBuf) {
    free(sConfig);
    free(sISMACrypConfig);
    return false;
  }

  MP4Duration maxLatency;
  bool OneByteHeader = false;
  if (mpeg4AudioType == MP4_MPEG4_CELP_AUDIO_TYPE) {
    snprintf(sdpBuf,
             sdpBufLen,
	    "a=fmtp:%u "
	    "streamtype=5; profile-level-id=15; mode=CELP-vbr; "
	    "config=%s; SizeLength=6; IndexLength=2; "
	    "IndexDeltaLength=2; Profile=0;"
	    "%s"
	    "\015\012",
	    payloadNumber,
	    sConfig,
	    sISMACrypConfig); 

    // 200 ms max latency for ISMA profile 1
    maxLatency = timeScale / 5;
    OneByteHeader = true;
  } else { // AAC
    snprintf(sdpBuf,
             sdpBufLen,
	    "a=fmtp:%u "
	    "streamtype=5; profile-level-id=15; mode=AAC-hbr; "
	    "config=%s; SizeLength=13; IndexLength=3; "
	    "IndexDeltaLength=3; Profile=1;"
	    "%s "                              //mjb
	    "\015\012",
	    payloadNumber,
	    sConfig,
	    sISMACrypConfig); 

    // 500 ms max latency for ISMA profile 1
    maxLatency = timeScale / 2;
  }

  /* add this to the track's sdp */
  MP4AppendHintTrackSdp(mp4File, hintTrackId, sdpBuf);

  free(sConfig);
  free(sdpBuf);
  free(sISMACrypConfig);		//mjb
  for(i = 0; i < ismac_table.settings[ISMACRYP_KEY_COUNT];i++) {
    free(ismac_table.keys[i]);
    free(ismac_table.salts[i]);	
  }
 
  // this is where the interleave check was....AWV

  bool rc;

  if (interleave) {
    u_int32_t samplesPerGroup = maxLatency / sampleDuration;
    u_int32_t stride;
    stride = samplesPerGroup / samplesPerPacket;

    if (OneByteHeader && stride > 3) stride = 3;
    if (!OneByteHeader && stride > 7) stride = 7;

#if 0
    printf("max latency %llu sampleDuration %llu spg %u spp %u strid %u\n",
                       maxLatency, sampleDuration, samplesPerGroup,
                       samplesPerPacket, stride);
#endif


    rc = MP4AV_CryptoAudioInterleaveHinter(
				     mp4File, 
				     mediaTrackId, 
				     hintTrackId,
				     sampleDuration, 
				     stride, // stride
				     samplesPerPacket,		    // bundle
				     maxPayloadSize,
				     icPp);

  } else {
    rc = MP4AV_CryptoAudioConsecutiveHinter(
				      mp4File, 
				      mediaTrackId, 
				      hintTrackId,
				      sampleDuration, 
				      2,				// perPacketHeaderSize
				      2,				// perSampleHeaderSize
				      maxLatency / sampleDuration,	// maxSamplesPerPacket
				      maxPayloadSize,
				      MP4GetSampleSize,
				      icPp);
  }

  if (!rc) {
    MP4DeleteTrack(mp4File, hintTrackId);
    return false;
  }

  return true;
}

// based on MP4AV_Rfc3016Hinter
extern "C" bool MP4AV_RfcCryptoVideoHinter(
	MP4FileHandle mp4File, 
	MP4TrackId mediaTrackId, 
        mp4av_ismacrypParams *icPp,
	u_int16_t maxPayloadSize,
	char* PayloadMIMEType)
{
  u_int32_t numSamples = MP4GetTrackNumberOfSamples(mp4File, mediaTrackId);
  u_int32_t maxSampleSize = MP4GetTrackMaxSampleSize(mp4File, mediaTrackId);
	
  if (numSamples == 0 || maxSampleSize == 0) {
    return false;
  }

  MP4TrackId hintTrackId =
    MP4AddHintTrack(mp4File, mediaTrackId);

  if (hintTrackId == MP4_INVALID_TRACK_ID) {
    return false;
  }

  u_int8_t payloadNumber = MP4_SET_DYNAMIC_PAYLOAD;

  MP4SetHintTrackRtpPayload(mp4File, hintTrackId, 
			    (const char*) PayloadMIMEType, &payloadNumber, 0);

  /* get the mpeg4 video configuration */
  u_int8_t* pConfig;
  u_int32_t configSize;
  u_int8_t systemsProfileLevel = 0xFE;

  MP4GetTrackESConfiguration(mp4File, mediaTrackId, &pConfig, &configSize);

  if (pConfig) {
    // compute the ISMACryp configuration parameters  mjb
    ISMACrypConfigTable_t ismac_table;
    u_int16_t i;
    memset(&ismac_table, 0, sizeof(ISMACrypConfigTable_t));
    if(!InitISMACrypConfigTable(&ismac_table, icPp)){
      return false;
    }
    char* sISMACrypConfig = NULL;
    if (!MP4AV_RfcCryptoPolicyOk(&ismac_table)) {
      for(i = 0; i < ismac_table.settings[ISMACRYP_KEY_COUNT];i++){
	free(ismac_table.keys[i]);		
	free(ismac_table.salts[i]);	
      }
      return false;
    }

    if (!MP4AV_RfcCryptoConfigure(&ismac_table, &sISMACrypConfig))
      return false;

    // attempt to get a valid profile-level
    static u_int8_t voshStartCode[4] = { 
      0x00, 0x00, 0x01, MP4AV_MPEG4_VOSH_START 
    };
    if (configSize >= 5 && !memcmp(pConfig, voshStartCode, 4)) {
      systemsProfileLevel = pConfig[4];
    } 
    if (systemsProfileLevel == 0xFE) {
      u_int8_t iodProfileLevel = MP4GetVideoProfileLevel(mp4File);
      if (iodProfileLevel > 0 && iodProfileLevel < 0xFE) {
	systemsProfileLevel = iodProfileLevel;
      } else {
	systemsProfileLevel = 1;
      }
    } 

    /* convert it into ASCII form */
    char* sConfig = MP4BinaryToBase16(pConfig, configSize);
    if (sConfig == NULL) {
      MP4DeleteTrack(mp4File, hintTrackId);
      free(sISMACrypConfig);
      for(i = 0; i < ismac_table.settings[ISMACRYP_KEY_COUNT];i++) {
	free(ismac_table.keys[i]);		
	free(ismac_table.salts[i]);	
      }
      return false;
    }

    int   sdpBufLen = strlen(sConfig) + strlen(sISMACrypConfig) + ISMACRYP_SDP_ADDITIONAL_CONFIG_MAX_LEN;
    char* sdpBuf = (char*)malloc(sdpBufLen);

    if (sdpBuf == NULL) {
     free(sISMACrypConfig);
     free(sConfig);
     return false;
    }

    snprintf(sdpBuf,
             sdpBufLen,
	    "a=fmtp:%u profile-level-id=%u; mode=mpeg4-video; "
	    "config=%s; %s; \015\012",
	    payloadNumber,
	    systemsProfileLevel,
	    sConfig,
	    sISMACrypConfig); 

    /* add this to the track's sdp */
    MP4AppendHintTrackSdp(mp4File, hintTrackId, sdpBuf);

    free(sConfig);
    free(sdpBuf);
    free(sISMACrypConfig);
    for(i = 0; i < ismac_table.settings[ISMACRYP_KEY_COUNT];i++) {
      free(ismac_table.keys[i]);		
      free(ismac_table.salts[i]);	
    }
  }
  u_int8_t* pSampleBuffer = (u_int8_t*)malloc(maxSampleSize);
  if (pSampleBuffer == NULL) {
    MP4DeleteTrack(mp4File, hintTrackId);
    return false;
  }
  
  u_int8_t useSelectiveEnc = 0;
  u_int8_t KIlen = 0;
  u_int8_t IVlen = 0;
  u_int8_t deltaIVlen = 0;
  if (MP4AV_GetiSFMSettings(mp4File, mediaTrackId, &useSelectiveEnc, 
			    &KIlen, &IVlen, false) == false) {
    return false;
  }

  for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) {
    u_int32_t sampleSize = maxSampleSize;  // +IVlen +KIlen +1
    MP4Timestamp startTime;
    MP4Duration duration;
    MP4Duration renderingOffset;
    bool isSyncSample;

    bool rc = MP4ReadSample(mp4File, mediaTrackId, sampleId, 
			    &pSampleBuffer, &sampleSize, 
			    &startTime, &duration, 
			    &renderingOffset, &isSyncSample);

    if (!rc) {
      MP4DeleteTrack(mp4File, hintTrackId);
      return false;
    }

    bool isBFrame = 
      (MP4AV_Mpeg4GetVopType(pSampleBuffer, sampleSize) == VOP_TYPE_B);

    MP4AddRtpVideoHint(mp4File, hintTrackId, isBFrame, renderingOffset);

    // comment from bill: may need to remove or put it in the header ??
    if (sampleId == 1) {
      MP4AddRtpESConfigurationPacket(mp4File, hintTrackId);
    }

    u_int32_t offset = 0;
    u_int32_t remaining = sampleSize;

    // TBD should scan for resync markers (if enabled in ES config)
    // and packetize on those boundaries

    while (remaining) {
      bool isLastPacket = false;
      u_int32_t length;

      // need to add ismacryp AU header length 
      if (remaining <= maxPayloadSize) {
	length = remaining;
	isLastPacket = true;
      } else {
	length = maxPayloadSize;
      }

      MP4AddRtpPacket(mp4File, hintTrackId, isLastPacket);
			
      ismaCrypSampleHdrDataInfo_t *iCrypSampleHdrInfo = 
	(ismaCrypSampleHdrDataInfo_t *) 
	malloc (sizeof(ismaCrypSampleHdrDataInfo_t));
      if (iCrypSampleHdrInfo == NULL)
	return false;
      
      memset(iCrypSampleHdrInfo, 0, sizeof(ismaCrypSampleHdrDataInfo_t));
      u_int16_t ismaCrypAUHdrLen = 0;
      bool rc = MP4AV_ProcessIsmaCrypHdrs(mp4File, mediaTrackId, 1, 
					  &sampleId, useSelectiveEnc, 
					  KIlen, IVlen, &deltaIVlen, 
					  &ismaCrypAUHdrLen,
					  &iCrypSampleHdrInfo, 
					  icPp);
      
      if (rc == false) {
	// handle error
	return false;
      }
      if (ismaCrypAUHdrLen > 0) {
	// add AU hdr length
	u_int8_t payloadHeader[2];
	payloadHeader[0] = ismaCrypAUHdrLen >> 8;
	payloadHeader[1] = ismaCrypAUHdrLen & 0xFF;
	MP4AddRtpImmediateData(mp4File, hintTrackId,
			       (u_int8_t*)&payloadHeader, sizeof(payloadHeader));
	// add ismacryp AU headers
	u_int32_t tmpSampleSize = maxSampleSize;
	u_int8_t * pSampleBuffer = (u_int8_t *) malloc ((tmpSampleSize + IVlen + KIlen + 1)
							* sizeof(u_int8_t));
	if (pSampleBuffer == NULL)
	  return false;
	
	u_int8_t * pCurSampleBuf = pSampleBuffer;
	bool rc2 = MP4ReadSample(mp4File, mediaTrackId, sampleId, 
				 &pSampleBuffer, &tmpSampleSize);
	if (rc2 == false) {
	  // handle error
	  return false;
	}

	if (iCrypSampleHdrInfo[0].hasEncField == 1) {
	  MP4AddRtpImmediateData(mp4File, hintTrackId, pCurSampleBuf, 1);  
	  pCurSampleBuf += 1;
	} 
	if (iCrypSampleHdrInfo[0].hasIVField == 1) {
	  MP4AddRtpImmediateData(mp4File, hintTrackId, pCurSampleBuf, IVlen);
	}
	pCurSampleBuf += IVlen;
	if (iCrypSampleHdrInfo[0].hasKIField == 1) {
	  MP4AddRtpImmediateData(mp4File, hintTrackId, pCurSampleBuf, KIlen);
	}
	pCurSampleBuf += KIlen;

	if (pSampleBuffer != NULL) {
	  free(pSampleBuffer);
	}

	// remove size of ismaCryp sample header from the sample size
	u_int32_t ismaCrypSampleHdrSize = MP4AV_GetIsmaCrypSampleHdrSize (iCrypSampleHdrInfo[0], 
									  IVlen, KIlen);
	sampleSize -= ismaCrypSampleHdrSize;
      }

      MP4AddRtpSampleData(mp4File, hintTrackId, sampleId, 
			  offset, length);

      offset += length;
      remaining -= length;
    }

    MP4WriteRtpHint(mp4File, hintTrackId, duration, isSyncSample);
  }

  return true;
}

