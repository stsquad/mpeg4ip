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
 *************************************************************************************
 * \file header.c
 *
 * \brief
 *    H.26L Slice, Picture, and Sequence headers
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *      - Stephan Wenger                  <stewe@cs.tu-berlin.de>
 *************************************************************************************
 */

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "elements.h"
#include "header.h"
#include "rtp.h"


// A little trick to avoid those horrible #if TRACE all over the source code
#if TRACE
#define SYMTRACESTRING(s) strncpy(sym.tracestring,s,TRACESTRING_SIZE)
#else
#define SYMTRACESTRING(s) // to nothing
#endif

// Local Functions
static int PutSliceStartCode(Bitstream *s);
static int PutPictureStartCode (Bitstream *s);
static int PutStartCode (int Type, Bitstream *s, char *ts);

// End local Functions


int PictureHeader()
{

// StW: PictureTypeSymbol is the Symbol used to encode the picture type.
// This is one of the dirtiest hacks if have seen so far from people who
// usually know what they are doing :-)
// img->type has one of three possible values (I, P, B), the picture type as coded
// distiguishes further by indicating whether one or more reference frames are
// used for P and B frames (hence 5 values).  See decoder/defines.h
// The mapping is performed in image.c select_picture_type()

  int dP_nr = assignSE2partition[input->partition_mode][SE_HEADER];
  Bitstream *currStream = ((img->currentSlice)->partArr[dP_nr]).bitstream;
  DataPartition *partition = &((img->currentSlice)->partArr[dP_nr]);
  SyntaxElement sym;
  int len = 0;

  sym.type = SE_HEADER;       // This will be true for all symbols generated here
  sym.mapping = n_linfo2;       // Mapping rule: Simple code number to len/info

  // Ok.  We are sure we want to code a Picture Header.  So, first: Put PSC
  len+=PutPictureStartCode (currStream);

  // Now, take care of te UVLC coded data structure described in VCEG-M79
  sym.value1 = 0;               // TRType = 0
  SYMTRACESTRING("PH TemporalReferenceType");
  len += writeSyntaxElement_UVLC (&sym, partition);

  sym.value1 = img->tr%256;         // TR, variable length
  SYMTRACESTRING("PH TemporalReference");
  len += writeSyntaxElement_UVLC (&sym, partition);

  // Size information.  If the picture is Intra, then transmit the size in MBs,
  // For all other picture type just indicate that it didn't change.
  // Note: this is currently the prudent way to do things, because we do not
  // have anything similar to Annex P of H.263.  However, we should anticipate
  // taht one day we may want to have an Annex P type functionality.  Hence, it is
  // unwise to assume that P pictures will never have a size indiciation.  So the
  // one bit for an "unchanged" inidication is well spent.

  if (img->type == INTRA_IMG)
  {
    // Double check that width and height are divisible by 16
    assert (img->width  % 16 == 0);
    assert (img->height % 16 == 0);

    sym.value1 = 1;             // SizeType = Width/Height in MBs
    SYMTRACESTRING("PH FullSizeInformation");
    len += writeSyntaxElement_UVLC (&sym, partition);
    sym.value1 = img->width / 16;
    SYMTRACESTRING("PH FullSize-X");
    len += writeSyntaxElement_UVLC (&sym, partition);
    sym.value1 = img->height / 16;
    SYMTRACESTRING("PH FullSize-Y");
    len += writeSyntaxElement_UVLC (&sym, partition);

  } else
  {    // Not an intra frame -> write "unchanged"
    sym.value1 = 0;             // SizeType = Unchanged
    SYMTRACESTRING("PHSizeUnchanged");
    len += writeSyntaxElement_UVLC (&sym, partition);
  }

  select_picture_type (&sym);
  SYMTRACESTRING("Hacked Picture Type Symbol");
  len+= writeSyntaxElement_UVLC (&sym, partition);

  // Finally, write Reference Picture ID 
  // WYK: Oct. 16, 2001. Now I use this for the reference frame ID (non-B frame ID). 
  // If current frame is a B-frame, it will not change; otherwise it is increased by 1 based
  // on refPicID of the previous frame. Thus, the decoder can know how many  non-B 
  // frames are lost, and then can adjust the reference frame buffers correctly.
  if (1)
  {
    sym.value1 = img->refPicID%16;         // refPicID, variable length
    SYMTRACESTRING("PHRefPicID");
    len += writeSyntaxElement_UVLC (&sym, partition);
  }

  return len;
}


