#include "mpeg4ip.h"
#include <rtp.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#define HAVE_SRTP 1
#ifdef HAVE_SRTP
#include "liblibsrtp.h"
#endif
#include <err.h>


bool dump_pak;

#define BUFFSIZE 1450
#define RX 5002
#define TX 5000
#define TTL 1
#define RTCP_BW 1500*0.05
#define FILENAME "default.mp4" // default
#define OUR_SSRC 0x200
#define THEIR_SSRC 0x100
#define EOP_PAD 16

static FILE *rtcp_file = NULL;

static void c_rtp_callback(struct rtp *session, rtp_event *e)
{
  e->type = RX_RTP;
  printf("callback : session %p, event type is %d\n", 
	 session, e->type);
  
}

#ifdef HAVE_SRTP
#define ENCRYPT_FUNCTION our_srtp_encrypt
#define DECRYPT_FUNCTION our_srtp_decrypt
#define RTCP_ENCRYPT_FUNCTION our_srtp_encrypt
#define RTCP_DECRYPT_FUNCTION our_srtp_decrypt

static int our_srtp_encrypt (void *foo, 
			     unsigned char *buffer, 
			     unsigned int *len)
{
  err_status_t err;
  int retdata;
  srtp_ctx_t *srtp_ctx = (srtp_ctx_t *)foo;
  int i;
	//  our_srtp = (bills_srtp_t *)foo;
  retdata = *len;

  if (dump_pak) {
    printf("rtp encrypt len = %d\n", *len);
    for (i = 0; i < retdata; i++) {
      printf("%02x ", buffer[i]);
      if (((i + 1) % 12) == 0) printf("\n");
    }
    printf("\n");
  }

  err = srtp_protect(srtp_ctx,
		     (void *)buffer,
		     &retdata);

  if (err != 0) {
    printf("return from srtp_protect - value is %d", err);
    return FALSE;
  }
  *len = retdata;
  if (dump_pak) {
    printf("\nprotected value\n");
    for (i = 0; i < retdata; i++) {
      printf("%02x ", buffer[i]);
      if (((i + 1) % 12) == 0) printf("\n");
    }
    printf("\n");
  }
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

  err = srtp_unprotect(srtp_ctx,
		       (void *)buffer,
		       &retdata);

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
  unsigned int ix;
  unsigned int retdata;
  
  retdata = *len;
  //  printf("starting at %u to %u\n", sizeof(rtp_packet_header) - 1, retdata);
  for (ix=sizeof(rtp_packet_header) - 1; ix < retdata; ix++) 
    buffer[ix] = ~buffer[ix];

  return TRUE;
}

static int our_decrypt(void *foo, unsigned char *buffer, unsigned int *len)
{
  struct rtp *session;
  unsigned int ix;
  unsigned int retdata;

  retdata = *len;
  for (ix=sizeof(rtp_packet_header) - 1; ix < retdata; ix++) buffer[ix] = ~buffer[ix];
  return TRUE;
}
static int our_rtcp_encrypt(void *foo, unsigned char *buffer, unsigned int *len)
{
  unsigned int ix;
  unsigned int retdata;

  if (rtcp_file != NULL) {
    fwrite(buffer, *len, 1, rtcp_file);
  }
  retdata = *len;
  //  printf("rtcp starting at %u to %u\n", sizeof(rtp_packet_header) - 1, retdata);
  for (ix=sizeof(rtp_packet_header) - 1; ix < retdata; ix++) 
    buffer[ix] = ~buffer[ix];

  return TRUE;
}

static int our_rtcp_decrypt(void *foo, unsigned char *buffer, unsigned int *len)
{
  struct rtp *session;
  unsigned int ix;
  unsigned int retdata;

  retdata = *len;
  for (ix=sizeof(rtp_packet_header) - 1; ix < retdata; ix++) buffer[ix] = ~buffer[ix];
  return TRUE;
}
#endif

