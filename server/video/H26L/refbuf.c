/*
***********************************************************************
* COPYRIGHT AND WARRANTY INFORMATION
*
* Copyright 2001, International Telecommunications Union, Geneva
*
* DISCLAIMER OF WARRANTY
*
* These software programs are available to the user without any
* license fee or royalty on an "as is" basis. The ITU disclaims
* any and all warranties, whether express, implied, or
* statutory, including any implied warranties of merchantability
* or of fitness for a particular purpose.  In no event shall the
* contributor or the ITU be liable for any incidental, punitive, or
* consequential damages of any kind whatsoever arising from the
* use of these programs.
*
* This disclaimer of warranty extends to the user of these programs
* and user's customers, employees, agents, transferees, successors,
* and assigns.
*
* The ITU does not represent or warrant that the programs furnished
* hereunder are free of infringement of any third-party patents.
* Commercial implementations of ITU-T Recommendations, including
* shareware, may be subject to royalty fees to patent holders.
* Information regarding the ITU-T patent policy is available from
* the ITU Web site at http://www.itu.int.
*
* THIS IS NOT A GRANT OF PATENT RIGHTS - SEE THE ITU-T PATENT POLICY.
************************************************************************
*/

/*!
 ************************************************************************
 * \file refbuf.c
 *
 * \brief
 *    Declarations of teh reference frame buffer types and functions
 ************************************************************************
 */

#define HACK


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#include "refbuf.h"

#define CACHELINESIZE 32

#ifdef HACK


/*!
 ************************************************************************
 * \brief
 *    Reference buffer write routines
 ************************************************************************
 */
void PutPel_14 (pel_t **Pic, int y, int x, pel_t val)
{
  Pic [IMG_PAD_SIZE*4+y][IMG_PAD_SIZE*4+x] = val;
}

void PutPel_11 (pel_t *Pic, int y, int x, pel_t val)
{
  Pic [y*img->width+x] = val;
}


/*!
 ************************************************************************
 * \brief
 *    Reference buffer read, Full pel
 ************************************************************************
 */
pel_t FastPelY_11 (pel_t *Pic, int y, int x)
{
  return Pic [y*img->width+x];
}


pel_t *FastLine16Y_11 (pel_t *Pic, int y, int x)
{
  return &Pic [y*img->width+x];
}

pel_t UMVPelY_11 (pel_t *Pic, int y, int x)
{
  if (x < 0)
  {
    if (y < 0)
      return Pic [0];
    if (y >= img->height)
      return Pic [(img->height-1) * img->width];
    return Pic [y*img->width];
  }

  if (x >= img->width)
  {
    if (y < 0)
      return Pic [img->width-1];
    if (y >= img->height)
      return Pic [img->height * img->width -1];
    return Pic [(y+1)*img->width -1 ];
  }

  if (y < 0)    // note: corner pixels were already processed
    return Pic [x];
  if (y >= img->height)
    return Pic [(img->height-1)*img->width+x];

  return Pic [y*img->width+x];
}

/*!
 ************************************************************************
 * \note
 *    The following function is NOT reentrant!  Use a buffer
 *    provided by the caller to change that (but it costs a memcpy()...
 ************************************************************************
 */
static pel_t line[16];
#if 0
pel_t *UMVLine16Y_11 (pel_t *Pic, int y, int x)
{
  int i;

  for (i=0; i<16; i++)
    line[i] = UMVPelY_11 (Pic, y, x+i);
  return line;
}
#else
pel_t *UMVLine16Y_11 (pel_t *Pic, int y, int x)
{
  int i, maxx;
  pel_t *Picy;

  Picy = &Pic [max(0,min(img->height-1,y)) * img->width];

  if (x < 0) {                    // Left edge ?

    maxx = min(0,x+16);
    for (i = x; i < maxx; i++)
      line[i-x] = Picy [0];       // Replicate left edge pixel

    maxx = x+16;
    for (i = 0; i < maxx; i++)    // Copy non-edge pixels
      line[i-x] = Picy [i];
  }
  else if (x > img->width-16)  {  // Right edge ?

    maxx = img->width;
    for (i = x; i < maxx; i++)
      line[i-x] = Picy [i];       // Copy non-edge pixels

    maxx = x+16;
    for (i = max(img->width,x); i < maxx; i++)
      line[i-x] = Picy [img->width-1];  // Replicate right edge pixel
  }
  else                            // No edge
    return &Picy [x];

  return line;
}
#endif


/*!
 ************************************************************************
 * \brief
 *    Reference buffer read, 1/2 pel
 ************************************************************************
 */
pel_t FastPelY_12 (pel_t **Pic, int y, int x)
{
  return Pic [IMG_PAD_SIZE*4+(y<<1)][IMG_PAD_SIZE*4+(x<<1)];
}


