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
 * Copyright (C) Cisco Systems Inc. 2006.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Will Clark              will_clark@cisco.com
 */

#include "liblibsrtp.h"
#include "rtp.h"
#include "mp4.h"
#include <assert.h>
#include "mutex.h"

//#define DEBUG_SRTP_CALLS 1
#define DUMP_RAW_RTP_PAK 0
#define DUMP_ENC_RTP_PAK 0
#define DUMP_RAW_RTCP_PAK 1
#define DUMP_ENC_RTCP_PAK 1

#ifdef HAVE_SRTP
struct srtp_if_t_ {
  srtp_policy_t in_policy, out_policy;
  srtp_t in_ctx, out_ctx;
  uint8_t tx_keysalt[32]; // 32, not 30, due to brokenness in libsrtp
  uint8_t rx_keysalt[32]; // 32, not 30, due to brokenness in libsrtp
  mutex_t mutex;
};
#endif

static int srtp_if_debug_level = LOG_DEBUG;

static error_msg_func_t error_msg_func = NULL;

void srtp_if_set_loglevel (int loglevel)
{
  srtp_if_debug_level = loglevel;
}

void srtp_if_set_error_func (error_msg_func_t func)
{
  error_msg_func = func;
}

#ifdef HAVE_SRTP
static void srtp_if_debug (int loglevel, const char *fmt, ...)
{
  if (loglevel <= srtp_if_debug_level) {
    if (error_msg_func != NULL) {
      va_list ap;
      va_start(ap, fmt);
      (error_msg_func)(loglevel, "libsrtp_if", fmt, ap);
      va_end(ap);
    } else {
      va_list ap;
      printf("libsrtp_if-%d:", loglevel);
      va_start(ap, fmt);
      vprintf(fmt, ap);
      va_end(ap);
      printf("\n");
    }
  }
}
#endif

#ifdef HAVE_SRTP
static bool srtp_if_initialize(void)
{
  static bool srtp_inited = false;
  err_status_t status;
  if (srtp_inited) return true;

  status = srtp_init();
  if (status) {
    srtp_if_debug(LOG_ALERT, "srtp initialization failed with error cde %d", status);
    return false;
  }
  srtp_inited = true;

  return true;
}
#endif

#ifdef HAVE_SRTP
static int our_srtp_encrypt (void *foo, 
			     uint8_t *buffer, 
			     uint32_t *len)
{
  err_status_t err;
  int retdata;
  srtp_if_t *srtp_if = (srtp_if_t *)foo;
  uint32_t i;
  retdata = *len;
  if (DUMP_RAW_RTP_PAK) {
    srtp_if_debug(LOG_DEBUG,"Calling srtp_protect, len %d", *len);
    for (i = 0; i < *len; i++) {
      printf("%02x ", buffer[i]);
      if (((i + 1) % 16) == 0) printf("\n");
    }
    printf("\n");
  }

#ifdef DEBUG_SRTP_CALLS
  srtp_if_debug(LOG_DEBUG,"calling srtp_protect: len %d proto %u seq %u", *len,
		buffer[1] & 0x7f, ntohs(*(uint16_t *)(buffer + 2)));
#endif

  MutexLock(srtp_if->mutex);
  err = srtp_protect(srtp_if->out_ctx, buffer, &retdata);
  MutexUnlock(srtp_if->mutex);

  if (DUMP_ENC_RTP_PAK) {
    srtp_if_debug(LOG_DEBUG,"Calling srtp_protect, ERR %d\n", *len);
    for (i = 0; i < *len; i++) {
      printf("%02x ", buffer[i]);
      if (((i + 1) % 16) == 0) printf("\n");
    }
    printf("\n");
  }

  if (err != 0) {
    srtp_if_debug(LOG_ERR, "failing srtp encrypts error %d len %u", err, *len);
    return FALSE;
  }
  *len = retdata;
  return TRUE;
}

