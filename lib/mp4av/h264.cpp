/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May wmay@cisco.com
 */

#include "mpeg4ip.h"
#include "mp4av_h264.h"
#include "mpeg4ip_bitstream.h"
//#define BOUND_VERBOSE 1

static uint8_t exp_golomb_bits[256] = {
8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3, 
3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 
2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 
};

uint32_t h264_ue (CBitstream *bs)
{
  uint32_t bits, read;
  int bits_left;
  uint8_t coded;
  bool done = false;
  uint32_t temp;
  bits = 0;
  // we want to read 8 bits at a time - if we don't have 8 bits, 
  // read what's left, and shift.  The exp_golomb_bits calc remains the
  // same.
  while (done == false) {
    bits_left = bs->bits_remain();
    if (bits_left < 8) {
      read = bs->PeekBits(bits_left) << (8 - bits_left);
      done = true;
    } else {
      read = bs->PeekBits(8);
      if (read == 0) {
	(void)bs->GetBits(8);
	bits += 8;
      } else {
	done = true;
      }
    }
  }
  coded = exp_golomb_bits[read];
  temp = bs->GetBits(coded);
  bits += coded;

  //  printf("ue - bits %d\n", bits);
  return bs->GetBits(bits + 1) - 1;
}

int32_t h264_se (CBitstream *bs) 
{
  uint32_t ret;
  ret = h264_ue(bs);
  if ((ret & 0x1) == 0) {
    ret >>= 1;
    int32_t temp = 0 - ret;
    return temp;
  } 
  return (ret + 1) >> 1;
}

static void h264_decode_annexb( uint8_t *dst, int *dstlen,
                                const uint8_t *src, const int srclen )
{
  uint8_t *dst_sav = dst;
  const uint8_t *end = &src[srclen];

  while (src < end)
  {
    if (src < end - 3 && src[0] == 0x00 && src[1] == 0x00 &&
        src[2] == 0x03)
    {
      *dst++ = 0x00;
      *dst++ = 0x00;

      src += 3;
      continue;
    }
    *dst++ = *src++;
  }

  *dstlen = dst - dst_sav;
}

extern "C" bool h264_is_start_code (const uint8_t *pBuf) 
{
  if (pBuf[0] == 0 && 
      pBuf[1] == 0 && 
      ((pBuf[2] == 1) ||
       ((pBuf[2] == 0) && pBuf[3] == 1))) {
    return true;
  }
  return false;
}

extern "C" uint32_t h264_find_next_start_code (const uint8_t *pBuf, 
					       uint32_t bufLen)
{
  uint32_t val, temp;
  uint32_t offset;

  offset = 0;
  if (pBuf[0] == 0 && 
      pBuf[1] == 0 && 
      ((pBuf[2] == 1) ||
       ((pBuf[2] == 0) && pBuf[3] == 1))) {
    pBuf += 3;
    offset = 3;
  }
  val = 0xffffffff;
  while (offset < bufLen - 3) {
    val <<= 8;
    temp = val & 0xff000000;
    val &= 0x00ffffff;
    val |= *pBuf++;
    offset++;
    if (val == H264_START_CODE) {
      if (temp == 0) return offset - 4;
      return offset - 3;
    }
  }
  return 0;
}

extern "C" uint8_t h264_nal_unit_type (const uint8_t *buffer)
{
  uint32_t offset;
  if (buffer[2] == 1) offset = 3;
  else offset = 4;
  return buffer[offset] & 0x1f;
}

extern "C" int h264_nal_unit_type_is_slice (const uint8_t type)
{
  if (type >= H264_NAL_TYPE_NON_IDR_SLICE && 
      type <= H264_NAL_TYPE_IDR_SLICE) {
    return true;
  }
  return false;
}

/*
 * determine if the slice we decoded is a sync point
 */
extern "C" bool h264_slice_is_idr (h264_decode_t *dec) 
{
  if (dec->nal_unit_type != H264_NAL_TYPE_IDR_SLICE)
    return false;
  if (H264_TYPE_IS_I(dec->slice_type)) return true;
  if (H264_TYPE_IS_SI(dec->slice_type)) return true;
  return false;
}

