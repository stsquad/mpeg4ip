/* $Id: bitpack.cpp,v 1.1 2001/02/05 20:26:21 cahighlander Exp $ */
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

/************************************************************/
/*     Sarnoff Very Low Bit Rate Still Image Coder          */
/*     Copyright 1995, 1996, 1997, 1998 Sarnoff Corporation */
/************************************************************/

/************************************************************/
/*  Filename: bitpack.c                                     */
/*  Author: Bing-Bing CHai                                  */
/*  Date: Dec. 24, 1997                                     */
/*                                                          */
/*  Descriptions:                                           */
/*      This file contains the functions to read and write  */
/*      to files. Most of the functions are the sames as    */ 
/*      that are used in older version of MZTE.             */
/*                                                          */
/*      The following functions have been created or        */
/*      modified.                                           */
/*        Void init_bit_packing_fp(File *fp)                */
/*        Int nextinputbit()                                */
/*        Int get_bit_rate()                                */
/*        Void restore_arithmetic_offset(Int bits_to_go)    */
/************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "basic.hpp"
#include "dataStruct.hpp"
#include "bitpack.hpp"
//#include "PEZW_zerotree.h"
//#include "wvtPEZW.h"
#include "msg.hpp"
#include "errorHandler.hpp"

#define BUFFER_SIZE 1024
#define EXTRABYTES 8  // hjlee 0901
#define MAXLOOKBITS (EXTRABYTES*8)  // hjlee 0901
static Int bytes_in_buffer=0, huff_put_buffer=0, huff_put_bits=0,
       byte_ptr=0,buffer_length=0,totalBitRate=0;
static Int  bit_num = -1;  /* signal to start */
static UInt bit_buf = 0;
static UChar output_buffer[BUFFER_SIZE];
static FILE *bitfile;
static Int byte_count=0;
static Int count, junkCount;
static Int zerocount=0;

/****************************************************************/
/* Intilization of file poInter, must be called prior to using  */
/* any function in this file. By Bing-Bing Chai                 */
/****************************************************************/
Void CVTCCommon::init_bit_packing_fp(FILE *fp, Int clearByte)
{ 
    byte_count=0;
    /*count=0;*/
    bitfile=fp;
    bytes_in_buffer=huff_put_buffer=huff_put_bits=bit_buf=0;

    /* clean byte poInter only when asked for */
    if(clearByte == 0)
       fseek(bitfile,-(buffer_length-byte_ptr+((bit_num+1)/8)),SEEK_CUR);

    buffer_length=byte_ptr=0;
    bit_num=-1;
}


/**********************************/
/*  return total bit rate in bits */
/**********************************/
Int CVTCEncoder::get_total_bit_rate()
{
  return (totalBitRate);
}

Int CVTCEncoder::get_total_bit_rate_dec()
{
  return (count);
}

Int CVTCEncoder::get_total_bit_rate_junk()
{
  return (junkCount);
}


/****************************************************************/
/******************  utilities for bit packing  *****************/
/****************************************************************/

/* put a byte on the bit stream */
#define emit_byte(val)  \
   if (bytes_in_buffer >= BUFFER_SIZE) \
      flush_bytes1(); \
   output_buffer[bytes_in_buffer++] = (UChar) (val)


/* Outputting bytes to the file */
Void CVTCCommon::flush_bytes1()
{
  if (bytes_in_buffer) {  
     fwrite(output_buffer, bytes_in_buffer, sizeof(UChar), bitfile);
  }

  bytes_in_buffer = 0;

}



/* Outputting bytes to the file and count the last few bits, used by main.c */
Void CVTCEncoder::flush_bytes()
{
  if (bytes_in_buffer) {
     fwrite(output_buffer, bytes_in_buffer, sizeof(UChar), bitfile);
     totalBitRate +=8-totalBitRate%8;
  }
  bytes_in_buffer = 0;
}


Void CVTCCommon::emit_bits(UShort code, Int size)
{
   register UInt put_buffer = code;
   register Int    put_bits   = huff_put_bits;
   Int c;

   if (size == 0) {
      return;
   }

   totalBitRate +=size;

   /* Mask off any excess bits in code */
   put_buffer &= (((Int)1) << size) - 1; 

   /* new number of bits in buffer */
   put_bits += size;
  
   /* align incoming bits */
   put_buffer <<= 24 - put_bits;  

   /* and merge with old buffer contents */
   put_buffer |= huff_put_buffer; 
  
   while (put_bits >= 8) {
     c = (Int) ((put_buffer >> 16) & 0xFF);
     emit_byte(c);
     put_buffer <<= 8;
     put_bits    -= 8;

   }
   huff_put_buffer = put_buffer;        /* Update global variables */
   huff_put_bits   = put_bits;
}


/* modified by Jie Liang (Texas Instruments)
   to return the last bits left in the last byte */

Void CVTCEncoder::flush_bits1 ()
{
  Int i;

  if((i=huff_put_bits%8)==0)
    return;

  emit_bits((UShort) 0x7F, 8-i); /* fill any partial byte with ones */
  huff_put_buffer = 0;                   /* and reset bit-buffer to empty */
  huff_put_bits = 0;
}