static int our_srtp_decrypt (void *foo, 
			     uint8_t *buffer, 
			     uint32_t *len)
{
  err_status_t err;
  int retdata;
	srtp_if_t *srtp_if = (srtp_if_t *)foo;
  uint32_t i;

  retdata = *len;

  if (DUMP_ENC_RTP_PAK) {
    srtp_if_debug(LOG_DEBUG,"Calling srtp_unprotect, len %d", *len);
    for (i = 0; i < *len; i++) {
      printf("%02x ", buffer[i]);
      if (((i + 1) % 16) == 0) printf("\n");
    }
    printf("\n");
  }
#ifdef DEBUG_SRTP_CALLS
  srtp_if_debug(LOG_DEBUG,"calling srtp_unprotect: len %d proto %u seq %u", *len,
		buffer[1] & 0x7f, ntohs(*(uint16_t *)(buffer + 2)));
#endif
  MutexLock(srtp_if->mutex);
  err = srtp_unprotect(srtp_if->in_ctx, buffer, &retdata);
  MutexUnlock(srtp_if->mutex);
  if(err != 0) {
    srtp_if_debug(LOG_DEBUG,"called srtp_unprotect: ERR %d", err);
  }

  if (DUMP_RAW_RTP_PAK) {
    for (i = 0; i < *len; i++) {
      printf("%02x ", buffer[i]);
      if (((i + 1) % 12) == 0) printf("\n");
    }
    printf("\n");
    srtp_if_debug(LOG_DEBUG,"exiting srtp_decrypt %d %d", err, retdata);
  }
  if (err != 0) {
    srtp_if_debug(LOG_ERR, "return from srtp_unprotect %d len %d orig %u",
		  err, retdata, *len);
    return FALSE;
  }
  *len = retdata;
  return TRUE;
}     

static int our_srtcp_encrypt (void *foo, 
			     unsigned char *buffer, 
			     uint32_t *len)
{
  err_status_t err;
  int retdata;
  srtp_if_t *srtp_if = (srtp_if_t *)foo;
  uint32_t i;

  retdata = *len;
  if (DUMP_ENC_RTCP_PAK) {
    srtp_if_debug(LOG_DEBUG,"Calling srtp_protect_rtcp, len %d", *len);
    for (i = 0; i < *len; i++) {
      printf("%02x ", buffer[i]);
      if (((i + 1) % 16) == 0) printf("\n");
    }
    printf("\n");
  }

  MutexLock(srtp_if->mutex);
  err = srtp_protect_rtcp(srtp_if->out_ctx, (void *)buffer, &retdata);
  MutexUnlock(srtp_if->mutex);
  if (DUMP_RAW_RTCP_PAK) {
    for (i = 0; i < (uint32_t)retdata; i++) {
      printf("%02x ", buffer[i]);
      if (((i + 1) % 16) == 0) printf("\n");
    }
    printf("\n");
    srtp_if_debug(LOG_DEBUG,"exiting srtcp_encrypt %d %d", err, retdata);
  }

  if (err != 0) {
    srtp_if_debug(LOG_ERR, "failing srtcp encrypts error %d len %u", err, *len);
    return FALSE;
  }
  *len = retdata;
  return TRUE;
}

static int our_srtcp_decrypt (void *foo, 
			     unsigned char *buffer, 
			     uint32_t *len)
{
  err_status_t err;
  int retdata;
  srtp_if_t *srtp_if = (srtp_if_t *)foo;
  uint32_t i;

  retdata = *len;
  if (DUMP_ENC_RTCP_PAK) {
    srtp_if_debug(LOG_DEBUG,"Calling srtp_unprotect_rtcp, len %d", *len);
    for (i = 0; i < *len; i++) {
      printf("%02x ", buffer[i]);
      if (((i + 1) % 16) == 0) printf("\n");
    }
    printf("\n");
  }
  MutexLock(srtp_if->mutex);
  err = srtp_unprotect_rtcp(srtp_if->in_ctx, (void *)buffer, &retdata);
  MutexUnlock(srtp_if->mutex);
  if (DUMP_RAW_RTCP_PAK) {
    for (i = 0; i < (uint32_t)retdata; i++) {
      printf("%02x ", buffer[i]);
      if (((i + 1) % 16) == 0) printf("\n");
    }
    printf("\n");
    srtp_if_debug(LOG_DEBUG,"exiting srtcp_decrypt %d %d", err, retdata);
  }
  if (err != 0) {
    srtp_if_debug(LOG_ERR,"return from srtp_unprotect_rtcp %d len %d orig %u",err, retdata,
		  *len);
    return FALSE;
  }
  *len = retdata;
  return TRUE;
}