extern "C" uint8_t h264_nal_ref_idc (const uint8_t *buffer)
{
  uint32_t offset;
  if (buffer[2] == 1) offset = 3;
  else offset = 4;
  return (buffer[offset] >> 5) & 0x3;
}

static void scaling_list (uint sizeOfScalingList, CBitstream *bs)
{
  uint lastScale = 8, nextScale = 8;
  uint j;

  for (j = 0; j < sizeOfScalingList; j++) {
    if (nextScale != 0) {
      int deltaScale = h264_se(bs);
      nextScale = (lastScale + deltaScale + 256) % 256;
    }
    if (nextScale == 0) {
      lastScale = lastScale;
    } else {
      lastScale = nextScale;
    }
  }
}

int h264_read_seq_info (const uint8_t *buffer, 
			uint32_t buflen, 
			h264_decode_t *dec)
{
  CBitstream bs;
  uint32_t header;
  uint8_t tmp[2048]; /* Should be enough for all SPS (we have at worst 13 bytes and 496 se/ue in frext) */
  int tmp_len;
  uint32_t dummy;

  if (buffer[2] == 1) header = 4;
  else header = 5;

  h264_decode_annexb( tmp, &tmp_len, buffer + header, MIN(buflen-header,2048) );
  bs.init(tmp, tmp_len * 8);

  //bs.set_verbose(true);
  try {
    dec->profile = bs.GetBits(8);
    dummy = bs.GetBits(1 + 1 + 1 + 1 + 4);
    dec->level = bs.GetBits(8);
    (void)h264_ue(&bs); // seq_parameter_set_id
    if (dec->profile == 100 || dec->profile == 110 ||
	dec->profile == 122 || dec->profile == 144) {
      dec->chroma_format_idc = h264_ue(&bs);
      if (dec->chroma_format_idc == 3) {
	dec->residual_colour_transform_flag = bs.GetBits(1);
      }
      dec->bit_depth_luma_minus8 = h264_ue(&bs);
      dec->bit_depth_chroma_minus8 = h264_ue(&bs);
      dec->qpprime_y_zero_transform_bypass_flag = bs.GetBits(1);
      dec->seq_scaling_matrix_present_flag = bs.GetBits(1);
      if (dec->seq_scaling_matrix_present_flag) {
	for (uint ix = 0; ix < 8; ix++) {
	  if (bs.GetBits(1)) {
	    scaling_list(ix < 6 ? 16 : 64, &bs);
	  }
	}
      }
    }
    dec->log2_max_frame_num_minus4 = h264_ue(&bs);
    dec->pic_order_cnt_type = h264_ue(&bs);
    if (dec->pic_order_cnt_type == 0) {
      dec->log2_max_pic_order_cnt_lsb_minus4 = h264_ue(&bs);
    } else if (dec->pic_order_cnt_type == 1) {
      dec->delta_pic_order_always_zero_flag = bs.GetBits(1);
      dec->offset_for_non_ref_pic = h264_se(&bs); // offset_for_non_ref_pic
      dec->offset_for_top_to_bottom_field = h264_se(&bs); // offset_for_top_to_bottom_field
      dec->pic_order_cnt_cycle_length = h264_ue(&bs); // poc_cycle_length
      for (uint32_t ix = 0; ix < dec->pic_order_cnt_cycle_length; ix++) {
        dec->offset_for_ref_frame[MIN(ix,255)] = h264_se(&bs); // offset for ref fram -
      }
    }
    dummy = h264_ue(&bs); // num_ref_frames
    dummy = bs.GetBits(1); // gaps_in_frame_num_value_allowed_flag
    uint32_t PicWidthInMbs = h264_ue(&bs) + 1;
    dec->pic_width = PicWidthInMbs * 16;
    uint32_t PicHeightInMapUnits = h264_ue(&bs) + 1;

    dec->frame_mbs_only_flag = bs.GetBits(1);
    dec->pic_height = 
      (2 - dec->frame_mbs_only_flag) * PicHeightInMapUnits * 16;
#if 0
    if (!dec->frame_mbs_only_flag) {
      printf("    mb_adaptive_frame_field_flag: %u\n", bs->GetBits(1));
    }
    printf("   direct_8x8_inference_flag: %u\n", bs->GetBits(1));
    temp = bs->GetBits(1);
    printf("   frame_cropping_flag: %u\n", temp);
    if (temp) {
      printf("     frame_crop_left_offset: %u\n", h264_ue(bs));
      printf("     frame_crop_right_offset: %u\n", h264_ue(bs));
      printf("     frame_crop_top_offset: %u\n", h264_ue(bs));
      printf("     frame_crop_bottom_offset: %u\n", h264_ue(bs));
    }
    temp = bs->GetBits(1);
    printf("   vui_parameters_present_flag: %u\n", temp);
    if (temp) {
      h264_vui_parameters(bs);
    }
#endif
  } catch (...) {
    return -1;
  }
  return 0;
}
extern "C" int h264_find_slice_type (const uint8_t *buffer, 
				     uint32_t buflen,
				     uint8_t *slice_type, 
				     bool noheader)
{
  uint32_t header;
  uint32_t dummy;
  if (noheader) header = 1;
  else {
    if (buffer[2] == 1) header = 4;
    else header = 5;
  }
  CBitstream bs;
  bs.init(buffer + header, (buflen - header) * 8);
  try {
    dummy = h264_ue(&bs); // first_mb_in_slice
    *slice_type = h264_ue(&bs); // slice type
  } catch (...) {
    return -1;
  }
  return 0;
}