pel_t UMVPelY_12 (pel_t **Pic, int y, int x)
{
  return UMVPelY_14 (Pic, y*2, x*2);
}


/*!
 ************************************************************************
 * \brief
 *    Reference buffer, 1/4 pel
 ************************************************************************
 */
pel_t UMVPelY_14 (pel_t **Pic, int y, int x)
{
  int width4  = ((img->width+2*IMG_PAD_SIZE-1)<<2);
  int height4 = ((img->height+2*IMG_PAD_SIZE-1)<<2);

  x = x + IMG_PAD_SIZE*4;
  y = y + IMG_PAD_SIZE*4;

  if (x < 0)
  {
    if (y < 0)
      return Pic [y&3][x&3];
    if (y > height4)
      return Pic [height4+(y&3)][x&3];
    return Pic [y][x&3];
  }

  if (x > width4)
  {
    if (y < 0)
      return Pic [y&3][width4+(x&3)];
    if (y > height4)
      return Pic [height4+(y&3)][width4+(x&3)];
    return Pic [y][width4+(x&3)];
  }

  if (y < 0)    // note: corner pixels were already processed
    return Pic [y&3][x];
  if (y > height4)
    return Pic [height4+(y&3)][x];

  return Pic [y][x];
}

pel_t FastPelY_14 (pel_t **Pic, int y, int x)
{
  return Pic [IMG_PAD_SIZE*4+y][IMG_PAD_SIZE*4+x];
}


/*!
 ************************************************************************
 * \brief
 *    Reference buffer, 1/8th pel
 ************************************************************************
 */
pel_t UMVPelY_18 (pel_t **Pic, int y, int x)
{
  byte out;
  int yfloor, xfloor, xq, yq;

  /* Maximum values (padding included) */
  int maxx8 = (img->width +2*IMG_PAD_SIZE-2)*8;
  int maxy8 = (img->height+2*IMG_PAD_SIZE-2)*8;

  /* Compensate for frame padding */
  x = x + IMG_PAD_SIZE*8;
  y = y + IMG_PAD_SIZE*8;

  if (x < 0)
    x = x&7;
  else if (x > maxx8)
    x = maxx8 + (x&7);

  if (y < 0)
    y = y&7;
  else if (y > maxy8)
    y = maxy8 + (y&7);

  xfloor = x>>1;
  yfloor = y>>1;

  if( (x&1) && (y&1) )
  {
    xq = x&7; 
    yq = y&7;

    // I & IV QUARTER
    if ((xq<4 && yq<4) || (xq>=4 && yq>=4)){
      if (xq==3 && yq==3){
        out=(Pic[yfloor-1][xfloor-1] + 3*Pic[yfloor+1][xfloor+1] + 2) / 4;
      }
      else if (xq==5 && yq==5){
        out=(3*Pic[yfloor][xfloor] + Pic[yfloor+2][xfloor+2] + 2) / 4;
      }
      else if ((xq==3 && yq==1) || (xq==7 && yq==5)){
        out=(3*Pic[yfloor][xfloor+1] + Pic[yfloor+2][xfloor-1] + 2) / 4;
      }
      else if ((xq==1 && yq==3) || (xq==5 && yq==7)){
        out=(3*Pic[yfloor+1][xfloor] + Pic[yfloor-1][xfloor+2] + 2) / 4;
      }
      else {
        out=(Pic[yfloor+1][xfloor] + Pic[yfloor][xfloor+1]) / 2;
      }
    }
     // II & III QUARTER
    else{
      if (xq==5 && yq==3){
        out=(3*Pic[yfloor+1][xfloor] + Pic[yfloor-1][xfloor+2] + 2) / 4;
      }
      else if (xq==3 && yq==5){
        out=(3*Pic[yfloor][xfloor+1]+Pic[yfloor+2][xfloor-1] + 2) / 4;
      }
      else if ((xq==1 && yq==5) || (xq==5 && yq==1)){
        out=(3*Pic[yfloor][xfloor]+Pic[yfloor+2][xfloor+2] + 2) / 4;
      }
      else if ((xq==3 && yq==7) || (xq==7 && yq==3)){
        out=(3*Pic[yfloor+1][xfloor+1]+Pic[yfloor-1][xfloor-1] + 2) / 4;
      }
      else{
        out=( Pic[yfloor][xfloor] + Pic[yfloor+1][xfloor+1] ) / 2;
      }
    }
  }
  else if (x&1)
  {
    out=( Pic[yfloor  ][xfloor  ] + Pic[yfloor  ][xfloor+1] ) / 2;
  }
  else if (y&1)
  {
    out=( Pic[yfloor  ][xfloor  ] + Pic[yfloor+1][xfloor  ] ) / 2;
  }
  else
    out=  Pic[yfloor  ][xfloor  ];

  return(out);
}

