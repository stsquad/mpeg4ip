#include "mpeg4ip.h"
#include "player_util.h"
#include "player_rtsp.h"

#define TRANSPORT_PARSE(a) static char *transport_parse_## a (char *transport, rtsp_transport_parse_t *r)

TRANSPORT_PARSE(unicast)
{
  ADV_SPACE(transport);
  r->have_unicast = 1;
  if (*transport == '\0') return (transport);

  if (*transport != ';')
    return (NULL);
  transport++;
  ADV_SPACE(transport);
  return (transport);
}

TRANSPORT_PARSE(multicast)
{
  ADV_SPACE(transport);
  r->have_multicast = 1;
  if (*transport == '\0') return (transport);

  if (*transport != ';')
    return (NULL);
  transport++;
  ADV_SPACE(transport);

  return (transport);
}


TRANSPORT_PARSE(client_port)
{
  uint32_t port;
  in_port_t our_port, our_port_max;
  if (*transport++ != '=') {
    return (NULL);
  }
  ADV_SPACE(transport);
  transport = convert_number(transport, &port);
  ADV_SPACE(transport);
  our_port = r->client_port;
  our_port_max = our_port + 1;

  if (port != our_port) {
    message(LOG_ERR, "media", "Returned client port %u doesn't match sent %u",
		  port, our_port);
    return (NULL);
  }
  if (*transport == ';') {
    transport++;
    return (transport);
  }
  if (*transport == '\0') {
    return (transport);
  }
  if (*transport != '-') {
    return (NULL);
  }
  transport++;
  ADV_SPACE(transport);
  transport = convert_number(transport, &port);
  if ((port < our_port) || 
      (port > our_port_max)) {
    message(LOG_ERR, "media", "Illegal client to port %u, range %u to %u",
			 port, our_port, our_port_max);
    return (NULL);
  }
  ADV_SPACE(transport);
  if (*transport == ';') {
    transport++;
  }
  return(transport);
}

TRANSPORT_PARSE(server_port)
{
  uint32_t fromport, toport;

  if (*transport++ != '=') {
    return (NULL);
  }
  ADV_SPACE(transport);
  transport = convert_number(transport, &fromport);
  ADV_SPACE(transport);

  r->server_port = fromport;

  if (*transport == ';') {
    transport++;
    return (transport);
  }
  if (*transport == '\0') {
    return (transport);
  }
  if (*transport != '-') {
    return (NULL);
  }
  transport++;
  ADV_SPACE(transport);
  transport = convert_number(transport, &toport);
  if (toport < fromport || toport > fromport + 1) {
    message(LOG_ERR, "media", "Illegal server to port %u, from is %u",
			 toport, fromport);
    return (NULL);
  }
  ADV_SPACE(transport);
  if (*transport == ';') {
    transport++;
  }
  return(transport);
}

TRANSPORT_PARSE(source)
{
  char *ptr, *newone;
  uint32_t addrlen;

  if (*transport != '=') {
    return (NULL);
  }
  transport++;
  ADV_SPACE(transport);
  ptr = transport;
  while (*transport != ';' && *transport != '\0') transport++;
  addrlen = transport - ptr;
  if (addrlen == 0) {
    return (NULL);
  }
  newone = (char *)malloc(addrlen + 1);
  if (newone == NULL) {
    message(LOG_ERR, "media", "Can't alloc memory for transport source");
    return (NULL);
  }
  strncpy(newone, ptr, addrlen);
  newone[addrlen] = '\0';
  r->source = newone;
  if (*transport == ';') transport++;
  return (transport);
}

TRANSPORT_PARSE(ssrc)
{
  uint32_t ssrc;
  if (*transport != '=') {
    return (NULL);
  }
  transport++;
  ADV_SPACE(transport);
  transport = convert_hex(transport, &ssrc);
  ADV_SPACE(transport);
  if (*transport != '\0') {
    if (*transport != ';') {
      return (NULL);
    }
    transport++;
  }
  r->have_ssrc = 1;
  r->ssrc = ssrc;
  return (transport);
}

