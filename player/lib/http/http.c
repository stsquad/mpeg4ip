/*
 *  Http put/get/.. standalone program using Http put mini lib
 *  written by L. Demailly
 *  (c) 1998 Laurent Demailly - http://www.demailly.com/~dl/
 *  (c) 1996 Observatoire de Paris - Meudon - France
 *  see LICENSE for terms, conditions and DISCLAIMER OF ALL WARRANTIES
 *
 * $Id: http.c,v 1.9 2001/09/11 18:33:17 wmaycisco Exp $
 *
 */




#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_lib.h"

int main(int argc,char **argv) 
{
  int  ret,lg,blocksize,r,i;
  char typebuf[70];
  char *data=NULL,*filename=NULL,*proxy=NULL;
  enum {
    ERR,
    DOPUT,
    DOGET,
    DODEL,
    DOHEA
  } todo=ERR;

  if (argc!=3) {
    fprintf(stderr,"usage: http <cmd> <url>\n\tby <L@Demailly.com>\n");
    return 1;
  }
  i=1;
  
  if (!strcasecmp(argv[i],"put")) {
    todo=DOPUT;
  } else if (!strcasecmp(argv[i],"get")) {
    todo=DOGET;
  } else if (!strcasecmp(argv[i],"delete")) {
    todo=DODEL;
  } else if (!strcasecmp(argv[i],"head")) {
    todo=DOHEA;
  }
  if (todo==ERR) {
    fprintf(stderr,
	    "Invalid <cmd> '%s',\nmust be 'put', 'get', 'delete', or 'head'\n",
	    argv[i]);
    return 2;
  }
  i++;
  

  if ((proxy=getenv("http_proxy"))) {
    ret=http_parse_url(proxy,&filename);
    if (ret<0) return ret;
    http_proxy_server=http_server;
    http_server=NULL;
    http_proxy_port=http_port;
  }

  ret=http_parse_url(argv[i],&filename);
  if (ret<0) {if (proxy) free(http_proxy_server); return ret;}

  switch (todo) {
/* *** PUT  *** */
    case DOPUT:
      fprintf(stderr,"reading stdin...\n");
      /* read stdin into memory */
      blocksize=16384;
      lg=0;  
      if (!(data=malloc(blocksize))) {
	return 3;
      }
      while (1) {
	r=read(0,data+lg,blocksize-lg);
	if (r<=0) break;
	lg+=r;
	if ((3*lg/2)>blocksize) {
	  blocksize *= 4;
	  fprintf(stderr,
		  "read to date: %9d bytes, reallocating buffer to %9d\n",
		  lg,blocksize);	
	  if (!(data=realloc(data,blocksize))) {
	    return 4;
	  }
	}
      }
      fprintf(stderr,"read %d bytes\n",lg);
      ret=http_put(filename,data,lg,0,NULL);
      fprintf(stderr,"res=%d\n",ret);
      break;
/* *** GET  *** */
    case DOGET:
      ret=http_get(filename,&data,&lg,typebuf);
      fprintf(stderr,"res=%d,type='%s',lg=%d\n",ret,typebuf,lg);
      fwrite(data,lg,1,stdout);
      break;
/* *** HEAD  *** */
    case DOHEA:
      ret=http_head(filename,&lg,typebuf);
      fprintf(stderr,"res=%d,type='%s',lg=%d\n",ret,typebuf,lg);
      break;
/* *** DELETE  *** */
    case DODEL:
      ret=http_delete(filename);
      fprintf(stderr,"res=%d\n",ret);
      break;
/* impossible... */
    default:
      fprintf(stderr,"impossible todo value=%d\n",todo);
      return 5;
  }
  if (data) free(data);
  free(filename);
  free(http_server);
  if (proxy) free(http_proxy_server);
  
  return ( (ret==201) || (ret==200) ) ? 0 : ret;
}
