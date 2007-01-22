#include "mpeg4ip.h"
#include <rtp.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>
//#define HAVE_SRTP 1
#ifdef HAVE_SRTP
#include "srtp/srtp.h"
#endif
#include "memory.h"

bool dump_pak = false;

#define BUFFSIZE 2048
#define TTL 1
#define RTCP_BW 1500*0.05
#define FILENAME "default.mp4" // default
#define OUR_SSRC 0x100
#define THEIR_SSRC 0x200
#define EOP_PAD 16

static FILE *rtcp_file = NULL;

struct rtp *session;
#if 0
uint16_t previous = 0;
#endif
uint32_t rtp_timestamp;
int fd;
int number_of_received_packet;
uint32_t number_of_bytes;

static void c_rtp_callback(struct rtp *session, rtp_event *e)
{
  rtp_packet *pak;
  char buf_from_file[BUFFSIZE];

  switch (e->type) {
  case RX_RTP:
    //printf("session %p, event type is RX_RTP\n", session);
    
    // process rtp packet 
    pak = (rtp_packet *)e->data;
    
    // Check sequence numbers received.  
    // Make sure that they are consecutive
#if 0
    if(previous != 0){
      if((pak->ph.ph_seq != (previous + 1))) {
	printf("packet loss");
	exit(1);
      }
    }
    previous = pak->ph.ph_seq;
#endif
     
    // Save the last rtp_timestamp from the packet for use below
    rtp_timestamp = pak->rtp_pak_ts;
    if (number_of_bytes != rtp_timestamp) {
      printf("error - bytes %u ts %u\n", number_of_bytes, rtp_timestamp);
      lseek(fd, rtp_timestamp, SEEK_SET);
      number_of_bytes = rtp_timestamp;
    }

    if((read(fd, buf_from_file, pak->rtp_data_len) == -1)){
      perror("file read");
      exit(1);
    }

    //debug by nori
    //write(1, pak->rtp_data, pak->rtp_data_len);
#if 0
    printf("payload data\n");
    for (ix = 0; ix < pak->rtp_data_len; ix++) {
      printf("%02x ", (unsigned char)pak->rtp_data[ix]);
      if (((ix + 1) % 8) == 0) printf("\n");
    }
    printf("\n");
#endif

#if 0
    printf("file data\n");
    for (ix = 0; ix < pak->rtp_data_len; ix++) {
      printf("%02x ", (unsigned char)buf_from_file[ix]);
      if (((ix + 1) % 8) == 0) printf("\n");
    }
    printf("\n");
#endif

    // Compare data in packet with data in file.  Use the
    // pak->rtp_data pointer and compare for pak->rtp_data_len bytes
    if (memcmp(pak->rtp_data, buf_from_file, pak->rtp_data_len) != 0) {
      printf("fail:received timestamp = %u sequence number = %u size %u\n",
	     rtp_timestamp,
	     pak->rtp_pak_seq,
	     pak->rtp_data_len);
    }else {
      //printf("my_ssrc = %d\n", rtp_my_ssrc(session));
      printf("success:received timestamp = %u sequence number = %u size %u\n",
	     rtp_timestamp,
	     pak->rtp_pak_seq,
	     pak->rtp_data_len);
    }
    number_of_bytes += pak->rtp_data_len;
    number_of_received_packet++;
    xfree(pak);
    break;    

  case RX_SR:
    printf("session %p, event type is RX_SR\n", session);
    break;
  case RX_RR:
    printf("session %p, event type is RX_RR\n", session);
    break;
  case RX_SDES:
    printf("session %p, event type is RX_SDES\n", session);
    break;
  case RX_BYE:
    printf("session %p, event type is RX_BYE\n", session);
    break;
  case SOURCE_CREATED:
    printf("session %p, event type is SOURCE_CREATED\n", session);
    break;
  case SOURCE_DELETED:
    printf("session %p, event type is SOURCE_DELETED\n", session);
    break;
  case RX_RR_EMPTY:
    printf("session %p, event type is RX_RR_EMPTY\n", session);
    break;
  case RX_RTCP_START:
    printf("session %p, event type is RX_RTCP_START\n", session);
    break;
  case RX_RTCP_FINISH:
    printf("session %p, event type is RX_RTCP_FINISH\n", session);
    break;
  case RR_TIMEOUT:
    printf("session %p, event type is RR_TIMEOUT\n", session);
    break;
  case RX_APP:
    printf("session %p, event type is RX_APP\n", session);
    break;
  }
}