Void CVTCEncoder::flush_bits ()
{
  Int i=(huff_put_bits%8);
  UShort	usflush;


  usflush = (0x7F >> i);
  emit_bits(usflush, 8-i);  /* fill any partial byte with ones */
  huff_put_buffer = 0;                   /* and reset bit-buffer to empty */
  huff_put_bits = 0;
}

Void CVTCEncoder::flush_bits_zeros ()
{
  Int i;

  if((i=huff_put_bits%8)==0)
    return;

  emit_bits((UShort) 0, 8-i); /* fill any partial byte with ones */
  huff_put_buffer = 0;                   /* and reset bit-buffer to empty */
  huff_put_bits = 0;
}
/*********************************************************/
/* put a parameter Into the file, refer to VTC syntax    */
/* for algorithm                                         */
/*********************************************************/
Int CVTCEncoder::put_param(Int value, Int nbits)
{
   Int extension;
   Int module = 1 << nbits;
   Int mask = (1 << nbits) - 1;
   Int put_bits = 0;
   while (value/module > 0) {
        extension = 1;
        emit_bits( ((value%module) | (extension << nbits)), nbits+1);
        value = value >> nbits;
        put_bits += nbits+1;
   }
   extension = 0;
   emit_bits( ((value & mask) | (extension << nbits)), nbits+1);
   put_bits += nbits+1;
   return (put_bits);

}




/********************************************************/
/***********  Utilities for bit unpacking  **************/
/********************************************************/

/*********************************************************/
/*  Get the next bit from a file                         */
/*  Modified to read in 1000 bytes Into a buffer at a    */
/*  time.                                                */
/*********************************************************/
Int CVTCCommon::nextinputbit()
{
   Int v;

   if (bit_num < 7) {
     if(buffer_length==byte_ptr){
        if((buffer_length=fread(output_buffer,1,BUFFER_SIZE,bitfile)) ==0){
	  /* fprintf(stderr,"out of source ");  */
        output_buffer[0]=0;
        buffer_length++;
      }
      if(buffer_length==BUFFER_SIZE){

        buffer_length -=EXTRABYTES;
        fseek(bitfile,-EXTRABYTES,SEEK_CUR);
      
	  }
      byte_ptr=0;
      byte_count+=buffer_length;
     }
     bit_buf = (bit_buf << 8) + output_buffer[byte_ptr++];
     bit_num += 8;
   }
   v = (bit_buf >> bit_num) & 0x00001;  
   bit_num--;
   count++;

   return v;
}



/* Read nbits bits from a file */
Int CVTCCommon::get_X_bits(Int nbits)
{
   Int v=0;

   while (nbits--)
      v = (v << 1) + nextinputbit();
   return v;
}

/*-------------------------------------------------------------------*/
/*********************************************************/
/* get a parameter Into the file, refer to VTC syntax    */
/* for algorithm                                         */
/*********************************************************/
Int CVTCDecoder::get_param(Int nbits)
{
   Int countg=0;
   Int word=0;
   Int value=0;
   Int module=1<<(nbits);

   do {
     word=get_X_bits(nbits+1);
     value += (word & (module-1))<<(countg*nbits);
     countg++;
   } while (word>>nbits);
   return (value);
}




/******************************************************************/
/* function to adjust the extra bytes read in by arithmetic coder.*/
/* called by ac_decoder_done                                      */
/******************************************************************/
Void CVTCDecoder::restore_arithmetic_offset(Int bits_to_go)
{
  /* for nonalignment, 14 has something to do with Max_frequency */
  bit_num +=14;
  count -= 14;

  if(((Int)(bit_buf>>(bit_num+1)) &1)==0){
    bit_num--;
    count++;
  }
}


/**********************************************************/
/* This function returns the next n bits in the bitstream */
/* where n<=32                                            */
/**********************************************************/
UInt CVTCEncoder::LookBitsFromStream (Int n)
{
  Int tmp_bit_num=bit_num+1, tmp_byte_ptr=byte_ptr;
  unsigned long tmp_buf=bit_buf,v;

  /* test if enough bits */
  /* hjlee 0901 */
  if(n>MAXLOOKBITS)
    errorHandler("LookBitsFromStream() can only return a maximum of "\
                 "%d bits.\n", MAXLOOKBITS);  
  if(buffer_length<BUFFER_SIZE-EXTRABYTES)
    if(((buffer_length-tmp_byte_ptr)<<3)+tmp_bit_num<n)
      /*errorHandler("LookBitsFromStream(): "\
                   "There are not enough bits in the bitstream.\n");*/
      return(0);

  /* get bits */
  while(tmp_bit_num<n/* && tmp_bit_num<(MAXLOOKBITS-8)*/){
    tmp_bit_num +=8;
    tmp_buf = (tmp_buf << 8) + output_buffer[tmp_byte_ptr++];
  }
  v =   (tmp_buf >> (tmp_bit_num-n));
  return(v&1 ); 

}


