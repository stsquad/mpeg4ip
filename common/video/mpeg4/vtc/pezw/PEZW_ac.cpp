/****************************************************************************/
/*   MPEG4 Visual Texture Coding (VTC) Mode Software                        */
/*                                                                          */
/*   This software was jointly developed by the following participants:     */
/*                                                                          */
/*   Single-quant,  multi-quant and flow control                            */
/*   are provided by  Sarnoff Corporation                                   */
/*     Iraj Sodagar   (iraj@sarnoff.com)                                    */
/*     Hung-Ju Lee    (hjlee@sarnoff.com)                                   */
/*     Paul Hatrack   (hatrack@sarnoff.com)                                 */
/*     Shipeng Li     (shipeng@sarnoff.com)                                 */
/*     Bing-Bing Chai (bchai@sarnoff.com)                                   */
/*     B.S. Srinivas  (bsrinivas@sarnoff.com)                               */
/*                                                                          */
/*   Bi-level is provided by Texas Instruments                              */
/*     Jie Liang      (liang@ti.com)                                        */
/*                                                                          */
/*   Shape Coding is provided by  OKI Electric Industry Co., Ltd.           */
/*     Zhixiong Wu    (sgo@hlabs.oki.co.jp)                                 */
/*     Yoshihiro Ueda (yueda@hlabs.oki.co.jp)                               */
/*     Toshifumi Kanamaru (kanamaru@hlabs.oki.co.jp)                        */
/*                                                                          */
/*   OKI, Sharp, Sarnoff, TI and Microsoft contributed to bitstream         */
/*   exchange and bug fixing.                                               */
/*                                                                          */
/*                                                                          */
/* In the course of development of the MPEG-4 standard, this software       */
/* module is an implementation of a part of one or more MPEG-4 tools as     */
/* specified by the MPEG-4 standard.                                        */
/*                                                                          */
/* The copyright of this software belongs to ISO/IEC. ISO/IEC gives use     */
/* of the MPEG-4 standard free license to use this  software module or      */
/* modifications thereof for hardware or software products claiming         */
/* conformance to the MPEG-4 standard.                                      */
/*                                                                          */
/* Those intending to use this software module in hardware or software      */
/* products are advised that use may infringe existing  patents. The        */
/* original developers of this software module and their companies, the     */
/* subsequent editors and their companies, and ISO/IEC have no liability    */
/* and ISO/IEC have no liability for use of this software module or         */
/* modification thereof in an implementation.                               */
/*                                                                          */
/* Permission is granted to MPEG members to use, copy, modify,              */
/* and distribute the software modules ( or portions thereof )              */
/* for standardization activity within ISO/IEC JTC1/SC29/WG11.              */
/*                                                                          */
/* Copyright 1995, 1996, 1997, 1998 ISO/IEC                                 */
/****************************************************************************/

/****************************************************************************/
/*     Texas Instruments Predictive Embedded Zerotree (PEZW) Image Codec    */
/*	   Developed by Jie Liang (liang@ti.com)                                */
/*													                        */ 
/*     Copyright 1996, 1997, 1998 Texas Instruments	      		            */
/****************************************************************************/

/****************************************************************************
   File name:         PEZW_ac.c
   Author:            Jie Liang  (liang@ti.com)
   Functions:         adaptive arithmetic coding functions
   Revisions:         This file was adopted from public domain
                      arithmetic coder with changes suited to PEZW coder.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "PEZW_ac.hpp"

/* size of memory blocks allocated each time running out of
   buffer for bitstream */
#define MemBlocksize 1000

#ifndef DEBUG_ZTR_DEC_BS
#define DEBUG_ZTR_DEC_BS   0
#endif

#define Code_value_bits 16

#define Top_value (((long)1<<Code_value_bits)-1)
#define First_qtr (Top_value/4+1)
#define Half	  (2*First_qtr)
#define Third_qtr (3*First_qtr)

static void output_bit (Ac_encoder *, int);
static void bit_plus_follow (Ac_encoder *, int);
static int input_bit (Ac_decoder *);
static void update_model (Ac_model *, int);

#define error(m)                                           \
do  {                                                         \
  fflush (stdout);                                            \
  fprintf (stderr, "%s:%d: error: ", __FILE__, __LINE__);     \
  fprintf (stderr, m);                                        \
  fprintf (stderr, "\n");                                     \
  exit (1);                                                   \
}  while (0)

#define check(b,m)                                         \
do  {                                                         \
  if (b)                                                      \
    error (m);                                                \
}  while (0)

static void
output_bit (Ac_encoder *ace, int bit)
{
  ace->buffer <<=1;
  if (bit)
	  ace->buffer |= 0x01; 

  ace->bits_to_go -= 1;
  ace->total_bits += 1;
  if (ace->bits_to_go==0)  {
    if (ace->fp)
      putc (ace->buffer, ace->fp);
    else
      putc_buffer (ace->buffer, &ace->stream, &ace->original_stream,
          &ace->space_left);

    ace->bits_to_go = 8;
	ace->buffer = 0;
  }

  return;
}

