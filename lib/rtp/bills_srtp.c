#include "srtp-1.0.2/include/rtp.h"
#include "bills_srtp.h"
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

extern int
srtp_init_from_hex(srtp_ctx_t *ctx, sec_serv_t sec_servs,
		   unsigned char *input_key, uint32_t ssrc);

extern cipher_type_t null_cipher;
extern cipher_type_t rijndael_icm;
extern auth_type_t   null_auth;
extern auth_type_t   tmmhv2;

bills_srtp_t *srtp_init (unsigned int our_ssrc,
			 unsigned int their_ssrc,
			 unsigned char *input_key,
			 int do_encryption,
			 int do_auth)
{
  bills_srtp_t *bptr;
  cipher_type_t *ctype;
  auth_type_t *atype;
  int cipher_key_len = 0;
  int auth_key_len = 0;
  int auth_tag_len = 0;
  err_status_t stat;
  sec_serv_t serv;

  bptr = (bills_srtp_t *)malloc(sizeof(bills_srtp_t));
  if (bptr == NULL) return NULL;
  serv = 0;

  memset(bptr, 0, sizeof(bills_srtp_t));
  if (do_encryption != 0) {
    ctype = &rijndael_icm;
    cipher_key_len = 16;
    serv |= sec_serv_conf;
  } else {
    ctype = &null_cipher;
  }

  if (do_auth != 0) {
    atype = &tmmhv2;
    auth_key_len = 94;
    auth_tag_len = 4;
    serv |= sec_serv_auth;
  } else {
    atype = &null_auth;
    auth_key_len = 0;
    auth_tag_len = 0;
  }

  printf("calling srtp_alloc for transmitter\n");
  stat = srtp_alloc(&bptr->sender_srtp_ctx,
		    ctype,
		    cipher_key_len,
		    atype,
		    auth_key_len,
		    auth_tag_len,
		    serv);
  if (stat) {
    printf("error - srtp alloc failed code %d\n", stat);
    free(bptr);
    return NULL;
  }

  printf("calling srtp_init_from_hex for transmitter\n");
  srtp_init_from_hex(&bptr->sender_srtp_ctx, sec_serv_conf_and_auth,
		     input_key, our_ssrc);

  printf("calling srtp_alloc for receiver\n");
  stat = srtp_alloc(&bptr->receiver_srtp_ctx,
		    ctype,
		    cipher_key_len,
		    atype,
		    auth_key_len,
		    auth_tag_len,
		    serv);
  if (stat) {
    printf("error - srtp alloc failed code %d (2nd pass)\n", stat);
    free(bptr);
    return NULL;
  }

  printf("calling srtp_init_from_hex for receiver\n");
  srtp_init_from_hex(&bptr->receiver_srtp_ctx, sec_serv_conf_and_auth,
		     input_key, their_ssrc);
  return bptr;
}