static uint8_t to_hex (const char ptr)
{
  if (isdigit(ptr)) {
    return (ptr - '0');
  }
  return (tolower(ptr) - 'a' + 10);
}

static bool string_to_hex (uint8_t *to, const char *from, uint to_len)
{
  while (*from != '\0') {
    if (to_len == 0) return false;
    *to = to_hex(*from++) << 4;
    *to |= to_hex(*from++);
    to++;
    to_len--;
  }
  return true;
}
#endif

#ifdef HAVE_SRTP
static void configure_cipher_auth (srtp_policy_t *policy, 
				   srtp_if_params_t *sparam) {

  assert(policy != 0);
  // bko -- protect
  crypto_policy_set_rtp_default(&policy->rtp);
  crypto_policy_set_rtcp_default(&policy->rtcp);

  if (sparam->rtp_enc == true) {
    srtp_if_debug(LOG_DEBUG, "configure_cipher_auth: RTP encryption selectd");
    if (sparam->enc_algo == SRTP_ENC_AES_CM_128) {
      srtp_if_debug(LOG_DEBUG, "configure_cipher_auth: 128bit RTP encryption");
      policy->rtp.cipher_type = AES_128_ICM;
      policy->rtp.cipher_key_len = 30;
    } else {
      srtp_if_debug(LOG_ALERT, "unrecognized cipher type for RTP");
      assert(0);
    }
  } else {
    srtp_if_debug(LOG_DEBUG, "configure_cipher_auth: null RTP encryption");
    policy->rtp.cipher_type     = NULL_CIPHER;
    policy->rtp.cipher_key_len  = 0; 
  }    

  if (sparam->rtp_auth == true) {
    if (sparam->auth_algo == SRTP_AUTH_HMAC_SHA1_80) {
      srtp_if_debug(LOG_DEBUG, "configure_cipher_auth: SHA1_80 auth");
      policy->rtp.auth_type = HMAC_SHA1;
      policy->rtp.auth_key_len = 20;
      policy->rtp.auth_tag_len = 10;
    } else if (sparam->auth_algo == SRTP_AUTH_HMAC_SHA1_32) {
      srtp_if_debug(LOG_DEBUG, "configure_cipher_auth: SHA1_32 auth");
      policy->rtp.auth_type = HMAC_SHA1;
      policy->rtp.auth_key_len = 20;
      policy->rtp.auth_tag_len = 4;    
    } else {
      srtp_if_debug(LOG_ALERT, "unrecognized auth type for RTP");
      assert(0);
    }

  } else {
    srtp_if_debug(LOG_DEBUG, "configure_cipher_auth: no auth");
    policy->rtp.auth_type       = NULL_AUTH;
    policy->rtp.auth_key_len    = 0;
    policy->rtp.auth_tag_len    = 0;    
  }

  if (sparam->rtp_enc == true && sparam->rtp_auth == true)
    policy->rtp.sec_serv = sec_serv_conf_and_auth;
  else if (sparam->rtp_enc == true && sparam->rtp_auth == false)
    policy->rtp.sec_serv = sec_serv_conf;
  else if (sparam->rtp_enc == false && sparam->rtp_auth == true)
    policy->rtp.sec_serv = sec_serv_auth;
  else
    policy->rtp.sec_serv = sec_serv_none;
  
  if (sparam->rtcp_enc == true) {
    srtp_if_debug(LOG_DEBUG, "configure_cipher_auth: enabling RTCP encrypt");
    policy->rtcp.sec_serv = sec_serv_conf_and_auth;
    if (sparam->enc_algo == SRTP_ENC_AES_CM_128) {
      policy->rtcp.cipher_type = AES_128_ICM;
      policy->rtcp.cipher_key_len = 30;
    } else {
      srtp_if_debug(LOG_ALERT, "unrecognized cipher type for RTCP");
      assert(0);
    }

    //rtcp is always authenticated
    if (sparam->auth_algo == SRTP_AUTH_HMAC_SHA1_80) {
      policy->rtcp.auth_type = HMAC_SHA1;
      policy->rtcp.auth_key_len = 20;
      policy->rtcp.auth_tag_len = 10;
    } else if (sparam->auth_algo == SRTP_AUTH_HMAC_SHA1_32) {
      policy->rtcp.auth_type = HMAC_SHA1;
      policy->rtcp.auth_key_len = 20;
      policy->rtcp.auth_tag_len = 4;    
    } else {
      srtp_if_debug(LOG_ALERT, "unrecognized auth type for RTCP");
      assert(0);
    }
  } else {
    //TODO check this
    policy->rtcp.sec_serv = sec_serv_auth;
    policy->rtcp.cipher_type     = NULL_CIPHER;
    policy->rtcp.cipher_key_len  = 0;
    //rtcp is always authenticated
    if (sparam->auth_algo == SRTP_AUTH_HMAC_SHA1_80) {
      policy->rtcp.auth_type = HMAC_SHA1;
      policy->rtcp.auth_key_len = 20;
      policy->rtcp.auth_tag_len = 10;
    } else if (sparam->auth_algo == SRTP_AUTH_HMAC_SHA1_32) {
      policy->rtcp.auth_type = HMAC_SHA1;
      policy->rtcp.auth_key_len = 20;
      policy->rtcp.auth_tag_len = 4;    
    } else {
      srtp_if_debug(LOG_ALERT, "unrecognized auth type for RTCP");
      assert(0);
    }
  }
  policy->next = NULL;

  srtp_if_debug(LOG_DEBUG, "SECURITY_SERVICES: %d, %d", sec_serv_conf, 
		policy->rtp.sec_serv);
}
#endif

