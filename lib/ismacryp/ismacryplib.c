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
 *              Will Clark              will_clark@cisco.com
 */

// define this for debug output
//#define ISMACRYP_ENC_DEBUG 1
//#define ISMACRYP_DEC_DEBUG 1

#include "ismacryplib_priv.h"

static ismacryp_session_id_t session_g = ISMACRYP_MIN_SESSION_ID;
static ismacryp_session_t *sessionList = NULL;

char ismacryp_keytypeStr[3][25] = { "KeyTypeOther", "KeyTypeVideo", "KeyTypeAudio" };


static uint32_t FOUR_CHAR_CODE (char *code)
{
  return code[0]<<24 | code[1]<<16 | code[2]<<8 | code[3];
}

#ifdef HAVE_SRTP
static uint8_t
local_nibble_to_hex_char(uint8_t nibble) {
  char buf[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
		  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
  return buf[nibble & 0xF];
}

static char *
v64_hex_string(const v64_t *x) {
  int i, j;
  char bit_string[1024];

  for (i=j=0; i < 8; i++) {
    bit_string[j++]  = local_nibble_to_hex_char(x->v8[i] >> 4);
    bit_string[j++]  = local_nibble_to_hex_char(x->v8[i] & 0xF);
  }
  
  bit_string[j] = 0; 
  return strdup(bit_string);
}
#endif

static void addToSessionList (ismacryp_session_t *sp) {

  ismacryp_session_t *temp;

  // critical section
  if (sessionList == NULL) {
     sessionList = sp;
     return;
     // end critical section 
  }

  temp = sessionList;
  while (temp->next != NULL)
     temp = temp->next;
  temp->next=sp;
  sp->prev = temp; 
  // end critical section

}

static ismacryp_rc_t removeFromSessionList (ismacryp_session_id_t sid) {

  ismacryp_session_t *temp1, *temp2;

  // critical section
  if (sessionList == NULL ) {
     fprintf(stdout, "Error. Try to remove session from empty list.\n");
     // end critical section
     return ismacryp_rc_sessid_error;
  }

  temp1=sessionList;

  // item to be removed is first in list
  if (temp1->sessid == sid ) { 
     if ( temp1->next == NULL ) {
         sessionList = NULL;
         // end critical section
         if (temp1->kms_uri)
            free(temp1->kms_uri);
         free(temp1);
         return ismacryp_rc_ok;
     }

     sessionList = sessionList->next;
     sessionList->prev = NULL;
     // end critical section
     if (temp1->kms_uri)
        free(temp1->kms_uri);
     free(temp1);
     return ismacryp_rc_ok;
    
  }

  // item to be removed is not first in list
  while(temp1->next != NULL) {
     temp2 = temp1->next;
     if(temp2->sessid == sid ) {
          if (temp2->next != NULL )
             temp2->next->prev = temp1;
          temp1->next = temp2->next;
          // end critical section
          if (temp2->kms_uri)
              free(temp2->kms_uri);
          free(temp2); 
          return ismacryp_rc_ok;
     }
     temp1 = temp1->next;
  }
  // end critical section


  fprintf(stdout, "Error. Try to remove non -existent session: %d\n", sid);
  // end critical section
  return ismacryp_rc_sessid_error;

}

static ismacryp_rc_t findInSessionList (ismacryp_session_id_t sid, ismacryp_session_t **s) {

  ismacryp_session_t *temp = sessionList;

  if (sessionList == NULL) {
     *s=NULL;
     fprintf(stdout, "Error. Try to find session in empty list.\n");
     return ismacryp_rc_sessid_error;
  }

  if (temp->sessid == sid) {
     *s = temp;
     return ismacryp_rc_ok;
  }

  while(temp->next != NULL) {
     temp=temp->next;  
     if(temp->sessid == sid) { 
        *s = temp;
         return ismacryp_rc_ok;
     }
  }

  fprintf(stdout, "Error. Try to find unknown session in list. sid: %d\n", sid);
  *s = NULL;
  return ismacryp_rc_sessid_error;
}

#ifdef HAVE_SRTP
static void printSessionList (void) {
  ismacryp_session_t *temp;
  int i = 0;

  fprintf(stdout, "Session List:\n");

  if (sessionList == NULL) {
     fprintf(stdout, " -- EMPTY --\n");
     return;
  }

  temp=sessionList;
  while(temp != NULL) {
     i++;
     fprintf(stdout, " -- Session #%d: session id: %d \n", i, temp->sessid );
     fprintf(stdout, "                 key l: %d salt l: %d ctr l: %d iv l: %d key t: %c\n",
                                       AES_KEY_LEN, AES_SALT_LEN, AES_COUNTER_LEN, 
                                       temp->IV_len, 
                                       ismacryp_keytypeStr[temp->key_type][7]);
     fprintf(stdout, "                 key : %s", 
#ifdef HAVE_SRTP
                octet_string_hex_string(temp->kk.ksc.key, AES_KEY_LEN)
#else
                "n/a"
#endif
                );
     fprintf(stdout, "\n");
     fprintf(stdout, "                 salt: %s", 
#ifdef HAVE_SRTP
                octet_string_hex_string(temp->kk.ksc.salt, AES_SALT_LEN)
#else
                "n/a"
#endif
                );
     fprintf(stdout, "\n");
     fprintf(stdout, "                 ctr : %s", 
#ifdef HAVE_SRTP
                octet_string_hex_string(temp->kk.ksc.counter, AES_COUNTER_LEN)
#else
                "n/a"
#endif
                );
     fprintf(stdout, "\n");
     temp=temp->next;
  }

}
#endif
//
// load key from file
// allocate and init aes_icm cipher and key
//
static ismacryp_rc_t loadKeyFromFile (ismacryp_session_t *sp,
                               ismacryp_keytype_t keytype,
                               const char *kmsfile)
{

#ifdef HAVE_SRTP
  FILE *fp;
  int i;
  char kms_data_file[KMS_DATA_FILE_FILENAME_MAX_LEN];
  char kms_data[KMS_DATA_FILE_MAX_LINE_LEN+1];
  char temp[25];
  size_t pathlen;
  size_t filenamelen;
  int foundKey = FALSE;
  int len;
#endif

#ifdef HAVE_SRTP
  // get the key material
  // NULL case, key and salt have been memset to 0, nothing breaks.

  strncpy(kms_data_file,getenv("HOME"),KMS_DATA_FILE_FILENAME_MAX_LEN);
  pathlen = strlen(kms_data_file);
  filenamelen = strlen(KMS_DATA_FILE);
  if ( (pathlen + filenamelen + 1) > KMS_DATA_FILE_FILENAME_MAX_LEN ) { // +1 for '/'
     fprintf(stdout,"key file name too long\n");
     return ismacryp_rc_keyfilename_error;
  }
  kms_data_file[pathlen+1] = kms_data_file[pathlen];
  kms_data_file[pathlen] = '/';
  strncpy(&kms_data_file[pathlen+1], KMS_DATA_FILE, filenamelen );

  // explicit override default kms file
  if (kmsfile)
  {
	// trim max length absent stdlib min()
	len = strlen(kmsfile);
	len = (len > KMS_DATA_FILE_FILENAME_MAX_LEN) ? KMS_DATA_FILE_FILENAME_MAX_LEN : len;
  	strncpy(kms_data_file, kmsfile, len);
	kms_data_file[len] = 0;
  }

  switch( keytype) {
  case KeyTypeVideo:
    strcpy(temp,KMS_DATA_VIDEOKEY_STR);
    break;
  case KeyTypeAudio:
    strcpy(temp,KMS_DATA_AUDIOKEY_STR);
    break;
  case KeyTypeOther:
  default:
    fprintf(stdout,"Unsupported key type: %d\n",keytype);
    return ismacryp_rc_keytype_error;
  }

  if ( !(fp=fopen(kms_data_file,"r")) ) {
     fprintf(stdout,"Can't open kms file: %s\n",kms_data_file);
     return(ismacryp_rc_keyfile_error);
  }

  i = 0;
  while ( fgets(kms_data,KMS_DATA_FILE_MAX_LINE_LEN,fp))  {
     len = strlen(kms_data);
     kms_data[len-1] = kms_data[len]; // get rid of newline
     i++;

     if ( !strncmp(kms_data, temp, strlen(KMS_DATA_AUDIOKEY_STR)) ) {
       for (i=0;i<AES_KEY_SALT_LEN;i++) {
	 uint temp;
	 fscanf(fp, "%x", &temp);
	 sp->kk.aes_overlay[i] = temp;
       }
       foundKey = TRUE;
       break;
     }
  }
  if ( !foundKey ) {
     fprintf(stdout, "Can't find %s\n", temp);
     fclose(fp);
     return ismacryp_rc_key_error;
  }

  fclose(fp);

#endif
  return ismacryp_rc_ok;


}

//
// allocate aes_icm cipher and  initialize aes icm 
// crypto context.
//
// invoked using session ID making it universal
// for internal and external use.
//
ismacryp_rc_t initSessionData (ismacryp_session_id_t session) 
{
#ifdef HAVE_SRTP
  err_status_t rc;
#endif
  ismacryp_rc_t irc;
  ismacryp_session_t *sp;

  // get sp by session ID
  irc = findInSessionList (session, &sp); 
  if (irc != ismacryp_rc_ok)
	return irc;
 
  if (sp == NULL) {
    fprintf(stdout, "Error. Try to init NULL session.\n");
    return ismacryp_rc_sessid_error;
  }

#ifdef HAVE_SRTP
  rc = err_status_ok;

  // Allocate cipher.
  //fprintf(stdout," - allocate cipher for session %d\n", session);
  rc=aes_icm_alloc_ismacryp(&(sp->cp), AES_KEY_SALT_LEN, 1);
  if ( rc != err_status_ok ) {
      fprintf(stdout," - allocate cipher for session %d FAILED  rc = %d\n", 
	session, rc );
      return ismacryp_rc_cipheralloc_error;
  }

  // Init cipher.
  rc=aes_icm_context_init(sp->cp->state, sp->kk.aes_overlay);
  if ( rc != err_status_ok ) {
      fprintf(stdout," - init cipher for session %d FAILED  rc = %d\n", session,
                                     rc );
      return ismacryp_rc_cipherinit_error;
  }
#endif

  #if defined(ISMACRYP_ENC_DEBUG) || defined(ISMACRYP_DEC_DEBUG)
  // diagnostic
  printSessionList();
  #endif // ISMACRYP_ENC_DEBUG || ISMACRYP_DEC_DEBUG

  return ismacryp_rc_ok;
}


//
// dealloc aes_icm cipher
//
// invoked using session ID making it universal
// for internal and external use.
//
ismacryp_rc_t unInitSessionData (ismacryp_session_id_t session) 
{
#ifdef HAVE_SRTP
  err_status_t rc;
#endif
  ismacryp_rc_t irc;
  ismacryp_session_t *sp;

  // get sp by session ID
  irc = findInSessionList (session, &sp); 
  if (irc != ismacryp_rc_ok)
	return irc;

  if (sp == NULL) {
    fprintf(stdout, "Error. Try to uninit NULL session.\n");
    return ismacryp_rc_sessid_error;
  }

#ifdef HAVE_SRTP
  rc = err_status_ok;
  rc=aes_icm_dealloc(sp->cp);
#endif

  return ismacryp_rc_ok;
}

//
// init ismacryp lib
//
ismacryp_rc_t ismacrypInitLib (void)
{
  return ismacryp_rc_ok;
}

//
// init ismacryp session
//
ismacryp_rc_t ismacrypInitSession (ismacryp_session_id_t *session,
                                   ismacryp_keytype_t keytype )
{
  ismacryp_session_t *sp = NULL;
  ismacryp_rc_t  rc;
  int len;
  char *temp;

  sp = malloc(sizeof(ismacryp_session_t));
  if ( sp == NULL ) {
     fprintf(stdout, "Init Session: %d malloc FAILED keytype %c\n", *session, 
                                                         ismacryp_keytypeStr[keytype][7]);
     *session = 0;
     return ismacryp_rc_memory_error;
  }
  memset(sp,0,sizeof(*sp));

  // critical section
  *session = session_g++;
  // critical section

  #if defined(ISMACRYP_ENC_DEBUG) || defined(ISMACRYP_DEC_DEBUG)
  fprintf(stdout, "Init Session: %d with keytype %c\n", *session, 
                                                         ismacryp_keytypeStr[keytype][7]);
  #endif // ISMACRYP_ENC_DEBUG || ISMACRYP_DEC_DEBUG 

  sp->sessid = *session;
  sp->next = NULL;
  sp->prev = NULL;

  // load default keys from file
  rc =  loadKeyFromFile(sp, keytype, 0);
  if( rc != ismacryp_rc_ok ) {
     fprintf(stdout, "Init Session: %d WARNING keytype %c default keys file\n",
		*session, ismacryp_keytypeStr[keytype][7]);
  }

  sp->keycount      = 1;
  sp->sample_count  = 0;
  sp->BSO           = 0;
  sp->IV_len        = ISMACRYP_DEFAULT_IV_LENGTH;
  sp->deltaIV_len   = ISMACRYP_DEFAULT_DELTA_IV_LENGTH;
  sp->key_type      = keytype;
  sp->selective_enc = FALSE;

  // store default kms_uri
  len = strlen(KMS_URI_STR) + 1; // add 1 for null char
  temp = malloc(len); 
  if ( temp == NULL ) {
      fprintf(stderr, "save kms_uri: FAILED for session %d\n", *session);
      *session = 0;
      free (sp);
      return ismacryp_rc_memory_error;
  }
  strncpy(temp, KMS_URI_STR, len);
  sp->kms_uri = temp;

  addToSessionList(sp); 
  #if defined(ISMACRYP_ENC_DEBUG) || defined(ISMACRYP_DEC_DEBUG)
  printSessionList();
  #endif // ISMACRYP_ENC_DEBUG || ISMACRYP_DEC_DEBUG

  // init cipher context
  return initSessionData(*session);

}

ismacryp_rc_t ismacrypEndSession (ismacryp_session_id_t session)
{
  ismacryp_session_t *sp;  

  if( findInSessionList(session, &sp) ) {
     fprintf(stderr, "\nEnd Session: %d FAILED\n", session);
     return ismacryp_rc_sessid_error;
  }

  unInitSessionData(session);
  removeFromSessionList(session); 
  #if defined(ISMACRYP_ENC_DEBUG) || defined(ISMACRYP_DEC_DEBUG)
  printSessionList();
  #endif // ISMACRYP_ENC_DEBUG || ISMACRYP_DEC_DEBUG
  return ismacryp_rc_ok;
}

ismacryp_rc_t ismacrypSetScheme (ismacryp_session_id_t session,
                                 ismacryp_scheme_t scheme)
{
  return ismacryp_rc_ok;
}

ismacryp_rc_t ismacrypGetScheme (ismacryp_session_id_t session,
                                 ismacryp_scheme_t *scheme)
{
  *scheme = FOUR_CHAR_CODE(ISMACRYP_DEFAULT_SCHEME);
  return ismacryp_rc_ok;
}

ismacryp_rc_t ismacrypSetSchemeVersion (ismacryp_session_id_t session,
                                        ismacryp_schemeversion_t schemeversion)
{
  return ismacryp_rc_ok;
}

ismacryp_rc_t ismacrypGetSchemeVersion (ismacryp_session_id_t session,
                                        ismacryp_schemeversion_t *schemeversion)
{
  *schemeversion = ISMACRYP_DEFAULT_SCHEME_VERSION;
  return ismacryp_rc_ok;
}

//
// unload current cipher if it exists, 
// load new cipher from file referenced by kms_uri
//
// save kms_uri associated with this session
//
ismacryp_rc_t ismacrypSetKMSUri (ismacryp_session_id_t session,
                                 char *kms_uri )
{
  ismacryp_rc_t rc;
  ismacryp_session_t *sp;
  int len;
  char *temp;

  if (!session)
     return ismacryp_rc_sessid_error;

  // get sp by session ID
  rc = findInSessionList (session, &sp); 
  if (rc != ismacryp_rc_ok)
	return rc;

  // deallocate current cipher
  rc = unInitSessionData(session);

  // load new key from file
  rc = loadKeyFromFile (sp,	
                      sp->key_type,	
                      kms_uri);

  // update cipher context
  rc = initSessionData(session);

  // store kms_uri
  len = strlen(kms_uri) + 1; // add 1 for null char
  temp = malloc(len); 
  if ( temp == NULL ) {
      fprintf(stderr, "save kms_uri: FAILED for session %d\n", session);
      return ismacryp_rc_memory_error;
  }
  strncpy(temp, kms_uri, len);
  sp->kms_uri = temp;

  #if defined(ISMACRYP_ENC_DEBUG) || defined(ISMACRYP_DEC_DEBUG)
  printSessionList();
  #endif // ISMACRYP_ENC_DEBUG || ISMACRYP_DEC_DEBUG

  return rc;

}

//
//  retrieve kms_uri associated with this session
// 
ismacryp_rc_t ismacrypGetKMSUri (ismacryp_session_id_t session,
                                 const char **kms_uri )
{

ismacryp_session_t *sp;
int len;
char *temp;
ismacryp_rc_t rc;

  // get sp from session
  if (!session)
     return ismacryp_rc_sessid_error;

  // get sp by session ID
  rc = findInSessionList (session, &sp); 
  if (rc != ismacryp_rc_ok)
	return rc;

  len = strlen(sp->kms_uri); 
  temp = malloc(len); 
  if ( temp == NULL ) {
      fprintf(stderr, "get kms uri: FAILED for session %d\n", session);
      return ismacryp_rc_memory_error;
  }
  strncpy(temp, sp->kms_uri, len);
  *kms_uri = temp;
  return ismacryp_rc_ok;
}

ismacryp_rc_t ismacrypSetSelectiveEncryption (ismacryp_session_id_t session,
                                              uint8_t selective_is_on )
{
  ismacryp_session_t *sp;

  if (findInSessionList(session, &sp)) {
     fprintf(stderr, "\nFailed to set selective encryption. Unknown session %d\n", session);
     return ismacryp_rc_sessid_error;
  }

  sp->selective_enc = selective_is_on;
  return ismacryp_rc_ok;
}

ismacryp_rc_t ismacrypGetSelectiveEncryption (ismacryp_session_id_t session,
                                              uint8_t *selective_is_on )
{
  ismacryp_session_t *sp;

  if (findInSessionList(session, &sp)) {
    fprintf(stderr, "\nFailed to get selective encryption. Unknown session %d\n", session);
    return ismacryp_rc_sessid_error;
  }

  *selective_is_on = sp->selective_enc;
  return ismacryp_rc_ok;

}

ismacryp_rc_t ismacrypSetKeyIndicatorLength (ismacryp_session_id_t session,
                                              uint8_t key_indicator_len)
{
  return ismacryp_rc_ok;
}

ismacryp_rc_t ismacrypGetKeyIndicatorLength (ismacryp_session_id_t session,
                                              uint8_t *key_indicator_len)
{
  *key_indicator_len = ISMACRYP_DEFAULT_KEYINDICATOR_LENGTH;
  return ismacryp_rc_ok;
}

ismacryp_rc_t ismacrypGetKeyIndPerAU (ismacryp_session_id_t session,
                                      uint8_t *key_ind_per_au)
{
  *key_ind_per_au = ISMACRYP_DEFAULT_KEYINDICATOR_PER_AU;
  return ismacryp_rc_ok;
}

ismacryp_rc_t ismacrypGetKeyCount (ismacryp_session_id_t session,
                                   uint8_t *keycount)
{
  ismacryp_session_t *sp;

  if (findInSessionList(session, &sp)) {
    fprintf(stderr, "\nFailed to get key count. Unknown session %d\n", session);
    return ismacryp_rc_sessid_error;
  }

  *keycount = sp->keycount;
  return ismacryp_rc_ok;
}

//
//	This API copies key and salt to malloc buffer.
//      the key and salt pointers are updated to 
// 	point at the malloc buffers. the application
// 	must free these buffers when no longer needed.
// 
ismacryp_rc_t ismacrypGetKey (ismacryp_session_id_t session,
                              uint8_t key_num,
                              uint8_t *key_len,
                              uint8_t *salt_len,
                              uint8_t **key,
                              uint8_t **salt,
                              uint8_t *lifetime_exp)
{
 
  int i;
  ismacryp_session_t *sp;
  uint8_t *tempk, *temps;
  // only support one key for now so key_num is irrelevant
  if (findInSessionList(session, &sp)) {
    fprintf(stderr, "\nFailed to get key. Unknown session %d\n", session);
    return ismacryp_rc_sessid_error;
  }

  *key_len      = AES_KEY_LEN;
  *salt_len     = AES_SALT_LEN;
  *lifetime_exp = ISMACRYP_DEFAULT_KEY_LIFETIME_EXP;

  tempk = (uint8_t *) malloc((size_t) *key_len);
  temps = (uint8_t *) malloc((size_t) *salt_len);
   
  if ( tempk == NULL || temps == NULL ) {
     CHECK_AND_FREE(tempk);
     CHECK_AND_FREE(temps);
     fprintf(stdout, "\nFailed to get key mem error. Session %d\n", session);
     return ismacryp_rc_memory_error;
  }

  for (i=0; i<*key_len; i++ ) {
    tempk[i] = sp->kk.ksc.key[i];
  }
  *key = tempk;

  for (i=0; i<*salt_len; i++ ) {
    temps[i] = sp->kk.ksc.salt[i];
  }
  *salt = temps;

  return ismacryp_rc_ok;

}
                            
//
//	This API has the same invocation signature
//	as ismacrypGetKey, but instead of making
//	copies of key and salt to malloc buffers,
//	this API instead updates key and salt pointers to the 
//	live key and salt buffers.  The application must NOT 
//	attempt to free the key and salt pointers!
// 
ismacryp_rc_t ismacrypGetKey2 (ismacryp_session_id_t session,
                              uint8_t key_num,
                              uint8_t *key_len,
                              uint8_t *salt_len,
                              uint8_t **key,
                              uint8_t **salt,
                              uint8_t *lifetime_exp)
{
 
  ismacryp_session_t *sp;
  // only support one key for now so key_num is irrelevant
  if (findInSessionList(session, &sp)) {
    fprintf(stderr, "\nFailed to get key. Unknown session %d\n", session);
    return ismacryp_rc_sessid_error;
  }

  *key_len      = AES_KEY_LEN;
  *salt_len     = AES_SALT_LEN;
  *lifetime_exp = ISMACRYP_DEFAULT_KEY_LIFETIME_EXP;

  *key = sp->kk.ksc.key;
  *salt = sp->kk.ksc.salt;

  return ismacryp_rc_ok;

}
                            


ismacryp_rc_t ismacrypSetIVLength (ismacryp_session_id_t session,
                                   uint8_t iv_len)
{
  ismacryp_session_t *sp;

  if (findInSessionList(session, &sp)) {
     fprintf(stderr, "Failed to  set IV length. Unknown session %d\n", session);
     return ismacryp_rc_sessid_error;
  }

  sp->IV_len = iv_len;
  return ismacryp_rc_ok;
}

ismacryp_rc_t ismacrypGetIVLength (ismacryp_session_id_t session,
                                   uint8_t *iv_len)
{
  ismacryp_session_t *sp;

  if (findInSessionList(session, &sp)) {
     fprintf(stderr, "Failed to get IV length. Unknown session %d \n", session);
     return ismacryp_rc_sessid_error;
  }

  *iv_len = sp->IV_len;
  return ismacryp_rc_ok;
}

ismacryp_rc_t ismacrypSetDeltaIVLength (ismacryp_session_id_t session,
                                         uint8_t delta_iv_len)
{
  ismacryp_session_t *sp;

  if (findInSessionList(session, &sp)) {
     fprintf(stderr, "Failed to set deltaIV length. Unknown session %d \n", session);
     return ismacryp_rc_sessid_error;
  }
  if ( delta_iv_len > ISMACRYP_MAX_DELTA_IV_LENGTH ) {
     fprintf(stderr, "Can't set deltaIV length for session %d, illegal length: %d . \n",
             session, delta_iv_len);
     return ismacryp_rc_protocol_error;
  }

  sp->deltaIV_len = delta_iv_len;
  return ismacryp_rc_ok;
} 

ismacryp_rc_t ismacrypGetDeltaIVLength (ismacryp_session_id_t session,
                                        uint8_t *delta_iv_len)
{
  ismacryp_session_t *sp;

  if (findInSessionList(session, &sp)) {
     fprintf(stderr, "Failed to get delta IV length. Unknown session %d \n", session);
     return ismacryp_rc_sessid_error;
  }

  *delta_iv_len = sp->deltaIV_len;
  return ismacryp_rc_ok;
}

uint32_t ismacrypEncryptSampleAddHeader2 (uint32_t session,
                                          uint32_t length,
                                          uint8_t *data,
                                          uint32_t *new_length,
                                          uint8_t **new_data) 
{
  ismacryp_rc_t rc = 
    ismacrypEncryptSampleAddHeader ( (ismacryp_session_id_t) session,
                                     length, data, new_length, new_data );

  return (uint32_t) rc;
}

ismacryp_rc_t ismacrypEncryptSampleAddHeader (ismacryp_session_id_t session,
                                              uint32_t length,
                                              uint8_t *data,
                                              uint32_t *new_length,
                                              uint8_t **new_data)
{
  ismacryp_session_t *sp;
#ifdef HAVE_SRTP
  err_status_t rc = err_status_ok;
  uint8_t  nonce[AES_KEY_LEN];
#endif
  uint8_t  *temp_data;
  int      header_length;

  if (findInSessionList(session, &sp)) {
     fprintf(stderr, "Failed to encrypt+add header. Unknown session %d \n", session);
     return ismacryp_rc_sessid_error;
  }

  sp->sample_count++;

  if (sp->selective_enc  ) {
         fprintf(stderr,"    Selective encryption is not supported.\n");
         return ismacryp_rc_unsupported_error;
  }
  else {
        header_length = ISMACRYP_DEFAULT_KEYINDICATOR_LENGTH +
                        sp->IV_len;

#ifdef ISMACRYP_ENC_DEBUG
        fprintf(stdout,"E s: %d, #%05d. l: %5d BSO: %6d IV l: %d ctr: %s left: %d\n", 
                       sp->sessid, sp->sample_count, length, sp->BSO, sp->IV_len,
#ifdef HAVE_SRTP
		v64_hex_string((v64_t *)&(((aes_icm_ctx_t *)(sp->cp->state))->counter.v64[1])),
                       ((aes_icm_ctx_t *)(sp->cp->state))->bytes_in_buffer
#else
                       "n/a",
                       0
#endif
                       );
#endif // ISMACRYP_ENC_DEBUG

        *new_length = header_length + length;
        //fprintf(stdout,"     session: %d  length : %d  new length : %d\n", 
        //                     sp->sessid, length, *new_length);
        temp_data = (uint8_t *) malloc((size_t) *new_length);
        if ( temp_data == NULL ) {
            fprintf(stderr, "Failed to encrypt+add header, mem error. Session %d \n", session);
            return ismacryp_rc_memory_error; 
        }
        memcpy( &temp_data[header_length], data, length);
        memset(temp_data,0,header_length);

        // this is where to set IV which for encryption is BSO
        *((uint32_t *)(&temp_data[header_length-sizeof(uint32_t)])) = htonl(sp->BSO);
         
        // increment BSO after setting IV
        sp->BSO+=length;
   }

#ifdef HAVE_SRTP
   if ( sp->sample_count == 1 ) {
       memset(nonce,0,AES_KEY_LEN);
       //rc=aes_icm_set_segment(sp->cp->state, 0); // defunct function.
       rc=aes_icm_set_iv(sp->cp->state, nonce);
   }

   // length will not be updated in calling function (obviously) awv.
   rc=aes_icm_encrypt_ismacryp(sp->cp->state, &temp_data[header_length],
			       &length, 1);
   if (rc != err_status_ok) {
        free(new_data);
        new_data = NULL;
        fprintf(stderr, "Failed to encrypt+add header. aes error %d %d \n", session, rc);
        return ismacryp_rc_encrypt_error;
   }
#endif

   *new_data = temp_data;
   return ismacryp_rc_ok;
}

ismacryp_rc_t ismacrypEncryptSample (ismacryp_session_id_t session,
                                     uint32_t length,
                                     uint8_t *data)
{
  ismacryp_session_t *sp;
#ifdef HAVE_SRTP
  err_status_t rc = err_status_ok;
  uint8_t  nonce[AES_KEY_LEN];
#endif

  if (findInSessionList(session, &sp)) {
     fprintf(stderr, "Failed to encrypt. Unknown session %d \n", session);
     return ismacryp_rc_sessid_error;
  }

  sp->sample_count++;
#ifdef ISMACRYP_ENC_DEBUG 
  fprintf(stdout,"E s: %d, #%05d. l: %5d BSO: %6d IV l: %d ctr: %s left: %d\n", 
                       sp->sessid, sp->sample_count, length, sp->BSO, sp->IV_len,
#ifdef HAVE_SRTP
	  v64_hex_string((v64_t *)&(((aes_icm_ctx_t *)(sp->cp->state))->counter.v64[1])),
                       ((aes_icm_ctx_t *)(sp->cp->state))->bytes_in_buffer
#else
                       "n/a",
                       0
#endif
                       );
#endif // ISMACRYP_ENC_DEBUG 

#ifdef HAVE_SRTP
  if ( sp->sample_count == 1 ) {
      memset(nonce,0,AES_KEY_LEN);
      //rc=aes_icm_set_segment(sp->cp->state, 0); // defunct function.
      rc=aes_icm_set_iv(sp->cp->state, nonce);
  }

  // length will not be updated in calling function (obviously) awv.
  rc=aes_icm_encrypt_ismacryp(sp->cp->state, data,&length, 1);
#endif

  return ismacryp_rc_ok;

}

ismacryp_rc_t ismacrypDecryptSampleRemoveHeader (ismacryp_session_id_t session,
                                                 uint32_t length,
                                                 uint8_t *data,
                                                 uint32_t *new_length,
                                                 uint8_t **new_data)
{
  ismacryp_session_t *sp;

  uint8_t  *temp_data;
  int      header_length;
  uint32_t *IV;

  if (findInSessionList(session, &sp)) {
     fprintf(stdout, "Failed to decrypt+remove header. Unknown session %d \n", session);
     return ismacryp_rc_sessid_error;
  }

  sp->sample_count++;


  if (sp->selective_enc  ) {
         fprintf(stdout,"    Selective encryption is not supported.\n");
         return ismacryp_rc_unsupported_error;
  }
  else {
        header_length = ISMACRYP_DEFAULT_KEYINDICATOR_LENGTH +
                        sp->IV_len;

        IV = (uint32_t *)(&data[header_length - sizeof(uint32_t)]);

        *new_length = length - header_length;
        temp_data = (uint8_t *) malloc((size_t) *new_length);
        if ( temp_data == NULL ) {
            fprintf(stdout, "Failed to decrypt+remove header, mem error. Session %d \n", session);
            return ismacryp_rc_memory_error; 
        }
        memcpy(temp_data, &data[header_length], *new_length);
  }
  ismacrypDecryptSampleRandomAccess(session, ntohl(*IV), *new_length, temp_data);

  *new_data = temp_data;
  return ismacryp_rc_ok;
}

ismacryp_rc_t ismacrypDecryptSample (ismacryp_session_id_t session,
                                     uint32_t length,
                                     uint8_t *data)
{
  ismacryp_session_t *sp;
#ifdef HAVE_SRTP
  err_status_t rc = err_status_ok;
  uint8_t  nonce[AES_KEY_LEN];
#endif

  if (findInSessionList(session, &sp)) {
     fprintf(stdout, "Failed to decrypt. Unknown session %d \n", session);
    return ismacryp_rc_sessid_error;
  }

  sp->sample_count++;

#ifdef HAVE_SRTP
  if ( sp->sample_count == 1 ) {
      memset(nonce,0,AES_KEY_LEN);
      //rc=aes_icm_set_segment(sp->cp->state, 0); // defunct function.
      rc=aes_icm_set_iv(sp->cp->state, nonce);
  }
#endif

  fprintf(stdout,"D s: %d  #%05d  L: %5d  Ctr: %s  Left: %d\n", 
                       sp->sessid, sp->sample_count, length,
#ifdef HAVE_SRTP
	  v64_hex_string((v64_t *)&(((aes_icm_ctx_t *)(sp->cp->state))->counter.v64[1])),
                       ((aes_icm_ctx_t *)(sp->cp->state))->bytes_in_buffer
#else
                       "n/a",
                       0
#endif
                       );


#ifdef HAVE_SRTP
  // length will not be updated in calling function (obviously) awv.
  rc=aes_icm_encrypt_ismacryp(sp->cp->state, data,&length, 1);
#endif

  return ismacryp_rc_ok;

}

ismacryp_rc_t ismacrypDecryptSampleRandomAccess (
                                    ismacryp_session_id_t session,
                                    uint32_t BSO,
                                    uint32_t length,
                                    uint8_t *data)
{ 
  ismacryp_session_t *sp;
#ifdef HAVE_SRTP
  err_status_t rc = err_status_ok;
  uint8_t   fakedata[16];
#endif
  uint32_t  counter;
  uint32_t  remainder;
  uint8_t   nonce[AES_KEY_LEN];
  
  if (findInSessionList(session, &sp)) {
     fprintf(stdout, "Failed to decrypt random access. Unknown session %d \n", session);
     return ismacryp_rc_sessid_error;
  }

  // calculate counter from BSO
  counter = BSO/AES_BYTES_PER_COUNT;
  remainder = BSO%AES_BYTES_PER_COUNT;
  if ( remainder ) {
       // a non-zero remainder means that the key corresponding to 
       // counter has only decrypted a number of bytes equal
       // to remainder in the preceding data. therefore that key must 
       // be used to decrypt the first (AES_BYTES_PER_COUNT - remainder)
       // bytes of this data. so we need to first set the previous key
       // and do a fake decrypt to get everything set up properly for this
       // decrypt.
     
       // this is the fake decrypt of remainder bytes.
       memset(nonce,0,AES_KEY_LEN);
       *((uint32_t *)(&nonce[12])) = htonl(counter);
#ifdef HAVE_SRTP
       rc=aes_icm_set_iv(sp->cp->state, nonce);
       rc=aes_icm_encrypt_ismacryp(sp->cp->state, fakedata, &remainder, 1);
#endif

       // now calculate the correct counter for this data 
       counter++;
       remainder = AES_BYTES_PER_COUNT - remainder;
  }

  memset(nonce,0,AES_KEY_LEN);
  *((uint32_t *)(&nonce[12])) = htonl(counter);
#ifdef HAVE_SRTP
  rc=aes_icm_set_iv(sp->cp->state, nonce);
  // set the number of bytes the previous key should decrypt 
  ((aes_icm_ctx_t *)(sp->cp->state))->bytes_in_buffer = remainder;
#endif
  
#ifdef ISMACRYP_DEC_DEBUG 
  fprintf(stdout,"D s: %d      RA BSO: %7d  L: %5d  Ctr: %s  Left: %d\n",
                       sp->sessid, BSO, length,
#ifdef HAVE_SRTP
	  v64_hex_string((v64_t *)&(((aes_icm_ctx_t *)(sp->cp->state))->counter.v64[1])),
	  //"invalid",
	    ((aes_icm_ctx_t *)(sp->cp->state))->bytes_in_buffer
#else
                       "n/a",
                       0
#endif
                       );
#endif  // ISMACRYP_DEC_DEBUG
  
  // length will not be updated in calling function (obviously) awv.
#ifdef HAVE_SRTP
  rc=aes_icm_encrypt_ismacryp(sp->cp->state, data,&length, 1);
#endif

  return ismacryp_rc_ok;
}