int h264_read_slice_info (const uint8_t *buffer, 
			  uint32_t buflen, 
			  h264_decode_t *dec)
{
  uint32_t header;
  uint8_t tmp[512]; /* Enough for the begining of the slice header */
  int tmp_len;
  uint32_t temp;

  if (buffer[2] == 1) header = 4;
  else header = 5;
  CBitstream bs;

  h264_decode_annexb( tmp, &tmp_len, buffer + header, MIN(buflen-header,512) );
  bs.init(tmp, tmp_len * 8);
  try {
    dec->field_pic_flag = 0;
    dec->bottom_field_flag = 0;
    dec->delta_pic_order_cnt[0] = 0;
    dec->delta_pic_order_cnt[1] = 0;
    temp = h264_ue(&bs); // first_mb_in_slice
    dec->slice_type = h264_ue(&bs); // slice type
    temp = h264_ue(&bs); // pic_parameter_set
    dec->frame_num = bs.GetBits(dec->log2_max_frame_num_minus4 + 4);
    if (!dec->frame_mbs_only_flag) {
      dec->field_pic_flag = bs.GetBits(1);
      if (dec->field_pic_flag) {
	dec->bottom_field_flag = bs.GetBits(1);
      }
    }
    if (dec->nal_unit_type == H264_NAL_TYPE_IDR_SLICE) {
      dec->idr_pic_id = h264_ue(&bs);
    }
    switch (dec->pic_order_cnt_type) {
    case 0:
      dec->pic_order_cnt_lsb = bs.GetBits(dec->log2_max_pic_order_cnt_lsb_minus4 + 4);
      if (dec->pic_order_present_flag && !dec->field_pic_flag) {
	dec->delta_pic_order_cnt_bottom = h264_se(&bs);
      }
      break;
    case 1:
      if (!dec->delta_pic_order_always_zero_flag) {
	dec->delta_pic_order_cnt[0] = h264_se(&bs);
      }
      if (dec->pic_order_present_flag && !dec->field_pic_flag) {
	dec->delta_pic_order_cnt[1] = h264_se(&bs);
      }
      break;
    }

  } catch (...) {
    return -1;
  }
  return 0;
}