srtp_if_t* srtp_setup (struct rtp *session, srtp_if_params_t *sparam)
{
  srtp_if_t *srtp_if = NULL;
#ifdef HAVE_SRTP
  err_status_t status;
  rtp_encryption_params_t rtp_params;
  uint8_t *tx_key, *rx_key;
  int ret;

  if (srtp_if_initialize() == false) return NULL;

  srtp_if = MALLOC_STRUCTURE(srtp_if_t);
  memset(srtp_if, 0, sizeof(srtp_if));
  
  srtp_if->mutex = MutexCreate();
  if (sparam->tx_key != NULL) {
    // create hex key and salt
    string_to_hex(srtp_if->tx_keysalt, sparam->tx_key, 16);
    string_to_hex(srtp_if->tx_keysalt + 16, sparam->tx_salt, 14);
    tx_key = srtp_if->tx_keysalt;
  } else {
    tx_key = sparam->tx_keysalt;
  }

  if (sparam->rx_key != NULL) {
    // create hex key and salt
    string_to_hex(srtp_if->rx_keysalt, sparam->rx_key, 16);
    string_to_hex(srtp_if->rx_keysalt + 16, sparam->rx_salt, 14);
    rx_key = srtp_if->rx_keysalt;
  } else {
    rx_key = sparam->rx_keysalt;
  }

  srtp_if->in_policy.key = rx_key;
  srtp_if->in_policy.ssrc.type = ssrc_any_inbound;
  srtp_if->in_policy.rtp.sec_serv = sec_serv_conf;
  srtp_if->in_policy.rtcp.sec_serv = sec_serv_none;
  srtp_if->out_policy.key = tx_key;
  srtp_if->out_policy.ssrc.type  = ssrc_any_outbound;

  configure_cipher_auth(&srtp_if->in_policy, sparam);
  configure_cipher_auth(&srtp_if->out_policy, sparam);

  //now create the struct for passing data to rtp
  status = srtp_create(&srtp_if->in_ctx, &srtp_if->in_policy);
  if (status) {
    srtp_if_debug(LOG_ALERT, "INbound policy create failed with code %d", 
		  status);
    return NULL;
  }

  assert(srtp_if->out_policy.rtcp.auth_key_len < 50);
  status = srtp_create(&srtp_if->out_ctx,&srtp_if->out_policy);
  if (status) {
    srtp_if_debug(LOG_ALERT, "OUTbound policy create failed with code %d", 
		  status);
    return NULL;
  }
  memset(&rtp_params, 0, sizeof(rtp_params));
  // huh ?  we're going to remove it from the mtu - this shouldn't be needed
  rtp_params.rtp_auth_alloc_extra = srtp_if->out_policy.rtp.auth_tag_len;
  rtp_params.rtcp_auth_alloc_extra= srtp_if->out_policy.rtcp.auth_tag_len;

  rtp_params.userdata = srtp_if;
  rtp_params.rtp_encrypt = our_srtp_encrypt;
  rtp_params.rtp_decrypt = our_srtp_decrypt;
  rtp_params.rtcp_encrypt = our_srtcp_encrypt;
  rtp_params.rtcp_decrypt = our_srtcp_decrypt;

  ret = rtp_set_encryption_params(session, &rtp_params);
  if (ret != 0) {
    destroy_srtp(srtp_if);
    srtp_if = NULL;
  }
#endif  
  return srtp_if;
}