static void
bit_plus_follow (Ac_encoder *ace, int bit)
{
  output_bit (ace, bit);
  while (ace->fbits > 0)  {
    output_bit (ace, !bit);
    ace->fbits -= 1;
  }

  return;
}

static int
input_bit (Ac_decoder *acd)
{
  int t;

  if (acd->bits_to_go==0)  {
    if (acd->fp)
	acd->buffer = getc (acd->fp);
    else
	acd->buffer = getc_buffer(&acd->stream);

    acd->bits_to_go = 8;
  }

  t = ((acd->buffer&0x80)>0);
  acd->buffer <<=1; 

  acd->bits_to_go -= 1;

  return t;
}

static void
update_model (Ac_model *acm, int sym)
{
  int i;

  if (acm->cfreq[0]==acm->Max_frequency)  {
    int cum = 0;
    acm->cfreq[acm->nsym] = 0;
    for (i = acm->nsym-1; i>=0; i--)  {
      acm->freq[i] = (acm->freq[i] + 1) / 2;
      cum += acm->freq[i];
      acm->cfreq[i] = cum;
    }
  }

  acm->freq[sym] += 1;
  for (i=sym; i>=0; i--)
    acm->cfreq[i] += 1;

  return;
}

void
Ac_encoder_init (Ac_encoder *ace, unsigned char *fn, 
                 int len, int IsStream)
{
  if (IsStream){
      ace->stream = fn;
      ace->original_stream = fn;
      ace->space_left = len;
      ace->fp = NULL;
  }
  else if (fn)  {
    ace->fp = fopen ((char *)fn, "w");
    check (!ace->fp, "arithmetic encoder could not open file");
  }  
  else  {
    ace->fp = NULL;
  }

  ace->bits_to_go = 8;

  ace->low = 0;
  ace->high = Top_value;
  ace->fbits = 0;
  ace->buffer = 0;

  ace->total_bits = 0;

  return;
}

void
Ac_encoder_done (Ac_encoder *ace)
{
  ace->fbits += 1;
  if (ace->low < First_qtr)
    bit_plus_follow (ace, 0);
  else
    bit_plus_follow (ace, 1);

  if (ace->fp){
    putc (ace->buffer >> ace->bits_to_go, ace->fp);
    fclose (ace->fp);
  }
  else if (ace->bits_to_go <8)
      putc_buffer (ace->buffer << ace->bits_to_go, &ace->stream,
          &ace->original_stream, &ace->space_left);
	

  if (DEBUG_ZTR_DEC_BS)
    fprintf(stdout, "bits to go: %d  last byte: %d\n",
	    ace->bits_to_go, *(ace->stream-1));
      
  return;
}

void
Ac_decoder_open (Ac_decoder *acd, unsigned char *fn, int IsStream)
{
  if (IsStream){
      acd->stream = fn;
      acd->fp = NULL;
  }
  else {
      acd->fp = fopen ((char *)fn, "r");
      check (!acd->fp, "arithmetic decoder could not open file");
  }
 
  return;
}

void
Ac_decoder_init (Ac_decoder *acd, unsigned char *fn)
{
  int i;

  acd->bits_to_go = 0;
  acd->garbage_bits = 0;

  acd->value = 0;
  for (i=1; i<=Code_value_bits; i++)  {
    acd->value = 2*acd->value + input_bit(acd);
  }
  acd->low = 0;
  acd->high = Top_value;

  return;
}

void
Ac_decoder_done (Ac_decoder *acd)
{
  fclose (acd->fp);

  return;
}

void
Ac_model_init (Ac_model *acm, int nsym, int *ifreq, int Max_freq, int adapt)
{
  int i;

  acm->nsym = nsym;

#ifndef MODEL_COUNT_LARGE
      acm->freq = (unsigned char *) (void *) calloc (nsym, sizeof (unsigned char));
#else
      acm->freq = (int *) (void *) calloc (nsym, sizeof (int));
#endif  

  check (!acm->freq, "arithmetic coder model allocation failure");
  acm->cfreq = (int *) (void *) calloc (nsym+1, sizeof (int));
  check (!acm->cfreq, "arithmetic coder model allocation failure");
  acm->Max_frequency = Max_freq;
  acm->adapt = adapt;

  if (ifreq)  {
    acm->cfreq[acm->nsym] = 0;
    for (i=acm->nsym-1; i>=0; i--)  {
      acm->freq[i] = ifreq[i];
      acm->cfreq[i] = acm->cfreq[i+1] + acm->freq[i];
    }


  if (acm->cfreq[0] > acm->Max_frequency)  {
    int cum = 0;
    acm->cfreq[acm->nsym] = 0;
    for (i = acm->nsym-1; i>=0; i--)  {
      acm->freq[i] = (acm->freq[i] + 1) / 2;
      cum += acm->freq[i];
      acm->cfreq[i] = cum;
    }
  }

    if (acm->cfreq[0] > acm->Max_frequency)
      error ("arithmetic coder model max frequency exceeded");
  }  else  {
    for (i=0; i<acm->nsym; i++) {
      acm->freq[i] = 1;
      acm->cfreq[i] = acm->nsym - i;
    }
    acm->cfreq[acm->nsym] = 0;
  }

  return;
}