int SliceHeader(int UseStartCode)
{
  int dP_nr = assignSE2partition[input->partition_mode][SE_HEADER];
  Bitstream *currStream = ((img->currentSlice)->partArr[dP_nr]).bitstream;
  DataPartition *partition = &((img->currentSlice)->partArr[dP_nr]);
  SyntaxElement sym;
  int len = 0;
  int Quant = img->qp;          // Hack.  This should be a parameter as soon as Gisle is done
  int MBPosition = img->current_mb_nr;  // Hack.  This should be a paremeter as well.

  sym.type = SE_HEADER;       // This will be true for all symbols generated here
  sym.mapping = n_linfo2;       // Mapping rule: Simple code number to len/info

  // Note 1.  Current implementation: Slice start code is similar to pictrure start code
  // (info is 1 for slice and 0 for picture start code).  Slice Start code is not
  // present except when generating an UVLC file.  Start codes are typical NAL
  // functionality, and hence NALs will take care the issue in the future.
  // Note 2:  Traditionally, the QP is a bit mask.  However, at numerically large QPs
  // we usually have high compression and don't want to waste bits, whereas
  // at low QPs this is not as much an issue.  Hence, the QUANT parameter
  // is coded as a UVLC calculated as 31 - QUANT.  That is, the UVLC representation
  // of 31-QUANT is coded, instead of QUANT.
  // Note 3:  In addition to the fields in VCEG-M79 there is one additional header field
  // with the MV resolution.  Currently defined values:
  // 0 == 1/4th pel resolution (old default)
  // 1 == 1/8th pel resolution
  // ... could be enhanced straightforward, it may make sense to define
  // 2 == 1/2 pel resolution
  // 3 == full pel resolution

  if (UseStartCode)
  {
    // Input mode 0, File Format, and not Picture Start (first slice in picture)
    // This is the only case where a slice start code makes sense
    //
    // Putting a Slice Start Code is the same like the picture start code.  It's
    // n ot as straightforward as one thinks,  See remarks above.
    len += PutSliceStartCode(currStream);
  };

  // Now we have coded the Pciture header when appropriate, and the slice header when
  // appropriate.  Follow with the content of the slice header: GOB address of the slice
  // start and (initial) QP.  For the QP see remarks above.  For the GOB address, use
  // straigtforward procedure.  Note that this allows slices to start at MB addresses
  // up to 2^15 MBs due ot start code emulation problems.  This should be enough for
  // most practical applications,. but probably not for cinema.  Has to be discussed.
  // For the MPEG tests this is irrelevant, because there the rule will be one silce--
  // one picture.

  // Put MB-Adresse
  assert (MBPosition < (1<<15));
  SYMTRACESTRING("SH FirstMBInSlice");
  sym.value1 = MBPosition;
  len += writeSyntaxElement_UVLC (&sym, partition);

  // Put Quant.  It's a bit irrationale that we still put the same quant here, but it's
  // a good provision for the future.  In real-world applications slices typically
  // start with Intra information, and Intra MBs will likely use a different quant
  // than Inter

  SYMTRACESTRING("SH SliceQuant");
  sym.value1 = 31 - Quant;
  len += writeSyntaxElement_UVLC (&sym, partition);
  if (img->types==SP_IMG)
  {
    SYMTRACESTRING("SH SP SliceQuant");
    sym.value1 = 31 - img->qpsp;
    len += writeSyntaxElement_UVLC (&sym, partition);
  }
  // Put the Motion Vector resolution as per reflector consensus

  SYMTRACESTRING("SH MVResolution");
  sym.value1 = input->mv_res;
  len += writeSyntaxElement_UVLC (&sym, partition);

  return len;
}




