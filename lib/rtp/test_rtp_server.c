#include <rtp.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>

#define BUFFSIZE 1450
#define RX 5002
#define TX 5000
#define TTL 1
#define RTCP_BW 1500*0.05
#define FILENAME "test_qcif_200_aac_64.mp4" // default

static void c_rtp_callback(struct rtp *session, rtp_event *e)
{
  e->type = RX_RTP;
  printf("callback : session %p, event type is %d\n", 
	 session, e->type);
  
}

static int our_encrypt(void *foo, unsigned char *buffer, unsigned int len)
{
  struct rtp *session;
  unsigned int ix;

  session = (struct rtp *)foo;
  for (ix = 12; ix < len; ix++) buffer[ix] = buffer[ix] + 1;
  printf("after encrypt %s\n",buffer);  
  return TRUE;
}

static int our_decrypt(void *foo, unsigned char *buffer, unsigned int len)
{
  struct rtp *session;
  unsigned int ix;

  session = (struct rtp *)foo;
  printf("before decrypt %s\n",buffer);
  for (ix=12; ix < len; ix++) buffer[ix] = buffer[ix] - 1;
  return TRUE;
}


int main (int argc, char *argv[])
{
  struct rtp *session;
  int   rx_port, tx_port, fd;
  int   read_size, send_size, packet_size, number_of_packet;
  char  buff[BUFFSIZE];
  char  pt[] = "";
  off_t cur_pos;
  char ip_addr[32];
  char filename[] = FILENAME;
  int c;                        
  struct hostent *h;
  struct utsname myname;
  
  // Setting of default session 
  if(uname(&myname) < 0){
    fprintf(stderr,"uname\n");
    exit(1);
  }
  if((h = gethostbyname(myname.nodename)) == NULL) {
    herror("gethostbyname");
    exit(1);
  }
  strcpy(ip_addr, inet_ntoa(*((struct in_addr *)h->h_addr)));
  rx_port = 5002;
  tx_port = 5000;

  // Option
  opterr = 0;
  while((c = getopt(argc, argv, "hi:r:t:f:")) != -1){
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
  printf("\n-Session Information-\n");
  printf("  ip_addr = %s\n", ip_addr);
  printf("  rx_port = %d\n", rx_port);
  printf("  tx_port = %d\n", tx_port);
  printf("  filename = %s\n", filename);
  printf("Press Return key...");
  getchar();

  // Open Original File to compare with received file
  if((fd = open(filename, O_RDONLY)) == -1){
    perror(filename);
    exit(1);
  }

  session = rtp_init(ip_addr, rx_port, tx_port, TTL, RTCP_BW, 
		     c_rtp_callback, NULL);
  rtp_set_encryption(session, our_encrypt, our_decrypt, buff);

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
				    *pt,//pt
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
    printf("buff_size = %d\n", strlen(buff));
    printf("send_size = %d\n", send_size);
    printf("my_ssrc = %d\n", rtp_my_ssrc(session));
    printf("set timestamp = %ld\n", cur_pos);

    cur_pos += read_size; 
    packet_size++;
    number_of_packet++;

    //rtp_periodic();
    rtp_send_ctrl(session,cur_pos,NULL);
    rtp_update(session);
    // Some sort of sleep here...
    usleep(10 * 1000);
  }
  
  printf("I've sent %d RTP packets!\n\n", number_of_packet);

  close(fd);
  rtp_done(session);
  return 0;
}