static void rtp_end(void)
{
  if (session != NULL) {
    //rtp_send_bye(session);
    rtp_done(session);
  }
  session = NULL;
}


#ifdef HAVE_SRTP
#define ENCRYPT_FUNCTION our_srtp_encrypt
#define DECRYPT_FUNCTION our_srtp_decrypt
#define RTCP_ENCRYPT_FUNCTION our_srtcp_encrypt
#define RTCP_DECRYPT_FUNCTION our_srtcp_decrypt
static int our_srtp_encrypt (void *foo, 
			     unsigned char *buffer, 
			     unsigned int *len)
{
  err_status_t err;
  int retdata;
  srtp_ctx_t *srtp_ctx = (srtp_ctx_t *)foo;
  retdata = *len;

  err = srtp_protect(srtp_ctx,
		     (void *)buffer,
		     &retdata);

  if (err != 0) {
    return FALSE;
  }
  *len = retdata;
  return TRUE;
}     
static int our_srtp_decrypt (void *foo, 
			     unsigned char *buffer, 
			     unsigned int *len)
{
  err_status_t err;
  int retdata;
  srtp_ctx_t *srtp_ctx = (srtp_ctx_t *)foo;

  retdata = *len;
  unsigned int i;

  if (dump_pak) {
    printf("Calling srtp_unprotect, len %d\n", *len);
    for (i = 0; i < *len; i++) {
      printf("%02x ", buffer[i]);
      if (((i + 1) % 12) == 0) printf("\n");
    }
    printf("\n");
  }
  printf("srtp unprotect %p\n", srtp_ctx);
  err = srtp_unprotect(srtp_ctx,
		       (void *)buffer,
		       &retdata);
  if (dump_pak) {
    printf("return from srtp_unprotect %d  length %d\n",err, retdata);
    for (i = 0; i < *len; i++) {
      printf("%02x ", buffer[i]);
      if (((i + 1) % 12) == 0) printf("\n");
    }
    printf("\n");
    printf("exiting srtp_decrypt\n");
  }
  if (err != 0) {
    return FALSE;
  }
  *len = retdata;
  return TRUE;
}     

static int our_srtcp_encrypt (void *foo, 
			     unsigned char *buffer, 
			     unsigned int *len)
{
  err_status_t err;
  int retdata;
  srtp_ctx_t *srtp_ctx = (srtp_ctx_t *)foo;
  retdata = *len;

  err = srtp_protect(srtp_ctx,
		     (void *)buffer,
		     &retdata);

  if (err != 0) {
    return FALSE;
  }
  *len = retdata;
  return TRUE;
}     
static int our_srtcp_decrypt (void *foo, 
			     unsigned char *buffer, 
			     unsigned int *len)
{
  err_status_t err;
  int retdata;
  srtp_ctx_t *srtp_ctx = (srtp_ctx_t *)foo;

  retdata = *len;
  unsigned int i;

  if (dump_pak) {
    printf("Calling srtp_unprotect, len %d\n", *len);
    for (i = 0; i < *len; i++) {
      printf("%02x ", buffer[i]);
      if (((i + 1) % 12) == 0) printf("\n");
    }
    printf("\n");
  }
  err = srtp_unprotect(srtp_ctx,
		       (void *)buffer,
		       &retdata);
  if (dump_pak) {
    printf("return from srtp_unprotect %d  length %d\n",err, retdata);
    for (i = 0; i < *len; i++) {
      printf("%02x ", buffer[i]);
      if (((i + 1) % 12) == 0) printf("\n");
    }
    printf("\n");
    printf("exiting srtp_decrypt\n");
  }
  if (err != 0) {
    return FALSE;
  }
  *len = retdata;
  return TRUE;
}     
#else
#define ENCRYPT_FUNCTION our_encrypt
#define DECRYPT_FUNCTION our_decrypt
#define RTCP_ENCRYPT_FUNCTION our_rtcp_encrypt
#define RTCP_DECRYPT_FUNCTION our_rtcp_decrypt

static int our_encrypt(void *foo, unsigned char *buffer, unsigned int *len)
{
  //  struct rtp *session;
  unsigned int ix;
  unsigned int retdata;
  
  retdata = *len;
  for (ix=sizeof(rtp_packet_header) - 1; ix < retdata; ix++) buffer[ix] = ~buffer[ix];
  return TRUE;
}

