#define HAVE_SRTP 1
#include "mpeg4ip.h"
#include <rtp.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "liblibsrtp.h"

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
    rtp_timestamp = pak->ph.ph_ts;
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
    int status =0;
  printf("ABOUT TO SRTP_INIT\n");

  srtp_if_t* srtp_session = srtp_setup_from_sdp("test", session, "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:ASNFZ4mrze8BI0VniavN7wEjRWeJq83vASNFZ4mr");
  if (status) {
    printf("error: srtp initialization failed with error cde %d\n", status);
    exit(1);
  }

#endif

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