TRANSPORT_PARSE(interleave)
{
  uint32_t chan, chan2;
  if (*transport != '=') {
    return (NULL);
  }
  transport++;
  ADV_SPACE(transport);
  transport = convert_number(transport, &chan);
  chan2 = r->interleave_port;
  if (chan != chan2) {
    message(LOG_ERR, "media",
	    "Transport interleave not what was requested %d %d", 
	    chan, chan2);
    return NULL;
  }
  ADV_SPACE(transport);
  if (*transport != '\0') {
    if (*transport != '-') {
      return (NULL);
    }
    transport++;
    transport = convert_number(transport, &chan2);
    if (chan + 1 != chan2) {
      message(LOG_ERR, "media", "Error in transport interleaved field");
      return (NULL);
    }
    
    if (*transport == '\0') return (transport);
  }
  if (*transport != ';') return (NULL);
  transport++;
  return (transport);
}

TRANSPORT_PARSE(destination)
{
  // just skip this
  while (*transport != ';' && *transport != '\0') transport++;
  if (*transport == ';') transport++;
  return transport;

}
#define TTYPE(a,b) {a, sizeof(a), b}

struct {
  const char *name;
  uint32_t namelen;
  char *(*routine)(char *transport, rtsp_transport_parse_t *);
} transport_types[] = 
{
  TTYPE("unicast", transport_parse_unicast),
  TTYPE("multicast", transport_parse_multicast),
  TTYPE("client_port", transport_parse_client_port),
  TTYPE("server_port", transport_parse_server_port),
  TTYPE("port", transport_parse_server_port),
  TTYPE("source", transport_parse_source),
  TTYPE("ssrc", transport_parse_ssrc),
  TTYPE("interleaved", transport_parse_interleave),
  TTYPE("destination", transport_parse_destination),
  {NULL, 0, NULL},
}; 

int process_rtsp_transport (rtsp_transport_parse_t *parse,
			    char *transport,
			    const char *proto)
{
  size_t protolen;
  int ix;

  if (transport == NULL) 
    return (-1);

  protolen = strlen(proto);
  
  if (strncasecmp(transport, proto, protolen) != 0) {
    message(LOG_ERR, "media", "transport %s doesn't match %s", 
	    transport, proto);
    return (-1);
  }
  transport += protolen;
  if (*transport == '/') {
    transport++;
    if (parse->use_interleaved) {
      if (strncasecmp(transport, "TCP", strlen("TCP")) != 0) {
	message(LOG_ERR, "media", "Transport is not TCP");
	return (-1);
      }
      transport += strlen("TCP");
    } else {
      if (strncasecmp(transport, "UDP", strlen("UDP")) != 0) {
	message(LOG_ERR, "media", "Transport is not UDP");
	return (-1);
      }
      transport += strlen("UDP");
    }
  }
  if (*transport != ';') {
    return (-1);
  }
  transport++;
  do {
    ADV_SPACE(transport);
    for (ix = 0; transport_types[ix].name != NULL; ix++) {
      if (strncasecmp(transport, 
		      transport_types[ix].name, 
		      transport_types[ix].namelen - 1) == 0) {
	transport += transport_types[ix].namelen - 1;
	ADV_SPACE(transport);
	transport = (transport_types[ix].routine)(transport, parse);
	break;
      }
    }
    if (transport_types[ix].name == NULL) {
      message(LOG_INFO, "media", 
	      "Illegal mime type in transport - skipping %s", 
	      transport);
      while (*transport != ';' && *transport != '\0') transport++;
      if (*transport != '\0') transport++;
    }
  } while (transport != NULL && *transport != '\0');

  if (transport == NULL) {
    return (-1);
  }
  return (0);
}
