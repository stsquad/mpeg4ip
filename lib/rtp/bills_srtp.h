typedef struct bills_srtp_t {
  srtp_ctx_t sender_srtp_ctx;
  srtp_ctx_t receiver_srtp_ctx;
} bills_srtp_t;

bills_srtp_t *srtp_init(unsigned int our_ssrc,
			unsigned int their_ssrc,
			unsigned char *input_key,
			int do_encrypt,
			int do_auth);