static void h264_compute_poc( h264_decode_t *dec ) {
  const int max_frame_num = 1 << (dec->log2_max_frame_num_minus4 + 4);
  int field_poc[2] = {0,0};
  enum {
    H264_PICTURE_FRAME,
    H264_PICTURE_FIELD_TOP,
    H264_PICTURE_FIELD_BOTTOM,
  } pic_type;

  /* FIXME FIXME it doesn't handle the case where there is a MMCO == 5
   * (MMCO 5 "emulates" an idr) */
  
  /* picture type */
  if (dec->frame_mbs_only_flag || !dec->field_pic_flag)
    pic_type = H264_PICTURE_FRAME;
  else if (dec->bottom_field_flag)
    pic_type = H264_PICTURE_FIELD_BOTTOM;
  else
    pic_type = H264_PICTURE_FIELD_TOP;

  /* frame_num_offset */
  if (dec->nal_unit_type == H264_NAL_TYPE_IDR_SLICE) {
    dec->pic_order_cnt_lsb_prev = 0;
    dec->pic_order_cnt_msb_prev = 0;
    dec->frame_num_offset = 0;
  } else {
    if (dec->frame_num < dec->frame_num_prev)
      dec->frame_num_offset = dec->frame_num_offset_prev + max_frame_num;
    else
      dec->frame_num_offset = dec->frame_num_offset_prev;
  }

  /* */
  if(dec->pic_order_cnt_type == 0) {
    const unsigned int max_poc_lsb = 1 << (dec->log2_max_pic_order_cnt_lsb_minus4 + 4);

    if (dec->pic_order_cnt_lsb < dec->pic_order_cnt_lsb_prev &&
        dec->pic_order_cnt_lsb_prev - dec->pic_order_cnt_lsb >= max_poc_lsb / 2)
      dec->pic_order_cnt_msb = dec->pic_order_cnt_msb_prev + max_poc_lsb;
    else if (dec->pic_order_cnt_lsb > dec->pic_order_cnt_lsb_prev &&
             dec->pic_order_cnt_lsb - dec->pic_order_cnt_lsb_prev > max_poc_lsb / 2)
      dec->pic_order_cnt_msb = dec->pic_order_cnt_msb_prev - max_poc_lsb;
    else
      dec->pic_order_cnt_msb = dec->pic_order_cnt_msb_prev;

    field_poc[0] = dec->pic_order_cnt_msb + dec->pic_order_cnt_lsb;
    field_poc[1] = field_poc[0];
    if (pic_type == H264_PICTURE_FRAME)
      field_poc[1] += dec->delta_pic_order_cnt_bottom;

  } else if (dec->pic_order_cnt_type == 1) {
    int abs_frame_num, expected_delta_per_poc_cycle, expected_poc;

    if (dec->pic_order_cnt_cycle_length != 0)
      abs_frame_num = dec->frame_num_offset + dec->frame_num;
    else
      abs_frame_num = 0;

    if (dec->nal_ref_idc == 0 && abs_frame_num > 0)
      abs_frame_num--;

    expected_delta_per_poc_cycle = 0;
    for (int i = 0; i < (int)dec->pic_order_cnt_cycle_length; i++ )
      expected_delta_per_poc_cycle += dec->offset_for_ref_frame[i];

    if (abs_frame_num > 0) {
      const int poc_cycle_cnt = ( abs_frame_num - 1 ) / dec->pic_order_cnt_cycle_length;
      const int frame_num_in_poc_cycle = ( abs_frame_num - 1 ) % dec->pic_order_cnt_cycle_length;

      expected_poc = poc_cycle_cnt * expected_delta_per_poc_cycle;
      for (int i = 0; i <= frame_num_in_poc_cycle; i++)
        expected_poc += dec->offset_for_ref_frame[i];
    } else {
      expected_poc = 0;
    }

    if (dec->nal_ref_idc == 0)
      expected_poc += dec->offset_for_non_ref_pic;

    field_poc[0] = expected_poc + dec->delta_pic_order_cnt[0];
    field_poc[1] = field_poc[0] + dec->offset_for_top_to_bottom_field;

    if (pic_type == H264_PICTURE_FRAME)
      field_poc[1] += dec->delta_pic_order_cnt[1];

  } else if (dec->pic_order_cnt_type == 2) {
    int poc;
    if (dec->nal_unit_type == H264_NAL_TYPE_IDR_SLICE) {
      poc = 0;
    } else {
      const int abs_frame_num = dec->frame_num_offset + dec->frame_num;
      if (dec->nal_ref_idc != 0)
        poc = 2 * abs_frame_num;
      else
        poc = 2 * abs_frame_num - 1;
    }
    field_poc[0] = poc;
    field_poc[1] = poc;
  }

  /* */
  if (pic_type == H264_PICTURE_FRAME)
    dec->pic_order_cnt = MIN(field_poc[0], field_poc[1] );
  else if (pic_type == H264_PICTURE_FIELD_TOP)
    dec->pic_order_cnt = field_poc[0];
  else
    dec->pic_order_cnt = field_poc[1];
}