/* modified by Jie Liang (Texas Instruments)
   to return the last bits left in the last byte */

Int CVTCDecoder::align_byte1()
{
  Int i;
  
  if((i=(bit_num+1)%8)==0)
    return 0;
  
  return(get_X_bits(i)<<(8-i));
}

Int CVTCDecoder::align_byte ()
{
  Int i;
  
  if((i=(bit_num+1)%8)==0)
    i=8;    /* for stuffing as defined in CD 6.2.1 */

  junkCount += i;
  count -= i;

  return(get_X_bits(i)<<(8-i));
}


/* added by Jie Liang (Texas Instruments) */
/* the data needs to by byte aligned first */

Int CVTCDecoder::get_allbits (Char *buffer)
{ 
  Int n,len;
  Int loc=0;

  /* read until end of file */
  do {
    buffer[loc]=get_X_bits(8);
    loc++;
  }while (!feof(bitfile));

  /* read until the data in buffer is finished */
  len = buffer_length-byte_ptr+2;
  for (n=0;n<len;n++)
    {
      buffer[loc]=get_X_bits(8);
      loc++;
    }
    
  return loc;
}


/* added by Jie Liang (Texas Instruments) */
/* check the next four bytes for start code */

Int CVTCDecoder::Is_startcode (long startcode)
{
  long next_4bytes=0;
  Int i;
  Int offset;
  
  if (bit_num>=7)
    offset=1;
  else
    offset=0;

  next_4bytes = output_buffer[byte_ptr-offset];
  for(i=0;i<3;i++)
    next_4bytes = (next_4bytes<<8)+output_buffer[byte_ptr-offset+1+i];

  if(next_4bytes == startcode)
    return 1;
  
  return 0;
}

#define MAXRUNZERO 22  // hjlee
Void CVTCEncoder::emit_bits_checksc(UInt code, Int size)
{
  Int m;
  Int bit;

  for(m=size-1;m>=0;m--){
    bit = (code>>m) & (0x01);
    emit_bits (bit, 1);
    
    /* update zero bit count */
    if (bit)
      zerocount=0;
    else
      zerocount++;

    if (zerocount>=MAXRUNZERO)
      {  /* exceeding maximum runs, add 1 */
        emit_bits (1, 1);
        zerocount=0;

    }
  } /* end of m - bits */

  return;
    
}


Void CVTCEncoder::emit_bits_checksc_init()
{
  zerocount=0;
}


Int CVTCDecoder::get_X_bits_checksc(Int nbits)
{
   Int v=0;
   Int bit;

   while (nbits--){
      if(zerocount==MAXRUNZERO) /*skip next bit */
        {
//          if(!nextinputbit())  ; deleted by swinder
		  nextinputbit(); // added by swinder
//            noteProgress("Possible start code emulation error: should be 1 after MAXZERORUN 0's");
		  zerocount = 0;
        }

      bit = nextinputbit();
      if(!bit)
        zerocount++;
      else
        zerocount =0;

      v = (v << 1) + bit;
   }

   return v;
}

Void CVTCDecoder::get_X_bits_checksc_init()
{
  zerocount=0;
}


Int CVTCDecoder::get_allbits_checksc (unsigned char *buffer)
{ 
  Int n,len;
  Int loc=0;

  /* read until end of file */
  do {
    buffer[loc]=get_X_bits_checksc(8);
    loc++;
  }while (!feof(bitfile));

  /* read until the data in buffer is finished */
  len = buffer_length-byte_ptr+2;
  for (n=0;n<len;n++)
    {
      buffer[loc]=get_X_bits_checksc(8);
      loc++;
    }
    
  return loc;
}

Int CVTCDecoder::align_byte_checksc ()
{
  Int bit;
  Int i;
  Int m;
  Int out=0;
  Int n=0;

  if((i=(bit_num+1)%8)==0)
    return 0;
  
  for(m=0;m<i;m++){
    if(zerocount==MAXRUNZERO){
      get_X_bits(1);
      zerocount=0;
    }
    else{
      bit = get_X_bits(1);
      out = (out<<1) | bit;
      if (bit)
        zerocount=0;
      else
        zerocount++;

      n++;
    }
  }

  return(out<<(8-n));
}


/* This program combine the stuff in bitbuffer Into the actual bitstream */
Void CVTCEncoder::write_to_bitstream(UChar *bitbuffer,Int total_bits)
{
  Int i,bit_stream_length=total_bits>>3, resid=total_bits%8;

  /* write to file assume file has been opened by control program*/
  for(i=0;i<bit_stream_length;i++)
    emit_bits(bitbuffer[i],8);

  if(resid != 0)
    emit_bits(bitbuffer[bit_stream_length]>>(8-resid),resid);
}



/* get the number of bytes already decoded */
int CVTCDecoder::decoded_bytes_from_bitstream ()
{
    long n;

    n = ftell(bitfile);
    return (n+byte_ptr);
}
