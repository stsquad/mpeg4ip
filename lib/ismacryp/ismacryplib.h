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
#include "mpeg4ip.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define ISMACRYP_SESSION_G_INIT	10
//#define ISMACRYP_DEBUG_TRACE	1



#ifdef __cplusplus
extern "C" {
#endif

/*
--------------------------------------------------------------------------------
General operation of ismacryplib.

The ismacryp library is a new addition to mpeg4ip which implements the ISMA 
encryption and authentication protocol defined in "ISMA 1.0 Encryption and 
Authentication, Version 1.0" specification which is published on the ISMA 
web site, www.isma.tv This specification is referred to as Ismacryp.

This library provides an interface to Ismacryp encryption for use by the various 
other libraries and tools while hiding encryption-specific details from the 
applications which use the library. The intent of this architecture is to 
accomodate future changes to Ismacryp (e.g. new  encryption scheme)  with 
little or no changes required to the applications.
	
Use of the library requires one call to initialize internal data structures 
which are private to the library. Once the library has been initialized, 
Ismacryp sessions can be initiated. A session defines an Ismacryp context 
which mainly consists of an encryption key and a salt for encryption various 
other protocol and state items.

The information that comprises a context is generally not required by the 
application but in cases where context information is required by the application, 
it is obtained by functions provided in this interface.

Applications access their Ismacryp context by means of a session identifier which 
is returned when a session is initiated and which is kept by the application for 
all future calls to the Ismacryp library.

After a session is initiated, applications encrypt or decrypt buffers of data by 
calling the appropriate Ismacryp library routines. When no more encryption or 
decryption services are needed, the application closes the Ismacryp session.  

Multi-threaded applications may use the library with each thread using one or more 
sessions as required.

Call sequence for encryption:
1. ismacrypInitLib                    (once per application)
2. ismacrypInitSession                (once per session, no limit on sessions)
3. ismacrypEncryptSample 
   or ismacrypEncryptSampleAddHeader  (once per each data buffer comprising a track)
4. ismacrypEndSession

Call sequence for decryption:
1. ismacrypInitLib                    (once per application)
2. ismacrypInitSession                (once per session, no limit on sessions)
3. ismacrypDecryptSample 
   or ismacrypDecryptSampleRemoveHeader 
   or ismacrypDecryptSampleRandomAccess (once per each data buffer comprising a track)
4. ismacrypEndSession


Note on Advanced Encryption Standard Integer Counter Mode (AES ICM) which is
used by Ismacryp for encryption. 

AES ICM alters its encryption key after 16 bytes of data have been encrypted. 
As such, the position in a data stream of a buffer of data is necessary 
information for decryption of that data.

The encryption functions provided in this interface require that data for a
track or a stream be encrypted in the exact same order in which it occurs
in the data stream. The postion in the data stream for data in a given session
is kept in the session context. When a buffer of data has been encrypted,
the library assumes that the next buffer of data begins at the point in the
data stream immediately following the previous buffer.

Decryption of tracks and streams may be performed in order if desired. In this
case, the application is responsible for delivering data buffers to the decryption
functions in the exact same order as those buffers were previously encrypted.
Failure to do so will result in improper decryption.

Alternatively an application may decrypt a data buffer from any random location
in the data stream but in this case the start location in the data stream of
the particular buffer must be provided to the appropriate decryption function.

The count of data in a data stream starts at 0 for the first byte of data
and increments by 1 for each byte.
--------------------------------------------------------------------------------
*/



/*
--------------------------------------------------------------------------------
All function calls in the Ismacryp library, save for the library initialization 
function, ismacrypInitLib,  and the session initialization function, 
ismacrypInitSession, require a session identifier of this type. Applications 
acquire a session identifier with the session initialization function, 
ismaCrypInitSession, and supply this identifier in all subsequent library 
function calls.
--------------------------------------------------------------------------------
*/
typedef uint32_t ismacryp_session_id_t; 


/*
--------------------------------------------------------------------------------
All Ismacryp library functions return a code indicating either successful 
completion of the function or failure.
--------------------------------------------------------------------------------
*/
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


/*
--------------------------------------------------------------------------------
There are two key types defined so that audio and video streams/tracks can be
encoded with a different key.
--------------------------------------------------------------------------------
*/
typedef enum  ismacryp_keytype_t { KeyTypeOther=0, 
                            KeyTypeVideo, 
                            KeyTypeAudio } ismacryp_keytype_t;


typedef uint32_t ismacryp_scheme_t;
typedef uint16_t ismacryp_schemeversion_t;

/*
--------------------------------------------------------------------------------
This function is called once by applications prior to using any other functions 
in the library.  It is used to initialize internal data structures in the 
library.  Return code of ismacryp_rc_ok indicates successful completion of this 
function.

Any return code other than ismacryp_rc_ok means that the library cannot 
initialize properly and, as a result, no other library functions will operate 
correctly and they should not be used. Erratic operation or catastrophic failure 
will result if library functions are used after a failure of this library 
initialization routine. It is important in multi-threaded applications that 
this function not be called by more than one thread.
--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypInitLib(void);


/*
--------------------------------------------------------------------------------
This function is used to initialize an Ismacryp session. The session parameter 
is updated with a value for the session and this is to be retained by the 
application for all further calls to the library. Return code of ismacryp_rc_ok 
indicates successful completion of this function. 

Any return code other than ismacryp_rc_ok means that the session was not properly 
initiated and the session identifier will contain a null or invalid value. Use 
of a null session identifier in any other library function will result in an error 
code being returned.
--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypInitSession(ismacryp_session_id_t *session, ismacryp_keytype_t  keytype);


/*
--------------------------------------------------------------------------------
This function is used to deallocate resources associated with a sesssion when 
an application no longer has need for Ismacryp lib services. The session 
parameter is the value initially obtained with ismaCrypInitSession. Return code 
of ismacryp_rc_ok indicates successful completion of this function.

Any return code other than ok means that this function was called with an invalid 
session identifier or a system error of some kind has occurred.
--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypEndSession(ismacryp_session_id_t session);


/*
--------------------------------------------------------------------------------
These functions are used to set and get the Ismacryp scheme (see specification)
for a given session.

The get function updates the scheme parameter with the only currently-defined 
encryption scheme of iAEC. The set function allows for setting of any defined 
Ismacryp schemes. The session parameter in both functions is the value initially 
obtained with ismaCrypInitSession. 

A return code of ismacryp_rc_ok indicates successful completion of these functions.

Any return code other than ismacryp_rc_ok means that these functions were called 
with invalid session identifiers or a system error has occurred. In this case the 
application must assume that scheme was not properly returned or not properly set.
--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypSetScheme(ismacryp_session_id_t session,
                                 ismacryp_scheme_t scheme);
ismacryp_rc_t ismacrypGetScheme(ismacryp_session_id_t session,
                                 ismacryp_scheme_t *scheme);


/*
--------------------------------------------------------------------------------
These functions are used to set and get the Ismacryp scheme version (see 
specification) for a given session.

The get function updates the scheme parameter with the only currently-defined 
encryption scheme version of 1. The set function allows for setting of any defined 
Ismacryp scheme versions. The session parameter in both functions is the value 
initially obtained with ismaCrypInitSession. 

A return code of ismacryp_rc_ok indicates successful completion of these functions.

Any return code other than ismacryp_rc_ok means that these functions were called 
with invalid session identifiers or a system error has occurred. In this case the 
application must assume that scheme version was not properly returned or not 
properly set.
--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypSetSchemeVersion(ismacryp_session_id_t session,
                                        ismacryp_schemeversion_t scheme);
ismacryp_rc_t ismacrypGetSchemeVersion(ismacryp_session_id_t session,
                                        ismacryp_schemeversion_t *scheme);


/*
--------------------------------------------------------------------------------
These functions are used to set and get the Ismacryp KMS URI (see 
specification) for a given session.

The get function updates the kms_uri parameter with the uri of the Key Management 
System. The set function allows for setting of this same parameter. The session 
parameter in both functions is the value initially obtained with ismaCrypInitSession. 

A return code of ismacryp_rc_ok indicates successful completion of these functions.

On success of the get function, kms_uri points at newly allocated memory which the 
calling function is responsible for freeing when it is no longer needed.

Any return code other than ismacryp_rc_ok means that these functions were called 
with invalid session identifiers or a system error has occurred. In this case the 
application must assume that KMS URI was not properly returned or not 
properly set.

NOTE: KMS URI is not supported in this implementation.
--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypSetKMSUri(ismacryp_session_id_t session,
                                 char *kms_uri );
ismacryp_rc_t ismacrypGetKMSUri(ismacryp_session_id_t session,
                                 const char **kms_uri );


/*
--------------------------------------------------------------------------------
These functions are used to set and get the Ismacryp selective encryption
value (see specification) for a given session.

The get function updates the selective_is_on parameter with either TRUE or FALSE 
(1 or 0) indicating whether or not selective encryption is enabled for the session. 
The set function allows for setting this same value. The session parameter in 
both functions is the value initially obtained with ismaCrypInitSession.

A return code of ismacryp_rc_ok indicates successful completion of these functions.

Any return code other than ismacryp_rc_ok means that these functions were called 
with invalid session identifiers or a system error has occurred. In this case the 
application must assume that the selective encryption value was not properly 
returned or not properly set.

NOTE: Selective Encryption is not supported in this implementation.
--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypSetSelectiveEncryption(ismacryp_session_id_t session,
                                              uint8_t selective_is_on );
ismacryp_rc_t ismacrypGetSelectiveEncryption(ismacryp_session_id_t session,
                                              uint8_t *selective_is_on );


/*
--------------------------------------------------------------------------------
These functions are used to set and get the Ismacryp key indicator length
value (see specification) for a given session.

The get function updates the key_indicator_len parameter with the key indicator 
length as defined in the Ismacryp spec. The set function allows for setting this 
same value. The session parameter in both functions is the value initially obtained 
with ismaCrypInitSession.

A return code of ismacryp_rc_ok indicates successful completion of these functions.

Any return code other than ismacryp_rc_ok means that these functions were called 
with invalid session identifiers or a system error has occurred. In this case the 
application must assume that the key indicator length was not properly 
returned or not properly set.

NOTE: Key indicator length pertains to the changing of keys during a session.
This is not supported in this implementation.
--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypSetKeyIndicatorLength(ismacryp_session_id_t session,
                                              uint8_t key_indicator_len);
ismacryp_rc_t ismacrypGetKeyIndicatorLength(ismacryp_session_id_t session,
                                              uint8_t *key_indicator_len);


/*
--------------------------------------------------------------------------------
This functions is used to get the Ismacryp key indicator per au value 
(see specification) for a given session. Value returned is either TRUE or
FALSE (1 or 0).

A return code of ismacryp_rc_ok indicates successful completion of this function.

Any return code other than ismacryp_rc_ok means that this function was called 
with invalid session identifier or a system error has occurred. In this case the 
application must assume that the key indicator per au value was not properly 
returned.

NOTE: Key indicator per au pertains to selective encryption which is not
supported in this implementation. Thus the value returned by this function is
always FALSE (0).
--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypGetKeyIndPerAU(ismacryp_session_id_t session,
                                      uint8_t *key_ind_per_au);


/*
--------------------------------------------------------------------------------
This functions is used to get the Ismacryp key count value (see specification) 
for a given session. 

A return code of ismacryp_rc_ok indicates successful completion of this function.

Any return code other than ismacryp_rc_ok means that this function was called 
with invalid session identifier or a system error has occurred. In this case the 
application must assume that the key count value was not properly 
returned.

NOTE: Key count pertains to the changing of keys during a session.
This is not supported in this implementation. Thus the value returned by this
function is always 1 (i.e. 1 key per session).
--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypGetKeyCount(ismacryp_session_id_t session,
                                   uint8_t *keycount);

/*
--------------------------------------------------------------------------------
These functions are used to set and get the Ismacryp initialization vector length
value (see specification) for a given session.

The get function updates the iv_len parameter with the length of the initialization 
vector as defined in the Ismacryp spec. The set function allows for setting this 
same value. The session parameter in both functions is the value initially obtained 
with ismaCrypInitSession. 

A return code of ismacryp_rc_ok indicates successful completion of these functions.

Any return code other than ismacryp_rc_ok means that these functions were called 
with invalid session identifiers or a system error has occurred. In this case the 
application must assume that the initialization vector length was not properly 
returned or not properly set.
--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypSetIVLength(ismacryp_session_id_t session,
                                   uint8_t iv_len);
ismacryp_rc_t ismacrypGetIVLength(ismacryp_session_id_t session,
                                   uint8_t *iv_len);


/*
--------------------------------------------------------------------------------
These functions are used to set and get the Ismacryp initialization vector delta
length value (see specification) for a given session.

The get function updates the delta_iv_len parameter with the length of the 
initialization vector that is conveyed with an individual AU as defined in the 
Ismacryp spec. The set function allows for setting this same value. The session 
parameter in both functions is the value initially obtained with ismaCrypInitSession.

A return code of ismacryp_rc_ok indicates successful completion of these functions.

Any return code other than ismacryp_rc_ok means that these functions were called 
with invalid session identifiers or a system error has occurred. In this case the 
application must assume that the initialization vector delta length was not properly 
returned or not properly set.
--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypSetDeltaIVLength(ismacryp_session_id_t session,
                                        uint8_t delta_iv_len);
ismacryp_rc_t ismacrypGetDeltaIVLength(ismacryp_session_id_t session,
                                        uint8_t *delta_iv_len);


/*
--------------------------------------------------------------------------------
This function updates several parameters related to the key and the salt for a 
given session. The session parameter is the value initially obtained with 
ismaCrypInitSession. 

A return code of ismacryp_rc_ok indicates successful completion of these functions.

In the case of multiple keys for a given session, the get function must be called 
one time for each key using the value obtained from ismacrypGetKeyCount. 

In each case of a call to this  function, the desired key is indicated in the 
key_num parameter. Key numbers start at 0 in keeping with common C coding conventions. 
For example, if the value returned by ismacrypGetKeyCount is 2, ismacrypGetKey 
would be called twice, once with key_num = 0 and a second time with key_num = 1.

For each key the key_len and salt_len parameters are updated with the length of the 
returned key and the length of the returned salt, respectively. The key and salt 
pointers are updated to point at buffers containing the key and the salt. These 
are newly allocated memory which the calling application is responsible for freeing 
when they are no longer needed.

Finally, the lifetime parameter is updated with the lifetime value for the given key.

Any return code other than ismacryp_rc_ok means that this functions was called 
with invalid session identifier or a system error has occurred. In this case 
the application must assume that the values obtained are incorrect.

--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypGetKey(ismacryp_session_id_t session,
                              uint8_t key_num,
                              uint8_t *key_len,
                              uint8_t *salt_len,
                              uint8_t **key,
                              uint8_t **salt,
                              uint8_t *lifetime_exp);

/*
--------------------------------------------------------------------------------

This API has the same signature as ismacrypGetKey, but instead
of making copies of key and salt in malloc buffers, this API
instead updates key and salt pointers to the live key and salt
buffers.  When using this API, the application must NOT attempt
to free the key and salt pointers!
--------------------------------------------------------------------------------*/

ismacryp_rc_t ismacrypGetKey2(ismacryp_session_id_t session,
                              uint8_t key_num,
                              uint8_t *key_len,
                              uint8_t *salt_len,
                              uint8_t **key,
                              uint8_t **salt,
                              uint8_t *lifetime_exp);

/*
--------------------------------------------------------------------------------

These APIs deallocate and allocate cipher resources for the specified 
session ID.  Used internally and externally, they can be used by application 
to alter cipher on-the-fly.  typical calling sequence is as follows:
1.  retrieve current keying material pointers and attributes
    first established by ismacrypInitSession:
	ismacrypGetKey2() 
2. application updates key and salt using pointers returned
   in step 1.
3. unload current cipher 
	unInitSessionData(...)
4. load new cipher and init crypto context
	initSessionData(...)
--------------------------------------------------------------------------------*/

ismacryp_rc_t unInitSessionData(ismacryp_session_id_t session);
ismacryp_rc_t initSessionData(ismacryp_session_id_t session);



/*
--------------------------------------------------------------------------------
This function is used to encrypt a sample, or buffer of data. The session 
parameter is the value initially obtained with ismaCrypInitSession. The data 
parameter is a pointer to the buffer holding that data that is to be encrypted. 
The length parameter is the length of the data buffer in bytes. The data will be 
encrypted in place. Return code of ismacryp_rc_ok indicates successful completion 
of this function. 

The location in the data stream for this session is updated by this function.
AES ICM requires knowledge of the point in the data stream at which a buffer
of data begins in order to properly operate. This information is kept in the
session state. As such, this function must be called for each buffer of data
in the exact order in which it appears in the data stream

It is critically important that the length parameter be correct. A length value 
shorter than the actual data buffer will result in some of the data not being 
encrypted. A length value longer than the actual data buffer will result in 
unpredictable errors or catastrophic failure.

Any return code other than ismacryp_rc_ok means that the function was called 
with an invalid session identifier or a system error has occurred. In this case 
the application must assume that the data buffer was not properly encrypted.

Data encrypted with this function should be decrypted using 
ismacrypDecryptSample
--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypEncryptSample(ismacryp_session_id_t session,
                                    uint32_t length,
                                    uint8_t *data ); 


/*
--------------------------------------------------------------------------------
This function is used to encrypt a sample, or buffer of data. The session 
parameter is the value initially obtained with ismaCrypInitSession. The data 
parameter is a pointer to the buffer holding that data that is to be encrypted. 
The length parameter is the length of the data buffer in bytes. 

Unlike ismacrypEncryptSample, the data will not be encrypted in place. Rather
a new buffer is allocated and the data is copied to that buffer and encrypted
in the new buffer with Ismacryp protocol headers prepended to the data. 
The new_length parameter is updated with the length of the new_data buffer.
It is the responsibility of the application to free the new_data buffer.

The location in the data stream for this session is updated by this function.
AES ICM requires knowledge of the point in the data stream at which a buffer
of data begins in order to properly operate. This information is kept in the
session state. As such, this function must be called for each buffer of data
in the exact order in which it appears in the data stream

Return code of ismacryp_rc_ok indicates successful completion of this function. 

Any return code other than ismacryp_rc_ok means that the function was called 
with an invalid session identifier or a system error has occurred. In this case 
the application must assume that the data buffer was not properly encrypted.

Data encrypted with this function should be decrypted using 
ismacrypDecryptSampleRemoveHeader
--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypEncryptSampleAddHeader(ismacryp_session_id_t session,
                                    uint32_t length,
                                    uint8_t *data,
                                    uint32_t *new_length,
                                    uint8_t **new_data);

uint32_t ismacrypEncryptSampleAddHeader2 (uint32_t session,
                                          uint32_t length,
                                          uint8_t *data,
                                          uint32_t *new_length,
                                          uint8_t **new_data);

/*
--------------------------------------------------------------------------------
This function is used to decrypt a sample, or buffer of data, that was previously 
encrypted with ismacrypEncryptSample. The session parameter is the value initially 
obtained with ismaCrypInitSession. The data parameter is a pointer to the buffer 
holding that data that is to be encrypted. The length parameter is the length of 
the data buffer in bytes. The data will be decrypted in place. 

Return code of ismacryp_rc_ok indicates successful completion of this function. 

The location in the data stream for this session is updated by this function.
AES ICM requires knowledge of the point in the data stream at which a buffer
of data begins in order to properly operate. This information is kept in the
session state. As such, this function must be called for each buffer of data
in the exact order in which it appears in the data stream.

It is critically important that the length parameter be correct. A length value 
shorter than the actual data buffer will result in some of the data not being 
decrypted. A length value longer than the actual data buffer will result in 
unpredictable errors or catastrophic failure.

Any return code other than ismacryp_rc_ok means that the function was called with 
an invalid session identifier or a system error has occurred. In this case the 
application must assume that the data buffer was not properly decrypted.
--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypDecryptSample(ismacryp_session_id_t session,
                                    uint32_t length,
                                    uint8_t *data);

/*
--------------------------------------------------------------------------------
This function is used to decrypt a sample, or buffer of data, that was previously
encrypted with ismacrypEncryptSampleAddHeader. The session parameter is the value 
initially obtained with ismaCrypInitSession. The data parameter is a pointer to 
the buffer holding that data that is to be decrypted.  The length parameter is 
the length of the data buffer in bytes. 

Unlike ismacrypDecryptSample, the data will not be decrypted in place. Rather
a new buffer is allocated and the data is copied to that buffer and decrypted
in the new buffer with Ismacryp protocol headers removed.  The new_length 
parameter is updated with the length of the new_data buffer.
It is the responsibility of the application to free the new_data buffer.

The location in the data stream for this session is updated by this function.
AES ICM requires knowledge of the point in the data stream at which a buffer
of data begins in order to properly operate. This information is kept in the
session state. As such, this function must be called for each buffer of data
in the exact order in which it appears in the data stream.

Return code of ismacryp_rc_ok indicates successful completion of this function. 

Any return code other than ismacryp_rc_ok means that the function was called 
with an invalid session identifier or a system error has occurred. In this case 
the application must assume that the data buffer was not properly encrypted.

Data encrypted with this function should be decrypted using 
ismacrypDecryptSampleRemoveHeader
--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypDecryptSampleRemoveHeader(ismacryp_session_id_t session,
                                    uint32_t length,
                                    uint8_t *data,
                                    uint32_t *new_length,
                                    uint8_t **new_data);


/*
--------------------------------------------------------------------------------
This function is used to decrypt a sample, or buffer of data, that was previously 
encrypted with ismacrypEncryptSample. The session parameter is the value initially 
obtained with ismaCrypInitSession. The data parameter is a pointer to the buffer 
holding that data that is to be encrypted. The length parameter is the length of 
the data buffer in bytes. The data will be decrypted in place. 

Return code of ismacryp_rc_ok indicates successful completion of this function. 

The location in the data stream for this session is NOT updated by this function.
AES ICM requires knowledge of the point in the data stream at which a buffer
of data begins in order to properly operate. This information is provided to 
the function by the BSO paramater. BSO is Ismacryp speak for Byte Steam Offset.
Counting in a data stream starts at 0 for the first byte and increments by
1 for each subsequent byte.

Unlike the the other encryption and decryption functions where the location in 
the byte stream of data for a session is kept by the session state, this function
allows decryption of data starting from any random point in the data stream.

It is critically important that the length parameter be correct. A length value 
shorter than the actual data buffer will result in some of the data not being 
decrypted. A length value longer than the actual data buffer will result in 
unpredictable errors or catastrophic failure.

Any return code other than ismacryp_rc_ok means that the function was called with 
an invalid session identifier or a system error has occurred. In this case the 
application must assume that the data buffer was not properly decrypted.
--------------------------------------------------------------------------------
*/
ismacryp_rc_t ismacrypDecryptSampleRandomAccess(ismacryp_session_id_t session,
                                    uint32_t BSO,
                                    uint32_t length,
                                    uint8_t *data);

#ifdef __cplusplus
}
#endif
#endif /* ifndef ISMACRYPLIB_H */
