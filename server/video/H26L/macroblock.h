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
 * \file
 *    macroblock.h
 *
 * \author
 *    Inge Lille-Langøy               <inge.lille-langoy@telenor.com>     \n
 *    Telenor Satellite Services                                          \n
 *    P.O.Box 6914 St.Olavs plass                                         \n
 *    N-0130 Oslo, Norway
 *
 ************************************************************************/
#ifndef _MACROBLOCK_H_
#define _MACROBLOCK_H_


//! just to make new temp intra mode table
const int  MODTAB[3][2]=
{
  { 0, 4},
  {16,12},
  { 8,20}
};

//! gives codeword number from CBP value, both for intra and inter
const int NCBP[48][2]=
{
  { 3, 0},{29, 2},{30, 3},{17, 7},{31, 4},{18, 8},{37,17},{ 8,13},{32, 5},{38,18},{19, 9},{ 9,14},
  {20,10},{10,15},{11,16},{ 2,11},{16, 1},{33,32},{34,33},{21,36},{35,34},{22,37},{39,44},{ 4,40},
  {36,35},{40,45},{23,38},{ 5,41},{24,39},{ 6,42},{ 7,43},{ 1,19},{41, 6},{42,24},{43,25},{25,20},
  {44,26},{26,21},{46,46},{12,28},{45,27},{47,47},{27,22},{13,29},{28,23},{14,30},{15,31},{ 0,12},
};

/*!
Return prob.(0-5) for the input intra prediction mode(0-5),
depending on previous (right/above) pred mode(0-6).                                \n
                                                                                   \n
NA values are set to 0 in the array.                                               \n
The modes for the neighbour blocks are signalled:                                  \n
0 = outside                                                                        \n
1 = DC                                                                             \n
2 = diagonal vert 22.5 deg                                                         \n
3 = vertical                                                                       \n
4 = diagonal 45 deg                                                                \n
5 = horizontal                                                                     \n
6 = diagonal horizontal -22.5 deg                                                  \n
                                                                                   \n
prob order=PRED_IPRED[B(block left)][A(block above)][intra mode] */
const byte PRED_IPRED[7][7][6]=
{
  {
    {0,0,0,0,0,0},
    {0,0,0,0,1,2},
    {0,0,0,0,1,2},
    {0,0,0,0,1,2},
    {0,0,0,0,1,2},
    {1,0,0,0,0,2},
    {1,0,0,0,2,0},
  },
  {
    {0,2,1,0,0,0},
    {0,2,5,3,1,4},
    {0,1,4,3,2,5},
    {0,1,2,3,4,5},
    {1,3,5,0,2,4},
    {1,4,5,2,0,3},
    {2,4,5,3,1,0},
  },
  {
    {1,0,2,0,0,0},
    {1,0,4,3,2,5},
    {1,0,2,4,3,5},
    {1,0,2,3,4,5},
    {2,1,4,0,3,5},
    {1,2,5,4,0,3},
    {0,1,5,4,3,2},
  },
  {
    {1,2,0,0,0,0},
    {2,4,0,1,3,5},
    {1,3,0,2,4,5},
    {2,1,0,3,4,5},
    {3,2,0,1,5,4},
    {2,5,0,3,1,4},
    {1,2,0,5,3,4},
  },
  {
    {0,1,2,0,0,0},
    {1,4,3,0,2,5},
    {0,3,2,1,4,5},
    {1,3,2,0,4,5},
    {1,4,3,0,2,5},
    {2,4,5,1,0,3},
    {2,4,5,1,3,0},
  },
  {
    {0,1,2,0,0,0},
    {0,3,5,2,1,4},
    {0,2,4,3,1,5},
    {0,3,2,4,1,5},
    {1,4,5,2,0,3},
    {1,4,5,2,0,3},
    {2,4,5,3,0,1},
  },
  {
    {0,1,2,0,0,0},
    {0,4,5,2,1,3},
    {0,1,5,3,2,4},
    {0,1,3,2,4,5},
    {1,4,5,0,3,2},
    {1,4,5,3,0,2},
    {1,3,5,4,2,0},
  }
};

/*!
  return codeword number from two combined intra prediction blocks
*/

const int IPRED_ORDER[6][6]=
{
  { 0, 2, 3, 9,10,20},
  { 1, 4, 8,11,19,21},
  { 5, 7,12,18,22,29},
  { 6,13,17,23,28,30},
  {14,16,24,27,31,34},
  {15,25,26,32,33,35},
};

extern int JQ[32][2];
extern int QP2QUANT[32];

#endif