extern "C" int h264_detect_boundary (const uint8_t *buffer, 
				     uint32_t buflen, 
				     h264_decode_t *decode)
{
  uint8_t temp;
  h264_decode_t new_decode;
  int ret;
  int slice = 0;
  memcpy(&new_decode, decode, sizeof(new_decode));

  temp = new_decode.nal_unit_type = h264_nal_unit_type(buffer);
  new_decode.nal_ref_idc = h264_nal_ref_idc(buffer);
  ret = 0;
  switch (temp) {
  case H264_NAL_TYPE_ACCESS_UNIT:
  case H264_NAL_TYPE_END_OF_SEQ:
  case H264_NAL_TYPE_END_OF_STREAM:
#ifdef BOUND_VERBOSE
    printf("nal type %d\n", temp);
#endif
    ret = 1;
    break;
  case H264_NAL_TYPE_NON_IDR_SLICE:
  case H264_NAL_TYPE_DP_A_SLICE:
  case H264_NAL_TYPE_DP_B_SLICE:
  case H264_NAL_TYPE_DP_C_SLICE:
  case H264_NAL_TYPE_IDR_SLICE:
    slice = 1;
    // slice buffer - read the info into the new_decode, and compare.
    if (h264_read_slice_info(buffer, buflen, &new_decode) < 0) {
      // need more memory
      return -1;
    }
    if (decode->nal_unit_type > H264_NAL_TYPE_IDR_SLICE || 
	decode->nal_unit_type < H264_NAL_TYPE_NON_IDR_SLICE) {
      break;
    }
    if (decode->frame_num != new_decode.frame_num) {
#ifdef BOUND_VERBOSE
      printf("frame num values different %u %u\n", decode->frame_num, 
	     new_decode.frame_num);
#endif
      ret = 1;
      break;
    }
    if (decode->field_pic_flag != new_decode.field_pic_flag) {
      ret = 1;
#ifdef BOUND_VERBOSE
      printf("field pic values different\n");
#endif
      break;
    }
    if (decode->nal_ref_idc != new_decode.nal_ref_idc &&
	(decode->nal_ref_idc == 0 ||
	 new_decode.nal_ref_idc == 0)) {
#ifdef BOUND_VERBOSE
      printf("nal ref idc values differ\n");
#endif
      ret = 1;
      break;
    }
    if (decode->frame_num == new_decode.frame_num &&
	decode->pic_order_cnt_type == new_decode.pic_order_cnt_type) {
      if (decode->pic_order_cnt_type == 0) {
	if (decode->pic_order_cnt_lsb != new_decode.pic_order_cnt_lsb) {
#ifdef BOUND_VERBOSE
	  printf("pic order 1\n");
#endif
	  ret = 1;
	  break;
	}
	if (decode->delta_pic_order_cnt_bottom != new_decode.delta_pic_order_cnt_bottom) {
	  ret = 1;
#ifdef BOUND_VERBOSE
	  printf("delta pic order cnt bottom 1\n");
#endif
	  break;
	}
      } else if (decode->pic_order_cnt_type == 1) {
	if (decode->delta_pic_order_cnt[0] != new_decode.delta_pic_order_cnt[0]) {
	  ret =1;
#ifdef BOUND_VERBOSE
	  printf("delta pic order cnt [0]\n");
#endif
	  break;
	}
	if (decode->delta_pic_order_cnt[1] != new_decode.delta_pic_order_cnt[1]) {
	  ret = 1;
#ifdef BOUND_VERBOSE
	  printf("delta pic order cnt [1]\n");
#endif
	  break;
	  
	}
      }
    }
    if (decode->nal_unit_type == H264_NAL_TYPE_IDR_SLICE &&
	new_decode.nal_unit_type == H264_NAL_TYPE_IDR_SLICE) {
      if (decode->idr_pic_id != new_decode.idr_pic_id) {
#ifdef BOUND_VERBOSE
	printf("idr_pic id\n");
#endif
	
	ret = 1;
	break;
      }
    }
    break;
  case H264_NAL_TYPE_SEQ_PARAM:
    if (h264_read_seq_info(buffer, buflen, &new_decode) < 0) {
      return -1;
    }
    // fall through
  default:
    if (decode->nal_unit_type <= H264_NAL_TYPE_IDR_SLICE) ret = 1;
    else ret = 0;
  } 

  /* save _prev values */
  if (ret)
  {
    new_decode.frame_num_offset_prev = decode->frame_num_offset;
    if (decode->pic_order_cnt_type != 2 || decode->nal_ref_idc != 0)
      new_decode.frame_num_prev = decode->frame_num;
    if (decode->nal_ref_idc != 0)
    {
      new_decode.pic_order_cnt_lsb_prev = decode->pic_order_cnt_lsb;
      new_decode.pic_order_cnt_msb_prev = decode->pic_order_cnt_msb;
    }
  }

  if( slice ) {  // XXX we compute poc for every slice in a picture (but it's not needed)
    h264_compute_poc( &new_decode );
  }


  // other types (6, 7, 8, 
#ifdef BOUND_VERBOSE
  if (ret == 0) {
    printf("no change\n");
  }
#endif

  memcpy(decode, &new_decode, sizeof(*decode));
  return ret;
}