int SequenceHeader (FILE *outf)
{
  int LenInBytes = 0;
  int HeaderInfo;         // Binary coded Headerinfo to be written in file
  int ProfileLevelVersionHash;


  // Handle the RTP case
  if (input->of_mode == PAR_OF_RTP)
  {
    if ((LenInBytes = RTPSequenceHeader (outf)) < 0)
    {
      snprintf (errortext, ET_SIZE, "SequenceHeaqder(): Problems writing the RTP Parameter Packet");
      error (errortext, 600);
      return -1;
    }
    else
      return LenInBytes*8;
  }
  // Non RTP-type file formats follow


  switch (input->SequenceHeaderType)
  {
  case 0:
    // No SequenceHeader, do nothing
    return 0;
  case 1:
    // A binary mini Sequence header.  Fixed length 32 bits.  Defined as follows
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |        0          |        1          |        2          | 3 |
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|2|3|4|5|6|7|8|9|0|1|
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |     TR Modulus        |     PicIDModulus      |OfMode |part |S|
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // S is symbol_mode (UVLC = 0, CABAC = 1), the other are as in the input file

    assert (input->TRModulus < 4096);
    assert (sizeof (int) == 4);
    assert (input->PicIdModulus <4096);
    assert (input->of_mode <16);
    assert (input->partition_mode <8);
    assert (input->symbol_mode < 2);

    ProfileLevelVersionHash = input->of_mode<<0 | input->partition_mode<<4 | input->symbol_mode<<7;
    HeaderInfo = input->TRModulus | input->PicIdModulus<<12 | ProfileLevelVersionHash<<24;

    if (1 != fwrite (&HeaderInfo, 4, 1, outf))
    {
      snprintf (errortext, ET_SIZE, "Error while writing Mini Sequence Header");
      error(errortext, 500);
    }
#if TRACE
    fprintf(p_trace, "Binary Mini Sequence Header 0x%x\n\n", HeaderInfo);
#endif

    return 32;

  case 2:
    // An ASCII representation of many input file parameters, useful for debug purposes
    //
    //
    assert ("To be hacked");
    return -1;
  case 3:
    // An UVLC coded representatioj of the Sequence Header.  To be hacked.
    // Note that all UVLC initialization has already taken place, so just use
    // write_synyaxelement() to put the sequence header symbols.  Has to be double
    // checked whether this is true for cabac as well.  Do we need a new context
    // for that?  Anyway:
    //
    assert ("to be hacked");
    return -1;
  default:
    snprintf (errortext, ET_SIZE, "Unspported Sequence Header Type (should not happen since checked in input module, exiting");
    error (errortext, 600);
    return -1;
  }
}

/********************************************************************************************
 ********************************************************************************************
 *
 * Local Support Functions
 *
 ********************************************************************************************
 ********************************************************************************************/

/*!
 ********************************************************************************************
 * \brief
 *    Puts a Picture Start Code into the Bitstream
 *
 * \return
 *    number of bits used for the PSC.
 *
 * \par Side effects:
 *    Adds picture start code to the Bitstream
 *
 * \par Remarks:
 *    THIS IS AN INTERIM SOLUTION FOR A PICTURE HEADER, see VCEG-M79
 *                                                                                        \par
 *    The PSC is a NAL functionality and, hence, should not be put into the
 *    bitstream by a module like this.  It was added here in response to
 *    the need for a quick hack for the MPEG tests.
 *    The PSC consists of a UVLC codeword of len 31 with info 0.  This results
 *    in 30 consecutove 0 bits in the bit stream, which should be easily
 *    identifyable if a need arises.
 ********************************************************************************************
*/

static int PutPictureStartCode (Bitstream *s)
{
  return PutStartCode (0, s, "\nPicture Header");
}


/*!
 ********************************************************************************************
 * \brief
 *    Puts a Slice Start Code into the Bitstream
 *
 * \return
 *    number of bits used for the PSC.
 *
 * \note
 *     See PutPictureStartCode()
 ********************************************************************************************
*/

static int PutSliceStartCode(Bitstream *s)
{
  return PutStartCode (1, s, "\nSlice Header");
}

/*!
 ********************************************************************************************
 * \brief Puts a Start Code into the Bitstream
 *    ts is a TraceString
 *
 * \return
 *    number of bits used for the PSC.
 *
 * \note
 *  See PutPictureStartCode()
 ********************************************************************************************
*/