static int our_decrypt(void *foo, unsigned char *buffer, unsigned int *len)
{
  //  struct rtp *session;
  unsigned int ix;
  unsigned int retdata;

  retdata = *len;
  for (ix=sizeof(rtp_packet_header) - 1; ix < retdata; ix++) buffer[ix] = ~buffer[ix];
  return TRUE;
}
static int our_rtcp_encrypt(void *foo, unsigned char *buffer, unsigned int *len)
{
  //  struct rtp *session;
  unsigned int ix;
  unsigned int retdata;
  
  retdata = *len;
  for (ix=sizeof(rtp_packet_header) - 1; ix < retdata; ix++) buffer[ix] = ~buffer[ix];
  return TRUE;
}

static int our_rtcp_decrypt(void *foo, unsigned char *buffer, unsigned int *len)
{
  //  struct rtp *session;
  unsigned int ix;
  unsigned int retdata;

  retdata = *len;
  for (ix=sizeof(rtp_packet_header) - 1; ix < retdata; ix++) buffer[ix] = ~buffer[ix];

  if (rtcp_file != NULL) {
    fwrite(buffer, *len, 1, rtcp_file);
  }
  return TRUE;
}
#endif

int main (int argc, char *argv[])
{
  int recv_judge;
  struct timeval tv;
  int rx_port, tx_port;
  char *ip_addr;
  char filename[1024];
  int c;                        
  struct hostent *h;
  struct utsname myname;
  void *srtp_data;

  unsigned int extra_len;
  int do_auth = 1, do_encrypt = 1;
  int do_rtcp = false;

  //int buff[BUFFSIZE];
  //const char passphrase[]="DESnori";
  // default session 
  if(uname(&myname) < 0){
    fprintf(stderr,"uname\n");
    exit(1);
  }
  if((h = gethostbyname(myname.nodename)) == NULL) {
    herror("gethostbyname");
    exit(1);
  }
  ip_addr = strdup(inet_ntoa(*((struct in_addr *)h->h_addr)));
  rx_port = 15000;
  tx_port = 15002;

  opterr = 0;
  while((c = getopt(argc, argv, "acdehi:r:t:f:")) != -1) {
    switch (c) {
    case 'a':
      do_auth = 0;
      break;
    case 'c':
      do_rtcp = 1;
      break;
    case 'd':
      dump_pak = true;
      break;
    case 'e':
      do_encrypt = 0;
      break;
    case 'h':
      printf("Usage: ./test_rtp_client -i <IP_Addr> -r <rx_port> -t <tx_port> -f <filename>\n");
      printf("Default values are as follows...\n");
      printf("ip_addr = %s\n", ip_addr);
      printf("rx_port = %d\n", rx_port);
      printf("tx_port = %d\n", tx_port);
      printf("Filename = %s\n", filename);
      exit(-1);
    case 'i':
      strcpy(ip_addr, optarg);
      break;
    case 'r':
      rx_port = atoi(optarg);
      break;
    case 't':
      tx_port = atoi(optarg);
      break;
    case 'f':
      strcpy(filename,optarg);
      break;
    case '?':
      printf("usage: ./test_rtp_client -i <IP_Addr> -r <rx_port> -t <tx_port> -f <filename>\n");
      exit(1);
    }
  }
  if (optind < argc) {
    for(; optind < argc; optind++) {
      printf("error->%s(#%d): put -[i|r|t|f] \n", argv[optind],optind-1);
    }
    exit(1);
  }

  // display session information
  printf("-Session Information-\n");
  printf("  ip_addr = %s\n", ip_addr);
  printf("  rx_port = %d\n", rx_port);
  printf("  tx_port = %d\n", tx_port);
  printf("  filename = %s\n", filename);
  printf("Press Return key...");
  //getchar();

  if (do_rtcp) {
    rtcp_file = fopen("client.rtcp", FOPEN_WRITE_BINARY);
  }
  
  if((fd = open(filename, O_RDONLY)) == -1){
    perror(filename);
    exit(-1);
  }

  if( (session = rtp_init(ip_addr, rx_port, tx_port, TTL, 
			  RTCP_BW, c_rtp_callback, NULL)) == NULL){
    exit(-1);
  }
  rtp_set_option(session, RTP_OPT_WEAK_VALIDATION, FALSE);
  rtp_set_option(session, RTP_OPT_PROMISC, TRUE);
  rtp_set_my_ssrc(session,OUR_SSRC);

  extra_len = 0;
  srtp_data = NULL;
  //rtp_set_encryption_key(session, passphrase);
  //rtp_set_encryption(session, our_encrypt, our_decrypt, buff);

  /////////// start will
  //set NULL security services
  //uint32_t ssrc = 0xdeadbeef; /* ssrc value hardcoded for now */
#ifdef HAVE_SRTP
  srtp_policy_t policy;
  char key[64];
  char keystr[128];
  unsigned int ix;
#if 1
  strcpy(keystr, "c1eec3717da76195bb878578790af71c4ee9f859e197a414a78d5abc7451");
  hex_string_to_octet_string(key, keystr, 60);
#else
  memset(key, 0, sizeof(key));
#endif

  for (ix = 0; ix < 30; ix++) {
    printf("%02x ", key[ix]);
  }
  printf("\n");
	
#if 0
  //NULL cipher
  policy.key                 =  (uint8_t *) key;
  policy.ssrc.type           = ssrc_any_inbound; //ssrc_specific;
  policy.ssrc.value          =  0x69;//THEIR_SSRC;
  policy.rtp.cipher_type     = NULL_CIPHER;
  policy.rtp.cipher_key_len  = 0; 
  policy.rtp.auth_type       = NULL_AUTH;
  policy.rtp.auth_key_len    = 0;
  policy.rtp.auth_tag_len    = 0;
  policy.rtp.sec_serv        = sec_serv_none;
  policy.rtcp.cipher_type    = NULL_CIPHER;
  policy.rtcp.cipher_key_len = 0; 
  policy.rtcp.auth_type      = NULL_AUTH;
  policy.rtcp.auth_key_len   = 0;
  policy.rtcp.auth_tag_len   = 0;
  policy.rtcp.sec_serv       = sec_serv_none;   
  policy.next                = NULL;
#else
  //confidentiality only, no auth
  crypto_policy_set_aes_cm_128_null_auth(&policy.rtp);
  crypto_policy_set_rtcp_default(&policy.rtcp);
  policy.ssrc.type  = ssrc_any_inbound;
  policy.key  = (uint8_t *) key;
  policy.next = NULL;
  policy.rtp.sec_serv = sec_serv_conf;//sec_servs;
  policy.rtcp.sec_serv = sec_serv_none;  /* we don't do RTCP anyway */	
#endif
  err_status_t status;

  printf("ABOUT TO SRTP_INIT\n");
  status = srtp_init();
  if (status) {
    printf("error: srtp initialization failed with error cde %d\n", status);
    exit(1);
  }
  printf("ABOUT TO SRTP_CREATE\n");
  srtp_ctx_t *srtp_ctx = NULL;
  status = srtp_create(&srtp_ctx, &policy);
  if (status) {
    printf("error: srtp_create() failed with code %d\n", status);
    exit(1);
  }
  printf("DONE WITH SRTP_CREATE\n");
  srtp_data = srtp_ctx;
#endif
  rtp_encryption_params_t params;
  params.rtp_encrypt = ENCRYPT_FUNCTION;
  params.rtcp_encrypt = RTCP_ENCRYPT_FUNCTION;
  params.rtp_decrypt = DECRYPT_FUNCTION;
  params.rtcp_decrypt = RTCP_DECRYPT_FUNCTION;
  params.userdata = srtp_data;
  params.rtp_auth_alloc_extra = params.rtcp_auth_alloc_extra = 0;

  rtp_set_encryption_params(session, &params);

  ////////////// end will

  recv_judge = 1;
  number_of_received_packet = 0;
  number_of_bytes = 0;
  while((recv_judge == 1)) {
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    recv_judge = rtp_recv(session, &tv, rtp_timestamp);

    // Here, we have to call a periodic function to send RTCP.
    //rtp_send_ctrl(session, rtp_timestamp, NULL);
    rtp_update(session);
  }

  printf("\nI've received %d RTP packets!\n\n", number_of_received_packet);

  close(fd);
  if (rtcp_file != NULL) 
    fclose(rtcp_file);
  rtp_end();
  return 0;
}
