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

#ifndef ISMACRYPLIB_H
#define ISMACRYPLIB_H

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#include "mpeg4ip.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef uint32_t ismacryp_session_id_t; 

typedef enum  ismacryp_rc_t { ismacryp_rc_ok = 0, 
                            ismacryp_rc_sessid_error, 
                            ismacryp_rc_keytype_error, 
                            ismacryp_rc_keyfile_error, 
                            ismacryp_rc_keyfilename_error, 
                            ismacryp_rc_key_error, 
                            ismacryp_rc_memory_error, 
                            ismacryp_rc_cipheralloc_error, 
                            ismacryp_rc_cipherinit_error, 
                            ismacryp_rc_unsupported_error, 
                            ismacryp_rc_encrypt_error, 
                            ismacryp_rc_protocol_error } ismacryp_rc_t;

typedef enum  ismacryp_keytype_t { KeyTypeOther=0, 
                            KeyTypeVideo, 
                            KeyTypeAudio } ismacryp_keytype_t;


typedef uint32_t ismacryp_scheme_t;
typedef uint16_t ismacryp_schemeversion_t;

ismacryp_rc_t ismacrypInitLib(void);
ismacryp_rc_t ismacrypInitSession(ismacryp_session_id_t *session, ismacryp_keytype_t  keytype);

ismacryp_rc_t ismacrypEndSession(ismacryp_session_id_t session);

ismacryp_rc_t ismacrypSetScheme(ismacryp_session_id_t session,
                                 ismacryp_scheme_t scheme);
ismacryp_rc_t ismacrypGetScheme(ismacryp_session_id_t session,
                                 ismacryp_scheme_t *scheme);

ismacryp_rc_t ismacrypSetSchemeVersion(ismacryp_session_id_t session,
                                        ismacryp_schemeversion_t scheme);
ismacryp_rc_t ismacrypGetSchemeVersion(ismacryp_session_id_t session,
                                        ismacryp_schemeversion_t *scheme);

ismacryp_rc_t ismacrypSetKMSUri(ismacryp_session_id_t session,
                                 char *kms_uri );
ismacryp_rc_t ismacrypGetKMSUri(ismacryp_session_id_t session,
                                 char **kms_uri );

ismacryp_rc_t ismacrypSetSelectiveEncryption(ismacryp_session_id_t session,
                                              uint8_t selective_is_on );
ismacryp_rc_t ismacrypGetSelectiveEncryption(ismacryp_session_id_t session,
                                              uint8_t *selective_is_on );


ismacryp_rc_t ismacrypSetKeyIndicatorLength(ismacryp_session_id_t session,
                                              uint8_t key_indicator_len);
ismacryp_rc_t ismacrypGetKeyIndicatorLength(ismacryp_session_id_t session,
                                              uint8_t *key_indicator_len);
ismacryp_rc_t ismacrypGetKeyIndPerAU(ismacryp_session_id_t session,
                                      uint8_t *key_ind_per_au);


ismacryp_rc_t ismacrypGetKeyCount(ismacryp_session_id_t session,
                                   uint8_t *keycount);

ismacryp_rc_t ismacrypGetKey(ismacryp_session_id_t session,
                              uint8_t key_num,
                              uint8_t *key_len,
                              uint8_t *salt_len,
                              uint8_t **key,
                              uint8_t **salt,
                              uint8_t *lifetime_exp);

ismacryp_rc_t ismacrypSetIVLength(ismacryp_session_id_t session,
                                   uint8_t iv_len);
ismacryp_rc_t ismacrypGetIVLength(ismacryp_session_id_t session,
                                   uint8_t *iv_len);
ismacryp_rc_t ismacrypSetDeltaIVLength(ismacryp_session_id_t session,
                                        uint8_t delta_iv_len);
ismacryp_rc_t ismacrypGetDeltaIVLength(ismacryp_session_id_t session,
                                        uint8_t *delta_iv_len);

uint32_t      ismacrypEncryptSampleAddHeader2(uint32_t session,
                                    uint32_t length,
                                    uint8_t *data,
                                    uint32_t *new_length,
                                    uint8_t **new_data);

ismacryp_rc_t ismacrypEncryptSampleAddHeader(ismacryp_session_id_t session,
                                    uint32_t length,
                                    uint8_t *data,
                                    uint32_t *new_length,
                                    uint8_t **new_data);

ismacryp_rc_t ismacrypEncryptSample(ismacryp_session_id_t session,
                                    uint32_t length,
                                    uint8_t *data ); 

ismacryp_rc_t ismacrypDecryptSample(ismacryp_session_id_t session,
                                    uint32_t length,
                                    uint8_t *data);

ismacryp_rc_t ismacrypDecryptSampleRandomAccess(ismacryp_session_id_t session,
                                    uint32_t BSO,
                                    uint32_t length,
                                    uint8_t *data);

ismacryp_rc_t ismacrypDecryptSampleRemoveHeader(ismacryp_session_id_t session,
                                    uint32_t length,
                                    uint8_t *data,
                                    uint32_t *new_length,
                                    uint8_t **new_data);
#ifdef __cplusplus
}
#endif
#endif /* ifndef ISMACRYPLIB_H */
