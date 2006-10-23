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
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2003, 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Alex Vanzella           alexv@cisco.com
 */

#ifndef ISMACRYPLIBPRIV_H
#define ISMACRYPLIBPRIV_H

#include <stdio.h>
#include <stdlib.h>

#include "mpeg4ip.h"
#include "ismacryplib.h"

#ifdef HAVE_SRTP
#include "srtp/aes_icm.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define KMS_URI_STR  "www.kms_uri.com"
#define KMS_DATA_FILE ".kms_data"
#define KMS_DATA_FILE_MAX_LINE_LEN 256 
#define KMS_DATA_FILE_FILENAME_MAX_LEN 256 
#define KMS_DATA_AUDIOKEY_STR  "AudioKey"
#define KMS_DATA_VIDEOKEY_STR  "VideoKey"

#define TEST_BUFFER_LEN   64
#define SAMPLE_TEXT_LEN   32
#define AES_TOT_LEN       32
#define AES_KEY_LEN       16
#define AES_SALT_LEN       8
#define AES_KEY_SALT_LEN  (AES_KEY_LEN+AES_SALT_LEN)
#define AES_COUNTER_LEN   (AES_TOT_LEN-AES_KEY_SALT_LEN) 

#define AES_BYTES_PER_COUNT 16

#define ISMACRYP_DEFAULT_SELECTIVE_ENC         FALSE
#define ISMACRYP_DEFAULT_SCHEME                "iAEC"
#define ISMACRYP_DEFAULT_SCHEME_VERSION        1
#define ISMACRYP_DEFAULT_IV_LENGTH             4
#define ISMACRYP_DEFAULT_DELTA_IV_LENGTH       0
#define ISMACRYP_MAX_DELTA_IV_LENGTH           2
#define ISMACRYP_DEFAULT_SELECTIVE_ENCRYPTION  0
#define ISMACRYP_DEFAULT_KEYINDICATOR_LENGTH   0
#define ISMACRYP_DEFAULT_KEYINDICATOR_PER_AU   FALSE
#define ISMACRYP_DEFAULT_KEY_LIFETIME_EXP      64
#define ISMACRYP_MIN_SESSION_ID                10
 
typedef struct ksc_struct{
 uint8_t  key[AES_KEY_LEN];
 uint8_t  salt[AES_SALT_LEN];
 uint8_t  counter[AES_COUNTER_LEN];
} ksc_t;


typedef struct sess_struct{
  ismacryp_session_id_t sessid;  
  union { 
    uint8_t  aes_overlay[AES_TOT_LEN];
    ksc_t    ksc;
  } kk;
 #ifdef HAVE_SRTP
    cipher_t            *cp;
 #endif
  uint8_t             keycount;
  uint8_t             IV_len;
  uint8_t             deltaIV_len;
  ismacryp_keytype_t  key_type;
  uint8_t             selective_enc;
  uint32_t            sample_count;
  uint32_t            BSO;
  char                *kms_uri;
  struct sess_struct *prev;
  struct sess_struct *next;

} ismacryp_session_t;


// 
//	PROTOS
//


#ifdef __cplusplus
}
#endif
#endif
