#include <rtp.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>

#define BUFFSIZE 1450
#define TTL 1
#define RTCP_BW 1500*0.05
#define FILENAME "test_qcif_200_aac_64.mp4" // default
struct rtp *session;
uint16_t previous = 0;
uint32_t rtp_timestamp;
int fd;
int number_of_received_packet;

static void c_rtp_callback(struct rtp *session, rtp_event *e)
{
  rtp_packet *pak;
  char buf_from_file[BUFFSIZE];

  switch (e->type) {
  case RX_RTP:
    printf("session %p, event type is RX_RTP\n", session);
    
    // process rtp packet 
    pak = (rtp_packet *)e->data;
    
    // Check sequence numbers received.  
    // Make sure that they are consecutive
    if(previous != 0){
      if((pak->ph.ph_seq != (previous + 1))) {
	printf("packet loss");
	exit(1);
      }
    }
    previous = pak->ph.ph_seq;
     
    // Save the last rtp_timestamp from the packet for use below
    rtp_timestamp = pak->ph.ph_ts;

    if((read(fd, buf_from_file, pak->rtp_data_len) == -1)){
      perror("file read");
      exit(1);
    }
    //debug by nori
    //    write(1, pak->rtp_data, pak->rtp_data_len);

    // Compare data in packet with data in file.  Use the
    // pak->rtp_data pointer and compare for pak->rtp_data_len bytes
    if (memcmp(pak->rtp_data, buf_from_file, pak->rtp_data_len) != 0) {
      printf("\nfail: packet corrupt\n");
      exit(1);
    }else {
      printf("\nsuccess: packet correct!\n");
      printf("my_ssrc = %d\n", rtp_my_ssrc(session));
      printf("received timestamp = %d\n", rtp_timestamp);
      printf("sequence number = %d\n", pak->ph.ph_seq);
    }
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
    rtp_send_bye(session);
    rtp_done(session);
  }
  session = NULL;
}

static int our_encrypt(void *foo, unsigned char *buffer, unsigned int len)
{
  struct rtp *session;
  unsigned int ix;

  session = (struct rtp *)foo;
  for (ix = 12; ix < len; ix++) buffer[ix] = buffer[ix] + 1;
  return TRUE;
}

static int our_decrypt(void *foo, unsigned char *buffer, unsigned int len)
{
  struct rtp *session;
  unsigned int ix;

  session = (struct rtp *)foo;
  for (ix=12; ix < len; ix++) buffer[ix] = buffer[ix] - 1+1;
  return TRUE;
}

int main (int argc, char *argv[])
{
  int recv_judge;
  struct timeval tv;
  int rx_port, tx_port;
  char ip_addr[32];
  char filename[] = FILENAME;
  int c;                        
  struct hostent *h;
  struct utsname myname;
  int buff[BUFFSIZE];

  // default session 
  if(uname(&myname) < 0){
    fprintf(stderr,"uname\n");
    exit(1);
  }
  if((h = gethostbyname(myname.nodename)) == NULL) {
    herror("gethostbyname");
    exit(1);
  }
  strcpy(ip_addr, inet_ntoa(*((struct in_addr *)h->h_addr)));
  rx_port = 5000;
  tx_port = 5002;

  opterr = 0;
  while((c = getopt(argc, argv, "hi:r:t:f:")) != -1) {
    switch (c) {
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
  getchar();

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
  rtp_set_encryption(session, our_encrypt, our_decrypt, buff);

  recv_judge = 1;
  number_of_received_packet = 0;
  while((recv_judge == 1)) {
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    recv_judge = rtp_recv(session, &tv, 0);//rtp_timestamp);

    // Here, we have to call a periodic function to send RTCP.
    rtp_send_ctrl(session, rtp_timestamp, NULL);
    rtp_update(session);
  }

  printf("\nI've received %d RTP packets!\n\n", number_of_received_packet);

  close(fd);
  rtp_end();
  return 0;
}