bool
srtp_parse_sdp(const char *media_type, 
	       const char* crypto,
	       srtp_if_params_t* params)
{
#ifdef HAVE_SRTP
  const char *inptr, *base64;
  uint32_t key_size;
  uint8_t *key;
  bool rtp_enc = true, rtp_auth = true, rtcp_enc = true;
  uint kdr;
  uint32_t ix;

  if (crypto == NULL) {
    return false;
  }
  if(params == NULL) {
    return false;
  }
  
  memset(params, 0, sizeof(srtp_if_params_t));

  crypto += strlen("a=crypto");
  ADV_SPACE(crypto);
  if (*crypto != ':') {
    srtp_if_debug(LOG_ERR, "%s: no : after a=crypto", media_type);
    return false;
  }
  crypto++;
  while (!isspace(*crypto) && *crypto != '\0') crypto++;

  ADV_SPACE(crypto);
  // crypto should point to the algorithm

  if (strncasecmp(crypto, "AES_CM_128_HMAC_SHA1_80", 
		  strlen("AES_CM_128_HMAC_SHA1_80")) == 0) {
    crypto += strlen("AES_CM_128_HMAC_SHA1_80");
    params->enc_algo = SRTP_ENC_AES_CM_128;
    params->auth_algo = SRTP_AUTH_HMAC_SHA1_80;
  } else if (strncasecmp(crypto, "AES_CM_128_HMAC_SHA1_32", 
		  strlen("AES_CM_128_HMAC_SHA1_32")) == 0) {
    crypto += strlen("AES_CM_128_HMAC_SHA1_32");
    params->enc_algo = SRTP_ENC_AES_CM_128;
    params->auth_algo = SRTP_AUTH_HMAC_SHA1_32;
  } else {
    srtp_if_debug(LOG_ERR, "%s: unknown crypto format %s", media_type,
		  crypto);
    return false;
  }
  ADV_SPACE(crypto);
  inptr = strcasestr(crypto, "inline");
  if (inptr == NULL) {
    srtp_if_debug(LOG_ERR, "%s: can't find inline: in crypto", media_type);
    return false;
  }
  inptr += strlen("inline");
  ADV_SPACE(inptr);
  if (*inptr != ':') {
    srtp_if_debug(LOG_ERR, "%s: can't find : after inline", media_type);
    return false;
  }
  inptr++;
  ADV_SPACE(inptr);
  base64 = inptr;
  while (!isspace(*inptr) && *inptr != '|' && *inptr != '\0') {
    inptr++;
  }
  
  key = Base64ToBinary(base64, inptr - base64, &key_size);

  // clone the key to a 32 byte long thing, not because we need to,
  // but because libsrtp reads off the end (!)
  if((key != NULL) && (key_size < 32)) {
    uint8_t *tmp_key = malloc(32);
    memcpy(tmp_key, key, key_size);
    free(key);
    key = tmp_key;
  }

  if (key == NULL) {
    srtp_if_debug(LOG_ERR, "%s:Can't decode base 64", media_type);
    return false;
  }

  srtp_if_debug(LOG_DEBUG, "key size is %u", key_size);
#if 1
  printf("\nparsed key: ");
  for (ix = 0; ix < key_size; ix++) {
    printf("%02x", key[ix]);
  }
  printf("\n");
#endif
  if (*inptr == '|') {
    // need to parse rest of inline
  }

  if (strcasestr(crypto, "UNENCRYPTED_SRTP") != NULL) {
    rtp_enc = false;
  }
  if (strcasestr(crypto, "UNENCRYPTED_SRTCP") != NULL) {
    rtcp_enc = false;
  }
  if (strcasestr(crypto, "UNAUTHENTICATED_SRTP") != NULL) {
    rtp_auth = false;
  }
  inptr = strcasestr(crypto, "KDR=");

  kdr = 0;
  if (inptr != NULL) {
    inptr += strlen("KDR=");
    ADV_SPACE(inptr);
    kdr += *inptr - '0';
    inptr++;
    if (isdigit(*inptr)) {
      kdr *= 10;
      kdr += *inptr - '0';
      inptr++;
    }
    if (!isspace(*inptr) && *inptr != '\0') {
      srtp_if_debug(LOG_ERR, "%s:Illegal kdr value", media_type);
      return false;
    }
  }

  //TODO use the SDP values to configure cipher type instead of SRTP default
  params->rx_keysalt = key;
  params->rx_key = params->rx_salt = NULL;
  params->tx_keysalt = key; 
  params->rtp_enc = rtp_enc;
  params->rtcp_enc = rtcp_enc;
  params->rtp_auth = rtp_auth;
  return true;
#else
  return false;
#endif
}