uint32_t h264_read_sei_value (const uint8_t *buffer, uint32_t *size) 
{
  uint32_t ret = 0;
  *size = 1;
  while (buffer[*size] == 0xff) {
    ret += 255;
    *size = *size + 1;
  }
  ret += *buffer;
  return ret;
}

extern "C" const char *h264_get_slice_name (const uint8_t slice_type)
{
  if (H264_TYPE_IS_P(slice_type)) return "P";  
  if (H264_TYPE_IS_B(slice_type)) return "B";  
  if (H264_TYPE_IS_I(slice_type)) return "I";
  if (H264_TYPE_IS_SI(slice_type)) return "SI";
  if (H264_TYPE_IS_SP(slice_type)) return "SP";
  return "UNK";
}

extern "C" bool h264_access_unit_is_sync (const uint8_t *pNal, uint32_t len)
{
  uint8_t nal_type;
  h264_decode_t dec;
  uint32_t offset;
  do {
    nal_type = h264_nal_unit_type(pNal);
    if (nal_type == H264_NAL_TYPE_SEQ_PARAM) return true;
    if (nal_type == H264_NAL_TYPE_PIC_PARAM) return true;
    if (nal_type == H264_NAL_TYPE_IDR_SLICE) return true;
    if (h264_nal_unit_type_is_slice(nal_type)) {
      if (h264_read_slice_info(pNal, len, &dec) < 0) return false;
      if (H264_TYPE_IS_I(dec.slice_type) ||
	  H264_TYPE_IS_SI(dec.slice_type)) return true;
      return false;
    }
    offset = h264_find_next_start_code(pNal, len);
    if (offset == 0 || offset > len) return false;
    pNal += offset;
    len -= offset;
  } while (len > 0);
  return false;
}

extern "C" char *h264_get_profile_level_string (const uint8_t profile, 
						const uint8_t level)
{
  const char *pro;
  char profileb[20], levelb[20];
  if (profile == 66) {
    pro = "Baseline";
  } else if (profile == 77) {
    pro = "Main";
  } else if (profile == 88) {
    pro =  "Extended";
  } else if (profile == 100) {
    pro = "High";
  } else if (profile == 110) {
    pro =  "High 10";
  } else if (profile == 122) {
    pro = "High 4:2:2";
  } else if (profile == 144) {
    pro = "High 4:4:4";
  } else {
    snprintf(profileb, sizeof(profileb), "Unknown Profile %x", profile);
    pro = profileb;
  } 
  switch (level) {
  case 10: case 20: case 30: case 40: case 50:
    snprintf(levelb, sizeof(levelb), "%u", level / 10);
    break;
  case 11: case 12: case 13:
  case 21: case 22:
  case 31: case 32:
  case 41: case 42:
  case 51:
    snprintf(levelb, sizeof(levelb), "%u.%u", level / 10, level % 10);
    break;
  default:
    snprintf(levelb, sizeof(levelb), "unknown level %x", level);
    break;
  }
  uint len =
    1 + strlen("H.264 @") + strlen(pro) + strlen(levelb);
  char *typebuffer = 
    (char *)malloc(len);
  if (typebuffer == NULL) return NULL;

  snprintf(typebuffer, len,  "H.264 %s@%s", pro, levelb);
  return typebuffer;
}