void
Ac_model_save (Ac_model *acm, int *ifreq)
{
  int i;

  for (i=acm->nsym-1; i>=0; i--)  {
    ifreq[i] = acm->freq[i];
  }

  return;
}

void
Ac_model_done (Ac_model *acm)
{
  acm->nsym = 0;
  free (acm->freq);
  acm->freq = NULL;
  free (acm->cfreq);
  acm->cfreq = NULL;

  return;
}

long
Ac_encoder_bits (Ac_encoder *ace)
{
  return ace->total_bits;
}

void
Ac_encode_symbol (Ac_encoder *ace, Ac_model *acm, int sym)
{
  long range;
 
  check (sym<0||sym>=acm->nsym, "symbol out of range");

#ifdef AC_DEBUG
  printf(" %d ", sym);
#endif

  range = (long)(ace->high-ace->low)+1;
  ace->high = ace->low + (range*acm->cfreq[sym])/acm->cfreq[0]-1;
  ace->low = ace->low + (range*acm->cfreq[sym+1])/acm->cfreq[0];

  for (;;)  {
    if (ace->high<Half)  {
      bit_plus_follow (ace, 0);
    }  else if (ace->low>=Half)  {
      bit_plus_follow (ace, 1);
      ace->low -= Half;
      ace->high -= Half;
    }  else if (ace->low>=First_qtr && ace->high<Third_qtr)  {
      ace->fbits += 1;
      ace->low -= First_qtr;
      ace->high -= First_qtr;
    }  else
      break;
    ace->low = 2*ace->low;
    ace->high = 2*ace->high+1;
  }

  if (acm->adapt)
    update_model (acm, sym);

  return;
}

int
Ac_decode_symbol (Ac_decoder *acd, Ac_model *acm)
{
  long range;
  int cum;
  int sym;

  range = (long)(acd->high-acd->low)+1;
  cum = (((long)(acd->value-acd->low)+1)*acm->cfreq[0]-1)/range;

  for (sym = 0; acm->cfreq[sym+1]>cum; sym++)
    /* do nothing */ ;

  check (sym<0||sym>=acm->nsym, "symbol out of range");

  acd->high = acd->low + (range*acm->cfreq[sym])/acm->cfreq[0]-1;
  acd->low = acd->low +  (range*acm->cfreq[sym+1])/acm->cfreq[0];

  for (;;)  {
    if (acd->high<Half)  {
      /* do nothing */
    }  else if (acd->low>=Half)  {
      acd->value -= Half;
      acd->low -= Half;
      acd->high -= Half;
    }  else if (acd->low>=First_qtr && acd->high<Third_qtr)  {
      acd->value -= First_qtr;
      acd->low -= First_qtr;
      acd->high -= First_qtr;
    }  else
      break;
    acd->low = 2*acd->low;
    acd->high = 2*acd->high+1;
    acd->value = 2*acd->value + input_bit(acd);
  }

  if (acm->adapt)
    update_model (acm, sym);

#ifdef AC_DEBUG
  printf(" %d ", sym);
#endif

  return sym;
}

int getc_buffer (unsigned char **stream)
{
   int output;
   
   output = **stream;
   (*stream)++;

   return(output);
}

void putc_buffer (int x, unsigned char **buffer_curr, 
         unsigned char **buffer_start, int *space_len)
{
    int len;
    unsigned char *temp;

    if (*space_len<=0)
    {
        /* reallocate memory */
        temp = *buffer_start;
        len =  *buffer_curr-*buffer_start;

        *buffer_start = (unsigned char *) calloc(len+MemBlocksize,
            sizeof(char));
        memcpy(*buffer_start, temp, len);

        *buffer_curr = *buffer_start+len;
        *space_len=MemBlocksize;
        free(temp);
    }

    **buffer_curr=x;
	(*buffer_curr)++;
    (*space_len)--;

    return;
}

/* adjust decoder buffer pointer to the right byte 
   at the end of the decoding. Returns the bit location
   for the last bit */
 
int AC_decoder_buffer_adjust (Ac_decoder *acd)
{
	int bits_to_go;


	/* rewind 14 bits for buffer */
	if (DEBUG_ZTR_DEC_BS)
		fprintf(stdout, "bits to go: %d \n",acd->bits_to_go);

	if (acd->bits_to_go>1)
		acd->stream--;
  
	acd->stream--;

	if (acd->bits_to_go>1)
		bits_to_go = acd->bits_to_go-2;
	else
		bits_to_go = acd->bits_to_go+6;

	return (bits_to_go);
}


void AC_free_model (Ac_model *acm)
{
  free(acm->freq);
  free(acm->cfreq);
}
