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
 *              Will Clark           will_clark@cisco.com
 */

#ifndef LIBLIBSRTP_H
#define LIBLIBSRTP_H

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#include "mpeg4ip.h"
#include "rtp.h"
#ifdef HAVE_SRTP
#include "srtp/srtp.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SRTP_MAX_TRAILER_LEN
#define AUTHENTICATION_DEFAULT_LEN SRTP_MAX_TRAILER_LEN
#else
#define AUTHENTICATION_DEFAULT_LEN 10
#endif


  typedef enum srtp_enc_algos_t {
    SRTP_ENC_AES_CM_128 = 0,
  } srtp_enc_algos_t;

  typedef enum srtp_auth_algos_t {
    SRTP_AUTH_HMAC_SHA1_80 = 0,
    SRTP_AUTH_HMAC_SHA1_32 = 1,
  } srtp_auth_algos_t;

  typedef struct srtp_if_params_t {
    const char *tx_key;
    const char *tx_salt;
    uint8_t *tx_keysalt;

    const char *rx_key;
    const char *rx_salt;
    uint8_t *rx_keysalt;
    
    srtp_enc_algos_t enc_algo;
    srtp_auth_algos_t auth_algo;

    bool rtp_enc;
    bool rtp_auth;
    bool rtcp_enc;
  } srtp_if_params_t;

  typedef struct srtp_if_t_ srtp_if_t;

  
  srtp_if_t *srtp_setup(struct rtp *session, srtp_if_params_t *sparams);

  bool
  srtp_parse_sdp(const char *media_type, 
		 const char* crypto,
		 srtp_if_params_t* params);

  srtp_if_t *srtp_setup_from_sdp(const char *media_name,
				 struct rtp *session, 
				 const char *aeqcryptoline);

  char *srtp_create_sdp(const char *media, srtp_if_params_t *params);
  
  void destroy_srtp(srtp_if_t *srtpdata);

  void srtp_if_set_loglevel(int loglevel);

  void srtp_if_set_error_func(error_msg_func_t func);
#ifdef __cplusplus
}
#endif
#endif /* ifndef LIBLIBSRTP_H */