static int PutStartCode (int Type, Bitstream *s, char *ts)
{

  SyntaxElement sym;

  // Putting the Start Codes is a bit tricky, because we cannot use writesyntaxelement()
  // directly.  Problem is that we want a codeword of len == 31 with an info of 0 or 1.
  // There is no simple mapping() rule to express this.  Of course, one could calculate
  // what info would have to be for such a case.  But I guess it's cleaner to use
  // the len/info interface directly.

  sym.len = LEN_STARTCODE;
  sym.inf = Type;
  sym.type = SE_HEADER;

#if TRACE
  strncpy(sym.tracestring, ts, TRACESTRING_SIZE);
#endif
  symbol2uvlc(&sym);      // generates the bit pattern
  writeUVLC2buffer(&sym, s);  // and puts it out to the buffer

#if TRACE
  trace2out(&sym);
#endif
  return LEN_STARTCODE;
}

// StW Note: This function is a hack.  It would be cleaner if the encoder maintains
// the picture type in the given format.  Note further that I have yet to understand
// why the encoder needs to know whether a picture is predicted from one or more
// reference pictures.

/*!
 ************************************************************************
 * \brief
 *    Selects picture type and codes it to symbol
 ************************************************************************
 */
void select_picture_type(SyntaxElement *symbol)
{
  int multpred;

#ifdef _ADDITIONAL_REFERENCE_FRAME_
  if (input->no_multpred <= 1 && input->add_ref_frame == 0)
#else
  if (input->no_multpred <= 1)
#endif
    multpred=FALSE;
  else
    multpred=TRUE;               // multiple reference frames in motion search

  if (img->type == INTRA_IMG)
  {
    symbol->len=3;
    symbol->inf=1;
    symbol->value1 = 2;
  }
  else if((img->type == INTER_IMG) && (multpred == FALSE) ) // inter single reference frame
  {
    symbol->len=1;
    symbol->inf=0;
    symbol->value1 = 0;
  }
  else if((img->type == INTER_IMG) && (multpred == TRUE)) // inter multiple reference frames
  {
    symbol->len=3;
    symbol->inf=0;
    symbol->value1 = 1;
  }
  else if((img->type == B_IMG) && (multpred == FALSE))
  {
    symbol->len=5;
    symbol->inf=0;
    symbol->value1 = 3;
  }
  else if((img->type == B_IMG) && (multpred == TRUE))
  {
    symbol->len=5;
    symbol->inf=1;
    symbol->value1 = 4;
  }
  else
  {
    error("Picture Type not supported!",1);
  }

  if((img->types == SP_IMG ) && (multpred == FALSE))
  {
    symbol->len=5;
    symbol->inf=0;
    symbol->value1 = 5;
  }
  else if((img->types == SP_IMG) && (multpred == TRUE))
  {
    symbol->len=5;
    symbol->inf=0;
    symbol->value1 = 6;
  }


#if TRACE
  snprintf(symbol->tracestring, TRACESTRING_SIZE, "Image type = %3d ", img->type);
#endif
  symbol->type = SE_PTYPE;
}


/*!
 ************************************************************************
 * \brief
 *    Writes the number of MBs of this slice
 ************************************************************************
 */
void LastMBInSlice()
{
  int dP_nr = assignSE2partition[input->partition_mode][SE_HEADER];
  DataPartition *partition = &((img->currentSlice)->partArr[dP_nr]);
  SyntaxElement sym;
  int d_MB_Nr;

  d_MB_Nr = img->current_mb_nr-img->currentSlice->start_mb_nr;
  if (d_MB_Nr == img->total_number_mb)
    d_MB_Nr = 0;

  sym.type = SE_HEADER;
  sym.mapping = n_linfo2;       // Mapping rule: Simple code number to len/info

  // Put MB-Adresse
  assert (d_MB_Nr < (1<<15));
  SYMTRACESTRING("SH Numbers of MB in Slice");
  sym.value1 = d_MB_Nr;
  writeSyntaxElement_UVLC (&sym, partition);
}