srtp_if_t *srtp_setup_from_sdp (const char *media_type, 
				struct rtp *session, 
				const char *crypto)
{
#ifdef HAVE_SRTP
  srtp_if_params_t params;

  if(!srtp_parse_sdp(media_type, crypto, &params)) {
    return NULL;
  }

  return srtp_setup(session, &params);
#else
  return NULL;
#endif
  
}

char *srtp_create_sdp (const char *type,
		       srtp_if_params_t *params)
{
#ifdef HAVE_SRTP
  char buffer[512];
  uint8_t saltkey[30];
  char *base64;
  const char *crypt_type;

  if (params->tx_key != NULL) {
    if (strlen(params->tx_key) != 32) {
      srtp_if_debug(LOG_ALERT,"key for media %s is %u, not 32", type, strlen(params->tx_key));
      return NULL;
    }
    if (strlen(params->tx_salt) != 28) {
      srtp_if_debug(LOG_ALERT,"salt for media %s is %u, not 28", type, strlen(params->tx_salt));
      return NULL;
    }
    srtp_if_debug(LOG_DEBUG, "stream %s key:%s salt %s", type, 
		  params->tx_key, params->tx_salt);
    string_to_hex(saltkey, params->tx_key, 30);
    string_to_hex(saltkey + 16, params->tx_salt, 14);
    params->tx_keysalt = saltkey;
  }

  base64 = MP4BinaryToBase64(params->tx_keysalt, 30);

  
  if (params->auth_algo == SRTP_AUTH_HMAC_SHA1_80) {
    crypt_type = "AES_CM_128_HMAC_SHA1_80";
  } else {
    crypt_type = "AES_CM_128_HMAC_SHA1_32";
  }

  snprintf(buffer, sizeof(buffer), 
	   "a=crypto:1 %s inline:%s %s%s%s",
	   crypt_type,
	   base64,
	   params->rtp_enc ? "" : "UNENCRYPTED_SRTP ",
	   params->rtp_auth ? "" : "UNAUTHENTICATED_SRTP ",
	   params->rtcp_enc ? "" : "UNENCRYPTED_SRTCP ");

  free(base64);
  return strdup(buffer);
#else
  return NULL;
#endif
}

void destroy_srtp(srtp_if_t *srtp_if) {
#ifdef HAVE_SRTP
  srtp_dealloc(srtp_if->in_ctx);
  srtp_dealloc(srtp_if->out_ctx);
  //what about policy?
  CHECK_AND_FREE(srtp_if);
#endif
}
