#include <rtp.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <srtp.h>
#include <bills_srtp.h>

//#define DUMP_ENCRYPTED_PAK 1

#define BUFFSIZE 1450
#define RX 5002
#define TX 5000
#define TTL 1
#define RTCP_BW 1500*0.05
#define FILENAME "test_qcif_200_aac_64.mp4" // default
#define OUR_SSRC 0x200
#define THEIR_SSRC 0x100
#define EOP_PAD 16

static void c_rtp_callback(struct rtp *session, rtp_event *e)
{
  e->type = RX_RTP;
  printf("callback : session %p, event type is %d\n", 
	 session, e->type);
  
}
#define DUMP_ENCRYPTED_PAK 1
static int our_srtp_encrypt (void *foo, 
			     unsigned char *buffer, 
			     unsigned int *len)
{
  err_status_t err;
  unsigned int retdata;
  //  bills_srtp_t *our_srtp = (bills_srtp_t *)foo;
  bills_srtp_t *our_srtp;
#ifdef DUMP_ENCRYPTED_PAK
  unsigned int i;
#endif
  our_srtp = (bills_srtp_t *)foo;
  printf("rtp encrypt len = %d\n", *len);
  retdata = *len;

  for (i = 0; i < retdata; i++) {
    printf("%02x ", buffer[i]);
    if (((i + 1) % 8) == 0) printf("\n");
  }
  printf("\nprotected value\n");

  err = srtp_protect(&our_srtp->sender_srtp_ctx,
		     (srtp_hdr_t *)buffer,
		     &retdata);

  if (err != 0) {
    printf("return from srtp_protect - value is %d", err);
    return FALSE;
  }
  *len = retdata;
#ifdef DUMP_ENCRYPTED_PAK
  for (i = 0; i < retdata; i++) {
    printf("%02x ", buffer[i]);
    if (((i + 1) % 8) == 0) printf("\n");
  }
  printf("\n");
#endif
  return TRUE;
}     

static int our_srtp_decrypt (void *foo, 
			     unsigned char *buffer, 
			     unsigned int *len)
{
  err_status_t err;
  int retdata;
  bills_srtp_t *our_srtp = (bills_srtp_t *)foo;
  retdata = *len;

  err = srtp_unprotect(&our_srtp->receiver_srtp_ctx,
		     (srtp_hdr_t *)buffer,
		     &retdata);

  if (err != 0) {
    return FALSE;
  }
  *len = retdata;
  return TRUE;
}     
#if 0
static int our_encrypt(void *foo, unsigned char *buffer, unsigned int *len)
{
  struct rtp *session;
  unsigned int ix;
  unsigned int retdata;
  
  retdata = *len;
  session = (struct rtp *)foo;
  for (ix = 12; ix < retdata; ix++) buffer[ix] = buffer[ix] + 1;
  printf("after encrypt %s\n",buffer);  
  return TRUE;
}

static int our_decrypt(void *foo, unsigned char *buffer, unsigned int *len)
{
  struct rtp *session;
  unsigned int ix;
  unsigned int retdata;

  retdata = *len;
  session = (struct rtp *)foo;
  printf("before decrypt %s\n",buffer);
  for (ix=12; ix < retdata; ix++) buffer[ix] = buffer[ix] - 1;
  return TRUE;
}
#endif

int main (int argc, char *argv[])
{
  struct rtp *session;
  int   rx_port, tx_port, fd;
  int   read_size, send_size, packet_size, number_of_packet;
  char  buff[BUFFSIZE];
  off_t cur_pos;
  char *ip_addr;
  char filename[1024];
  int c;                        
  struct hostent *h;
  struct utsname myname;
  bills_srtp_t *srtp_data;
  unsigned int extra_len;
  int do_auth = 1, do_encrypt = 1;

  //  const char passphrase[]="DESnori";
  unsigned char input_key[] = "111111111111111111111111111111111111111111111111111111111111";


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
  rx_port = 5002;
  tx_port = 5000;

  // Option
  opterr = 0;
  while((c = getopt(argc, argv, "aehi:r:t:f:")) != -1){
    switch (c) {
    case 'a':
      do_auth = 0;
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

  if ( (session = rtp_init(ip_addr, rx_port, tx_port, TTL, RTCP_BW, 
			   c_rtp_callback, NULL) ) == NULL){
    exit(-1);
  }
  rtp_set_my_ssrc(session,OUR_SSRC);
  srtp_data = srtp_init(OUR_SSRC,THEIR_SSRC,input_key, do_encrypt, do_auth);
  extra_len = auth_get_tag_length(srtp_data->sender_srtp_ctx.authenticator);
  extra_len += EOP_PAD;
  //tp_set_encryption(session, our_encrypt, our_decrypt, buff);
  rtp_set_encryption(session, our_srtp_encrypt, our_srtp_decrypt, srtp_data, extra_len);
  //  rtp_set_encryption_key(session, passphrase);


  cur_pos = 100;
  packet_size = 64;
  number_of_packet = 0;
  while(1) {
    // change BUFFSIZE to be an incrementing value from 64 to 1450
    if(packet_size > 1450)
      packet_size = 725;
    if((read_size = read(fd, buff, packet_size)) == -1){
      perror("file read");
      exit(1);
    }
    if (read_size == 0) break;
    if (((send_size = rtp_send_data(session,
				    cur_pos,
				    97,//pt
				    0,//m
				    0,//cc 
				    NULL, //csrc[],
				    buff,//data
				    read_size,//data_len
				    NULL,//*extn
				    0,
				    0)
	  ) < 0)) 
      {
	printf("rtp_send_data error\n");
	exit(1);
      }

    printf("\n");
    printf("read_size = %d\n", read_size);
    printf("send_size = %d\n", send_size);
    //printf("my_ssrc = %d\n", rtp_my_ssrc(session));
    printf("set timestamp = %ld\n", cur_pos);

    cur_pos += read_size; 
    packet_size++;
    number_of_packet++;

    //rtp_periodic();
    //rtp_send_ctrl(session,cur_pos,NULL);
    rtp_update(session);
    // Some sort of sleep here...
    usleep(1 * 1000);
  }
  
  printf("I've sent %d RTP packets!\n\n", number_of_packet);

  close(fd);
  rtp_done(session);
  return 0;
}