int main (int argc, char *argv[])
{
  struct rtp *session;
  int   rx_port, tx_port, fd;
  uint32_t   read_size, packet_size, number_of_packet;
  uint8_t  buff[BUFFSIZE];
  off_t cur_pos;
  char *ip_addr;
  char filename[1024];
  int c;                        
  struct hostent *h;
  struct utsname myname;
  //  void *srtp_data;
  unsigned int extra_len;
  int do_auth = 1, do_encrypt = 1;
  ssize_t readit;
  ssize_t sendit;
  int do_rtcp = false;

  // Setting of default session 
  if(uname(&myname) < 0){
    fprintf(stderr,"uname\n");
    exit(1);
  }
  if((h = gethostbyname(myname.nodename)) == NULL) {
    herror("gethostbyname");
    exit(1);
  }
  ip_addr = strdup(inet_ntoa(*((struct in_addr *)h->h_addr)));
  rx_port = 15002;
  tx_port = 15000;

  // Option
  opterr = 0;
  while((c = getopt(argc, argv, "acdehi:r:t:f:")) != -1){
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
  printf("\n-Session Information-\n");
  printf("  ip_addr = %s\n", ip_addr);
  printf("  rx_port = %d\n", rx_port);
  printf("  tx_port = %d\n", tx_port);
  printf("  filename = %s\n", filename);
  printf("Press Return key...");
  //  getchar();

  // Open Original File to compare with received file
  if((fd = open(filename, O_RDONLY)) == -1){
    perror(filename);
    exit(1);
  }

  if ( (session = rtp_init_xmitter(ip_addr, rx_port, tx_port, TTL, RTCP_BW, 
				   c_rtp_callback, NULL) ) == NULL){
    exit(-1);
  }
  rtp_set_my_ssrc(session,OUR_SSRC);

#ifdef HAVE_SRTP
	/////////// start will
	//set NULL security services
  //uint32_t ssrc = 0xdeadbeef; /* ssrc value hardcoded for now */
  srtp_policy_t policy;
  char key[64];
  char keystr[128];
  uint ix;
#if 1
  //strcpy(keystr, "c1eec3717da76195bb878578790af71c4ee9f859e197a414a78d5abc7451");
  strcpy(keystr, "0123456789abcdef0123456789abcdef0123456789abcdef0123456789ab");
  hex_string_to_octet_string(key, keystr, 60);
#else
  memset(key, 0, sizeof(key));
#endif

  for (ix = 0; ix < 30; ix++) {
    printf("%02x", ((unsigned char) key[ix]));
  }
  printf("\n");
#if 0
  // NULL cipher
  policy.key                 =  (uint8_t *) key;
  policy.ssrc.type           = ssrc_any_outbound; //ssrc_specific;
  policy.ssrc.value          = 0x96;//OUR_SSRC;
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
  //crypto_policy_set_aes_cm_128_null_auth(&policy.rtp);
  crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtp);
  crypto_policy_set_rtcp_default(&policy.rtcp);
  policy.ssrc.type  = ssrc_any_outbound;
  policy.key  = (uint8_t *) key;
  policy.next = NULL;
  policy.rtp.sec_serv = sec_serv_conf;//sec_servs;
  policy.rtcp.sec_serv = sec_serv_none;  /* we don't do RTCP anyway */
#endif
  err_status_t status;

  printf("ABOUT TO SRTP_INIT\n");
  status = srtp_init();
  if (status) {
    printf("error: srtp initialization failed with error code %d\n", status);
    exit(1);
  }
  printf("ABOUT TO SRTP_CREAT\n");
  srtp_ctx_t *srtp_ctx = NULL;
  status = srtp_create(&srtp_ctx, &policy);
  if (status) {
    printf("error: srtp_create() failed with code %d\n", status);
    exit(1);
  }
  printf("DONE WITH SRTP_CREATE\n");
  extra_len = 0;
#else

  extra_len = 0;
  srtp_data = NULL;
#endif
  rtp_set_encryption(session, ENCRYPT_FUNCTION, DECRYPT_FUNCTION, 
		     RTCP_ENCRYPT_FUNCTION, RTCP_DECRYPT_FUNCTION, 
		     srtp_ctx, extra_len);

  if (do_rtcp) {
    rtcp_file = fopen("server.rtcp", FOPEN_WRITE_BINARY);
  }

  cur_pos = 0;
  packet_size = 64;
  number_of_packet = 0;
  while(1) {
    // change BUFFSIZE to be an incrementing value from 64 to 1450
    if(packet_size > 1450)
      packet_size = 725;
    readit = read(fd, buff, packet_size);
    if (readit < 0) {
      perror("file read");
      exit(1);
    }
    read_size = readit;
    //int buflen = readit;
    if (read_size == 0) break;
    //printf("about to protect\n");
#if 0
    sendit = 
      rtp_send_data(session,
		    cur_pos,
		    97,//pt
		    0,//m
		    0,//cc 
		    NULL, //csrc[],
		    buff,//data
		    read_size,//data_len
		    NULL,//*extn
		    0,
		    0);
#else
    {
      struct iovec iov[2];
      iov[0].iov_len = read_size % 48;
      if (iov[0].iov_len == 0) iov[0].iov_len = 1;
      iov[0].iov_base = buff;
      iov[1].iov_len = read_size - iov[0].iov_len;
      iov[1].iov_base = buff + iov[0].iov_len;

      sendit = rtp_send_data_iov(session, cur_pos, 97, 0, 0, NULL, 
				 iov, read_size > iov[0].iov_len ? 2 : 1,
				 NULL, 0, 0, 0);
    }
#endif
    if (sendit < 0) {
      printf("rtp_send_data error\n");
      exit(1);
    }
    if (do_rtcp)
      rtp_send_ctrl(session, cur_pos, NULL);
    printf("set timestamp = "U64", size %u\n", cur_pos, read_size);
		
    cur_pos += read_size; 
    packet_size++;
    number_of_packet++;
		
    //rtp_periodic();
    //rtp_send_ctrl(session,cur_pos,NULL);
    rtp_update(session);
    // Some sort of sleep here...
    usleep(2 * 1000);
  }
  
  printf("I've sent %d RTP packets!\n\n", number_of_packet);

  close(fd);
  if (rtcp_file != NULL) 
    fclose(rtcp_file);
  rtp_done(session);
  return 0;
}
