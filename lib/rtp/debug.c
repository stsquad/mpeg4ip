/*
 * FILE:    debug.c
 * PROGRAM: RAT
 * AUTHORS: Isidor Kouvelas 
 *          Colin Perkins 
 *          Mark Handley 
 *          Orion Hodson
 *          Jerry Isdale
 * 
 * $Revision: 1.8 $
 * $Date: 2007/09/18 20:52:01 $
 *
 * Copyright (c) 1995-2000 University College London
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Computer Science
 *      Department at University College London
 * 4. Neither the name of the University nor of the Department may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config_unix.h"
#include "config_win32.h"
#include "gettimeofday.h"
#include "debug.h"

void _dprintf(const char *format, ...)
{
#ifdef DEBUG
#ifdef _WIN32
        char msg[65535];
        va_list ap;
        
        va_start(ap, format);
	_vsnprintf(msg, 65535, format, ap);
        va_end(ap);
        OutputDebugString(msg);
#else 
        va_list ap;
 
        va_start(ap, format);
        vfprintf(stderr, format, ap);
        va_end(ap);
#endif /* WIN32 */
#else
        UNUSED (format);
#endif /* DEBUG */
}

/**
 * debug_dump:
 * @lp: pointer to memory region.
 * @len: length of memory region in bytes.
 * 
 * Writes a dump of a memory region to stdout.  The dump contains a
 * hexadecimal and an ascii representation of the memory region.
 * 
 **/
void debug_dump(void*lp, long len)
{
   char * p;
   long i, j, start;
   char Buff[80];
   char stuffBuff[10];
   char tmpBuf[10];
  
   _dprintf("Dump of %ld=%lx bytes\n",len, len);
   start = 0L;
   while (start < len)
   {
    /* start line with pointer position key */
      p = (char*)lp + start;
      sprintf(Buff,"%p: ",p);
  
    /* display each character as hex value */
      for (i=start, j=0; j < 16; p++,i++, j++)
      {
         if (i < len)
         {
            sprintf(tmpBuf,"%X",((int)(*p) & 0xFF));
  
            if (strlen((char *)tmpBuf) < 2)
            {
               stuffBuff[0] = '0';
               stuffBuff[1] = tmpBuf[0];
               stuffBuff[2] = ' ';
               stuffBuff[3] = '\0';
            } else
            {
               stuffBuff[0] = tmpBuf[0];
               stuffBuff[1] = tmpBuf[1];
               stuffBuff[2] = ' ';
               stuffBuff[3] = '\0';
            }
            strcat(Buff, stuffBuff);
         } else
            strcat(Buff," ");
         if (j == 7) /* space between groups of 8 */
            strcat(Buff," ");
      }
  
    /* fill out incomplete lines */
      for(;j<16;j++)
      {
         strcat(Buff,"   ");
         if (j == 7)
            strcat(Buff," ");
      }
      strcat(Buff,"  ");
  
    /* display each character as character value */
      for (i=start,j=0,p=(char*)lp+start;
         (i < len && j < 16); p++,i++, j++)
      {
         if ( ((*p) >= ' ') && ((*p) <= '~') )   /* test displayable */
            sprintf(tmpBuf,"%c", *p);
         else
            sprintf(tmpBuf,"%c", '.');
         strcat(Buff,tmpBuf);
         if (j == 7)   /* space between groups of 8 */
            strcat(Buff," ");
      }
      _dprintf("%s\n", Buff);
      start = i;  /* next line starting byte */
   }
}

/**
 * debug_set_core_dir:
 * @argv0: the application path (usually argv[0] in main()).
 * 
 * Creates a directory with the application name and makes it the
 * current working directory.  
 * 
 * This function exists because some unix variants use the name 'core'
 * for core dump files.  When an application uses multiple processes,
 * this can be problematic if the failure of one process leads to the
 * failure of another because the dependent process 'core' file will
 * overwrite the core of the failing process.
 **/
void debug_set_core_dir(const char *argv0)
{
#if defined(DEBUG) && !defined(_WIN32)
        struct stat s;
        char coredir[64];
        const char *appname;

        appname = strrchr(argv0, '/');
        if (appname == NULL) {
                appname = argv0;
        } else {
                appname = appname + 1;
        }

        /* Should check length of appname, but this is debug code   */
        /* and developers should know better than to have 64 char   */
        /* app name.                                                */
        sprintf(coredir, "core-%s", appname);

        mkdir(coredir, S_IRWXU);
        if (stat(coredir, &s) != 0) {
                debug_msg("Could not stat %s\n", coredir);
                return;
        }
        if (!S_ISDIR(s.st_mode)) {
                debug_msg("Not a directory: %s\n", coredir);
                return;
        }
        if (!(s.st_mode & S_IWUSR) || !(s.st_mode & S_IXUSR)) {
                debug_msg("Cannot write in or change to %s\n", coredir);
                return;
        }
        if (chdir(coredir)) {
                perror(coredir);
        }
#endif /* DEBUG */
        UNUSED(argv0);
}

static int rtp_debug_level =
#ifdef DEBUG
LOG_DEBUG;
#else
LOG_ERR;
#endif

void rtp_set_loglevel (int loglevel)
{
  rtp_debug_level = loglevel;
}
static rtp_error_msg_func_t error_msg_func = NULL;

void rtp_set_error_msg_func (rtp_error_msg_func_t func)
{
  error_msg_func = func;
}
void rtp_message (int loglevel, const char *fmt, ...)
{
  va_list ap;
  if (loglevel <= rtp_debug_level) {
    va_start(ap, fmt);
    if (error_msg_func != NULL) {
      (error_msg_func)(loglevel, "rtp", fmt, ap);
    } else {
 #if _WIN32 && _DEBUG
	  char msg[1024];

      _vsnprintf(msg, 1024, fmt, ap);
      OutputDebugString(msg);
      OutputDebugString("\n");
#else
      struct timeval thistime;
      struct tm thistm;
      char buffer[80];
      time_t secs;

      gettimeofday(&thistime, NULL);
      // To add date, add %a %b %d to strftime
      secs = thistime.tv_sec;
      localtime_r(&secs, &thistm);
      strftime(buffer, sizeof(buffer), "%T", &thistm);
      printf("%s.%03ld-rtp-%d: ",
	     buffer, (unsigned long)thistime.tv_usec / 1000, loglevel);
      vprintf(fmt, ap);
      printf("\n");
#endif
    }
    va_end(ap);
  }
}