pel_t FastPelY_18 (pel_t **Pic, int y, int x)
{
  byte out;
  int yfloor, xfloor, xq, yq;

  /* Compensate for frame padding */
  x = x + IMG_PAD_SIZE*8;
  y = y + IMG_PAD_SIZE*8;

  xfloor = x>>1;
  yfloor = y>>1;

  if( (x&1) && (y&1) )
  {
    xq = x&7; 
    yq = y&7;

    // I & IV QUARTER
    if ((xq<4 && yq<4) || (xq>=4 && yq>=4)){
      if (xq==3 && yq==3){
        out=(Pic[yfloor-1][xfloor-1] + 3*Pic[yfloor+1][xfloor+1] + 2) / 4;
      }
      else if (xq==5 && yq==5){
        out=(3*Pic[yfloor][xfloor] + Pic[yfloor+2][xfloor+2] + 2) / 4;
      }
      else if ((xq==3 && yq==1) || (xq==7 && yq==5)){
        out=(3*Pic[yfloor][xfloor+1] + Pic[yfloor+2][xfloor-1] + 2) / 4;
      }
      else if ((xq==1 && yq==3) || (xq==5 && yq==7)){
        out=(3*Pic[yfloor+1][xfloor] + Pic[yfloor-1][xfloor+2] + 2) / 4;
      }
      else {
        out=(Pic[yfloor+1][xfloor] + Pic[yfloor][xfloor+1]) / 2;
      }
    }
     // II & III QUARTER
    else{
      if (xq==5 && yq==3){
        out=(3*Pic[yfloor+1][xfloor] + Pic[yfloor-1][xfloor+2] + 2) / 4;
      }
      else if (xq==3 && yq==5){
        out=(3*Pic[yfloor][xfloor+1]+Pic[yfloor+2][xfloor-1] + 2) / 4;
      }
      else if ((xq==1 && yq==5) || (xq==5 && yq==1)){
        out=(3*Pic[yfloor][xfloor]+Pic[yfloor+2][xfloor+2] + 2) / 4;
      }
      else if ((xq==3 && yq==7) || (xq==7 && yq==3)){
        out=(3*Pic[yfloor+1][xfloor+1]+Pic[yfloor-1][xfloor-1] + 2) / 4;
      }
      else{
        out=( Pic[yfloor][xfloor] + Pic[yfloor+1][xfloor+1] ) / 2;
      }
    }
  }
  else if (x&1)
  {
    out=( Pic[yfloor  ][xfloor  ] + Pic[yfloor  ][xfloor+1] ) / 2;
  }
  else if (y&1)
  {
    out=( Pic[yfloor  ][xfloor  ] + Pic[yfloor+1][xfloor  ] ) / 2;
  }
  else
    out=  Pic[yfloor  ][xfloor  ];

  return(out);
}


void InitRefbuf ()
{
  int width  = img->width;
  int height = img->height;
  int num_frames = img->buf_cycle;
  int i;

  if (NULL == (Refbuf11_P = malloc ((width * height + 4711) * sizeof (pel_t))))
    no_mem_exit ("InitRefbuf: Refbuf11_P");
  if (NULL == (Refbuf11 = malloc (num_frames * sizeof (pel_t *))))
    no_mem_exit ("InitRefbuf: Refbuf11");
  for (i=0; i<num_frames; i++)
    if (NULL == (Refbuf11[i] = malloc ((width * height + 4711) * sizeof (pel_t))))
      no_mem_exit ("InitRefbuf: Refbuf11");
}


/*!
 ************************************************************************
 * \brief
 *    Substitutes function oneforthpix_2. It should be worked
 *    out how this copy procedure can be avoided.
 ************************************************************************
 */
void copy2mref()
{
  int j, uv;

  img->frame_cycle=img->number % img->buf_cycle;  /*GH img->no_multpred used insteadof MAX_MULT_PRED
                                                    frame buffer size = img->no_multpred+1*/
  // Luma
  for (j=0; j < (img->height+2*IMG_PAD_SIZE)*4; j++)
    memcpy(mref[img->frame_cycle][j],mref_P[j], (img->width+2*IMG_PAD_SIZE)*4);


  //  Chroma:
  for (uv=0; uv < 2; uv++)
    for (j=0; j < img->height_cr; j++)
        memcpy(mcef[img->frame_cycle][uv][j],mcef_P[uv][j],img->width_cr);

  // Full pel represnetation for MV search

  memcpy (Refbuf11[img->frame_cycle], Refbuf11_P, (img->width*img->height));
}

#endif


#ifndef HACK
// Alloc and free for reference buffers

refpic_t *AllocRefPic (int Id,
            int NumCols,
            int NumRows,
            int MaxMotionVectorX,   // MV Size may be used to allocate additional
            int MaxMotionVectorY)   // memory around boundaries fro UMV search
{
  refpic_t *pic;
  int xs, ys;

  if (NULL == (pic = malloc (sizeof (refpic_t))))
    no_mem_exit("AllocRefPic: pic");

  if (NumCols %2 != 0)
    error ("AllocRefPic: Number of columns must be divisible by two, exiting",600);
  if (NumRows %2 != 0)
    error ("AllocRefPic: Number of rows must be divisible by two, exiting",600);

  pic->Id = Id;
  xs = NumCols + MaxMotionVectorX + MaxMotionVectorX;
  if (xs % (CACHELINESIZE/sizeof (pel_t)) != 0)
    xs = ((xs / (CACHELINESIZE/sizeof (pel_t)))+1) * (CACHELINESIZE/sizeof (pel_t));
  ys = NumRows + MaxMotionVectorY + MaxMotionVectorY;
  pic->x_ysize = xs * 4;
  pic->y_ysize = (NumRows + MaxMotionVectorY + MaxMotionVectorY) * 4;

  pic->x_yfirst = MaxMotionVectorX * 4;
  pic->x_ylast  = (NumCols + MaxMotionVectorX) * 4;
  pic->y_yfirst = MaxMotionVectorY * 4;
  pic->x_ylast  = (NumRows + MaxMotionVectorY) * 4;

  pic->x_uvsize = xs/2;
  pic->y_uvsize = ys/2;

  pic->x_uvfirst  = MaxMotionVectorX/2;
  pic->x_uvlast = (NumCols + MaxMotionVectorX)/2;
  pic->y_uvfirst  = MaxMotionVectorY/2;
  pic->y_uvlast = (NumRows + MaxMotionVectorY)/2;

  if (NULL == (pic->y = malloc (16 * sizeof (pel_t)* xs * ys)))
    no_mem_exit ("AllocRefPic: pic->y");
  if (NULL == (pic->u = malloc (sizeof (pel_t) * (xs/2) * (ys/2))))
    no_mem_exit ("AllocRefPic: pic->u");
  if (NULL == (pic->v = malloc (sizeof (pel_t) * (xs/2) * (ys/2))))
    no_mem_exit ("AllocRefPic: pic->v");

  return pic;
}


int FreeRefPic (refpic_t *Pic)
{
  if (Pic == NULL)
    return 0;
  if (Pic->y != NULL)
    free (Pic->y);
  if (Pic->u != NULL)
    free (Pic->u);
  if (Pic->v != NULL)
    free (Pic->v);
  free (Pic);
  return (0);
}



// Access functions for full pel (1/1 pel)

pel_t PelY_11 (refpic_t *Pic, int x, int y)
{
  register int pos;

  pos = x<<2+Pic->x_yfirst;   // Y Structures are 1/4 pel, hence *4
  pos += (Pic->x_ysize * (Pic->y_yfirst + y<<2));

  return Pic->y[pos];
}

pel_t PelU_11 (refpic_t *Pic, int x, int y)
{
  register int pos;

  pos = x+Pic->x_uvfirst;   // UV Structures are 1/1 pel
  pos += Pic->x_uvsize * (Pic->y_uvfirst + y);

  return Pic->u[pos];
}

pel_t PelV_11 (refpic_t *Pic, int x, int y)
{
  register int pos;

  pos = x+Pic->x_uvfirst;   // UV Structures are 1/1 pel
  pos += Pic->x_uvsize * (Pic->y_uvfirst + y);

  return Pic->v[pos];
}


pel_t *MBLineY_11 (refpic_t *Pic, int x, int y)
{
  error ("MBLineY_11: net yet implemented.", 600);
}


// Access functions for half pel (1/2 pel)

pel_t PelY_12 (refpic_t *Pic, int x, int y)
{
  register int pos;

  pos = (x<<1+Pic->x_yfirst);   // Structures are 1/4 pel, hence *4
  pos += (Pic->x_ysize * (Pic->y_yfirst + y<<1));

  return (Pic->v[pos]);
};


// Access functions for quater pel (1/4 pel)

pel_t PelY_14 (refpic_t *Pic, int x, int y)
{
  return (Pic->y[ (Pic->y_yfirst+y)*Pic->x_ysize  + x + Pic->x_yfirst]);
}


// Access functions for one-eigths pel (1/8 pel)

pel_t PelY_18 (refpic_t *Pic, int x, int y)
{
  error ("PelY_18: No 1/8th pel support yet", 600);
}

#endif

