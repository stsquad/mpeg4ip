#include "mpeg4ip.h"
#include <bitstream.h>
#include <math.h>

#define H264_START_CODE 0x000001
#define H264_PREVENT_3_BYTE 0x000003

typedef struct h264_decode_t {
  uint32_t log2_max_frame_num_minus4;
  uint32_t log2_max_pic_order_cnt_lsb_minus4;
  uint32_t pic_order_cnt_type;
  uint8_t frame_mbs_only_flag;
  uint8_t pic_order_present_flag;
  uint8_t delta_pic_order_always_zero_flag;

  uint8_t nal_ref_idc;
  uint8_t nal_unit_type;
  
  uint8_t field_pic_flag;
  uint8_t bottom_field_flag;
  uint32_t frame_num;
  uint32_t idr_pic_id;
  uint32_t pic_order_cnt_lsb;
  int32_t delta_pic_order_cnt_bottom;
  int32_t delta_pic_order_cnt[2];
} h264_decode_t;

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
  uint8_t coded;

  bits = 0;
  while ((read = bs->PeekBits(8)) == 0) {
    bs->GetBits(8);
    bits += 8;
  }
  coded = exp_golomb_bits[read];
  bs->GetBits(coded);
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

void h264_check_0s (CBitstream *bs, int count)
{
  uint32_t val;
  val = bs->GetBits(count);
  if (val != 0) {
    printf("field error - %d bits should be 0 is %x\n", 
	   count, val);
  }
}

void h264_hrd_parameters (CBitstream *bs)
{
  uint32_t cpb_cnt = h264_ue(bs);
  printf("      cpb_cnt_minus1: %u\n", cpb_cnt);
  printf("      bit_rate_scale: %u\n", bs->GetBits(4));
  printf("      cpb_size_scale: %u\n", bs->GetBits(4));
  for (uint32_t ix = 0; ix < cpb_cnt; cpb_cnt++) {
    printf("      bit_rate_value_minus1[%u]: %u\n", ix, h264_ue(bs));
    printf("      cpb_size_value_minus1[%u]: %u\n", ix, h264_ue(bs));
    printf("      cbr_flag[%u]: %u\n", ix, bs->GetBits(1));
  }
  printf("      initial_cpb_removal_delay_length_minus1: %u\n", bs->GetBits(5));
  printf("      cpb_removal_delay_length_minus1: %u\n", bs->GetBits(5));
  printf("      dpb_output_delay_length_minus1: %u\n", bs->GetBits(5));
  printf("      time_offset_delay: %u\n", bs->GetBits(5));
}

void h264_vui_parameters (CBitstream *bs)
{
  uint32_t temp, temp2;
  temp = bs->GetBits(1);
  printf("    aspect_ratio_info_present_flag: %u\n", temp);
  if (temp) {
    temp = bs->GetBits(8);
    printf("     aspect_ratio_idc:%u\n", temp);
    if (temp == 0xff) { // extended_SAR
      printf("      sar_width: %u\n", bs->GetBits(16));
      printf("      sar_height: %u\n", bs->GetBits(16));
    }
  }
  temp = bs->GetBits(1);
  printf("    overscan_info_present_flag: %u\n", temp);
  if (temp) {
    printf("     overscan_appropriate_flag: %u\n", bs->GetBits(1));
  }
  temp = bs->GetBits(1);
  printf("    video_signal_info_present_flag: %u\n", temp);
  if (temp) {
    printf("     video_format: %u\n", bs->GetBits(3));
    printf("     video_full_range_flag: %u\n", bs->GetBits(1));
    temp = bs->GetBits(1);
    printf("     colour_description_present_flag: %u\n", temp);
    if (temp) {
      printf("      colour_primaries: %u\n", bs->GetBits(8));
      printf("      transfer_characteristics: %u\n", bs->GetBits(8));
      printf("      matrix_coefficients: %u\n", bs->GetBits(8));
    }
  }
    
  temp = bs->GetBits(1);
  printf("    chroma_loc_info_present_flag: %u\n", temp);
  if (temp) {
    printf("     chroma_sample_loc_type_top_field: %u\n", h264_ue(bs));
    printf("     chroma_sample_loc_type_bottom_field: %u\n", h264_ue(bs));
  }
  temp = bs->GetBits(1);
  printf("    timing_info_present_flag: %u\n", temp);
  if (temp) {
    printf("     num_units_in_tick: %u\n", bs->GetBits(32));
    printf("     time_scale: %u\n", bs->GetBits(32));
    printf("     fixed_frame_scale: %u\n", bs->GetBits(1));
  }
  temp = bs->GetBits(1);
  printf("    nal_hrd_parameters_present_flag: %u\n", temp);
  if (temp) {
    h264_hrd_parameters(bs);
  }
  temp2 = bs->GetBits(1);
  printf("    vcl_hrd_parameters_present_flag: %u\n", temp2);
  if (temp2) {
    h264_hrd_parameters(bs);
  }
  if (temp || temp2) {
    printf("     low_delay_hrd_flag: %u\n", bs->GetBits(1));
    printf("     pic_struct_present_flag: %u\n", bs->GetBits(1));
    temp = bs->GetBits(1);
    if (temp) {
      printf("      motion_vectors_over_pic_boundaries_flag: %u\n", bs->GetBits(1));
      printf("      max_bytes_per_pic_denom: %u\n", h264_ue(bs));
      printf("      max_bits_per_mb_denom: %u\n", h264_ue(bs));
      printf("      log2_max_mv_length_horizontal: %u\n", h264_ue(bs));
      printf("      log2_max_mv_length_vertical: %u\n", h264_ue(bs));
      printf("      num_reorder_frames: %u\n", h264_ue(bs));
      printf("      max_dec_frame_buffering: %u\n", h264_ue(bs));
    }
  }
}
    
static uint32_t calc_ceil_log2 (uint32_t val)
{
  uint32_t ix, cval;

  ix = 0; cval = 1;
  while (ix < 32) {
    if (cval >= val) return ix;
    cval <<= 1;
    ix++;
  }
  return ix;
}

void h264_parse_sequence_parameter_set (h264_decode_t *dec, CBitstream *bs)
{
  uint32_t temp;
  printf("   profile %x\n", bs->GetBits(8));
  printf("   constaint_set0_flag: %d\n", bs->GetBits(1));
  printf("   constaint_set1_flag: %d\n", bs->GetBits(1));
  printf("   constaint_set2_flag: %d\n", bs->GetBits(1));
  h264_check_0s(bs, 5);
  printf("   level_idc: %u\n", bs->GetBits(8));
  printf("   seq parameter set id: %u\n", h264_ue(bs));
  dec->log2_max_frame_num_minus4 = h264_ue(bs);
  printf("   log2_max_frame_num_minus4: %u\n", dec->log2_max_frame_num_minus4);
  dec->pic_order_cnt_type = h264_ue(bs);
  printf("   pic_order_cnt_type: %u\n", dec->pic_order_cnt_type);
  if (dec->pic_order_cnt_type == 0) {
    dec->log2_max_pic_order_cnt_lsb_minus4 = h264_ue(bs);
    printf("    log2_max_pic_order_cnt_lsb_minus4: %u\n", 
	   dec->log2_max_pic_order_cnt_lsb_minus4);
  } else if (dec->pic_order_cnt_type == 1) {
    dec->delta_pic_order_always_zero_flag = bs->GetBits(1);
    printf("    delta_pic_order_always_zero_flag: %u\n", 
	   dec->delta_pic_order_always_zero_flag);
    printf("    offset_for_non_ref_pic: %d\n", h264_se(bs));
    printf("    offset_for_top_to_bottom_field: %d\n", h264_se(bs));
    temp = h264_ue(bs);
    for (uint32_t ix = 0; ix < temp; ix++) {
      printf("      offset_for_ref_frame[%u]: %d\n",
	     ix, h264_se(bs));
    }
  }
  printf("   num_ref_frames: %u\n", h264_ue(bs));
  printf("   gaps_in_frame_num_value_allowed_flag: %u\n", bs->GetBits(1));
  uint32_t PicWidthInMbs = h264_ue(bs) + 1;
    
  printf("   pic_width_in_mbs_minus1: %u (%u)\n", PicWidthInMbs - 1, 
	 PicWidthInMbs * 16);
  uint32_t PicHeightInMapUnits = h264_ue(bs) + 1;

  printf("   pic_height_in_map_minus1: %u\n", 
	 PicHeightInMapUnits - 1);
  dec->frame_mbs_only_flag = bs->GetBits(1);
  printf("   frame_mbs_only_flag: %u\n", dec->frame_mbs_only_flag);
  printf("     derived height: %u\n", (2 - dec->frame_mbs_only_flag) * PicHeightInMapUnits * 16);
  if (!dec->frame_mbs_only_flag) {
    printf("    mb_adaptive_frame_field_flag: %u\n", bs->GetBits(1));
  }
  printf("   direct_8x8_inference_flag: %u\n", bs->GetBits(1));
  temp = bs->GetBits(1);
  printf("   frame_cropping_flag: %u\n", bs->GetBits(1));
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
}
void h264_parse_pic_parameter_set (h264_decode_t *dec, CBitstream *bs)
{
  uint32_t num_slice_groups, temp, iGroup;
    printf("   pic_parameter_set_id: %u\n", h264_ue(bs));
    printf("   seq_parameter_set_id: %u\n", h264_ue(bs));
    printf("   entropy_coding_mode_flag: %u\n", bs->GetBits(1));
    dec->pic_order_present_flag = bs->GetBits(1);
    printf("   pic_order_present_flag: %u\n", dec->pic_order_present_flag);
    num_slice_groups = h264_ue(bs);
    printf("   num_slice_groups_minus1: %u\n", num_slice_groups);
    if (num_slice_groups > 0) {
      temp = h264_ue(bs);
      printf("    slice_group_map_type: %u\n", temp);
      if (temp == 0) {
	for (iGroup = 0; iGroup <= num_slice_groups; iGroup++) {
	  printf("     run_length_minus1[%u]: %u\n", iGroup, h264_ue(bs));
	}
      } else if (temp == 2) {
	for (iGroup = 0; iGroup < num_slice_groups; iGroup++) {
	  printf("     top_left[%u]: %u\n", iGroup, h264_ue(bs));
	  printf("     bottom_right[%u]: %u\n", iGroup, h264_ue(bs));
	}
      } else if (temp < 6) { // 3, 4, 5
	printf("     slice_group_change_direction_flag: %u\n", bs->GetBits(1));
	printf("     slice_group_change_rate_minus1: %u\n", h264_ue(bs));
      } else if (temp == 6) {
	temp = h264_ue(bs);
	printf("     pic_size_in_map_units_minus1: %u\n", temp);
	uint32_t bits = calc_ceil_log2(num_slice_groups + 1);
	printf("     bits - %u\n", bits);
	for (iGroup = 0; iGroup <= temp; iGroup++) {
	  printf("      slice_group_id[%u]: %u\n", iGroup, bs->GetBits(bits));
	}
      }
    }
    printf("   num_ref_idx_l0_active_minus1: %u\n", h264_ue(bs));
    printf("   num_ref_idx_l1_active_minus1: %u\n", h264_ue(bs));
    printf("   weighted_pred_flag: %u\n", bs->GetBits(1));
    printf("   weighted_bipred_idc: %u\n", bs->GetBits(2));
    printf("   pic_init_qp_minus26: %u\n", h264_se(bs));
    printf("   pic_init_qs_minus26: %u\n", h264_se(bs));
    printf("   chroma_qp_index_offset: %u\n", h264_se(bs));
    printf("   deblocking_filter_control_present_flag: %u\n", bs->GetBits(1));
    printf("   constrained_intra_pred_flag: %u\n", bs->GetBits(1));
    printf("   redundant_pic_cnt_present_flag: %u\n", bs->GetBits(1));
	  
}

uint32_t h264_find_next_start_code (uint8_t *pBuf, 
				    uint32_t bufLen)
{
  uint32_t val;
  uint32_t offset;

  offset = 0;
  if (pBuf[0] == 0 && pBuf[1] == 0 && pBuf[2] == 1) {
    pBuf += 3;
    offset = 3;
  }
  val = 0xffffffff;
  while (offset < bufLen - 3) {
    val <<= 8;
    val &= 0x00ffffff;
    val |= *pBuf++;
    offset++;
    if (val == H264_START_CODE) {
      return offset - 3;
    }
  }
  return 0;
}

static const char *nal[] = {
  "Coded slice of non-IDR picture",
  "Coded slice data partition A", 
  "Coded slice data partition B", 
  "Coded slice data partition C", 
  "Coded slice of an IDR picture",
  "SEI",
  "Sequence parameter set",
  "Picture parameter set",
  "Access unit delimeter", 
  "End of Sequence",
  "end of stream", 
  "filler data",
};

static const char *nal_unit_type (uint8_t type)
{
  if (type == 0 || type >= 24) {
    return "unspecified";
  }
  if (type < 13) {
    return nal[type - 1];
  }
  return "reserved";
}
const char *slice_type[] = {
  "P", 
  "B", 
  "I", 
  "SP", 
  "SI", 
  "P", 
  "B", 
  "I",
  "SP",
  "SI",
};
void h264_slice_header (h264_decode_t *dec, CBitstream *bs)
{
  uint32_t temp;
  printf("   first_mb_in_slice: %u\n", h264_ue(bs));
  temp = h264_ue(bs);
  printf("   slice_type: %u (%s)\n", temp, temp < 10 ? slice_type[temp] : "invalid");
  printf("   pic_parameter_set_id: %u\n", h264_ue(bs));
  dec->frame_num = bs->GetBits(dec->log2_max_frame_num_minus4 + 4);
  printf("   frame_num: %u (%u bits)\n", 
	 dec->frame_num, 
	 dec->log2_max_frame_num_minus4 + 4);
  // these are defaults
  dec->field_pic_flag = 0;
  dec->bottom_field_flag = 0;
  dec->delta_pic_order_cnt[0] = 0;
  dec->delta_pic_order_cnt[1] = 0;
  if (!dec->frame_mbs_only_flag) {
    dec->field_pic_flag = bs->GetBits(1);
    printf("   field_pic_flag: %u\n", dec->field_pic_flag);
    if (dec->field_pic_flag) {
      dec->bottom_field_flag = bs->GetBits(1);
      printf("    bottom_field_flag: %u\n", dec->bottom_field_flag);
    }
  }
  if (dec->nal_unit_type == 5) {
    dec->idr_pic_id = h264_ue(bs);
    printf("   idr_pic_id: %u\n", dec->idr_pic_id);
  }
  switch (dec->pic_order_cnt_type) {
  case 0:
    dec->pic_order_cnt_lsb = bs->GetBits(dec->log2_max_pic_order_cnt_lsb_minus4 + 4);
    printf("   pic_order_cnt_lsb: %u\n", dec->pic_order_cnt_lsb);
    if (dec->pic_order_present_flag && !dec->field_pic_flag) {
      dec->delta_pic_order_cnt_bottom = h264_se(bs);
      printf("   delta_pic_order_cnt_bottom: %u\n", 
	     dec->delta_pic_order_cnt_bottom);
    }
    break;
  case 1:
    if (!dec->delta_pic_order_always_zero_flag) {
      dec->delta_pic_order_cnt[0] = h264_se(bs);
      printf("   delta_pic_order_cnt[0]: %d\n", 
	     dec->delta_pic_order_cnt[0]);
    }
    if (dec->pic_order_present_flag && !dec->field_pic_flag) {
      dec->delta_pic_order_cnt[1] = h264_se(bs);
      printf("   delta_pic_order_cnt[1]: %d\n", 
	     dec->delta_pic_order_cnt[1]);
    }
    break;
  }
  
}    

void h264_slice_layer_without_partitioning (h264_decode_t *dec, 
					    CBitstream *bs)
{
  h264_slice_header(dec, bs);
}
uint8_t h264_parse_nal (h264_decode_t *dec, CBitstream *bs)
{
  uint8_t type = 0;

  try {
    bs->GetBits(24);
    h264_check_0s(bs, 1);
    dec->nal_ref_idc = bs->GetBits(2);
    dec->nal_unit_type = type = bs->GetBits(5);
    printf(" ref %u type %u %s\n", dec->nal_ref_idc, type, nal_unit_type(type));
    switch (type) {
    case 1:
    case 5:
      h264_slice_layer_without_partitioning(dec, bs);
      break;
    case 7:
      h264_parse_sequence_parameter_set(dec, bs);
      break;
    case 8:
      h264_parse_pic_parameter_set(dec, bs);
      break;
    case 9:
      printf("   primary_pic_type: %u\n", bs->GetBits(3));
      break;
    }
  } catch (...) {
    printf("Error reading bitstream\n");
  }
  return type;
}
  
// false if different picture, true if nal is the same picture
bool compare_boundary (h264_decode_t *prev, h264_decode_t *on)
{
  if (prev->frame_num != on->frame_num) {
    return false;
  }
  if (prev->field_pic_flag != on->field_pic_flag) {
    return false;
  }
  if (prev->nal_ref_idc != on->nal_ref_idc &&
      (prev->nal_ref_idc == 0 ||
       on->nal_ref_idc == 0)) {
    return false;
  }
  
  if (prev->frame_num == on->frame_num &&
      prev->pic_order_cnt_type == on->pic_order_cnt_type) {
    if (prev->pic_order_cnt_type == 0) {
      if (prev->pic_order_cnt_lsb != on->pic_order_cnt_lsb) {
	return false;
      }
      if (prev->delta_pic_order_cnt_bottom != on->delta_pic_order_cnt_bottom) {
	return false;
      }
    } else if (prev->pic_order_cnt_type == 1) {
      if (prev->delta_pic_order_cnt[0] != on->delta_pic_order_cnt[0]) {
	return false;
      }
      if (prev->delta_pic_order_cnt[1] != on->delta_pic_order_cnt[1]) {
	return false;
      }
    }
  }

  if (prev->nal_unit_type == 5 &&
      on->nal_unit_type == 5) {
    if (prev->idr_pic_id != on->idr_pic_id) {
      return false;
    }
  }

  return true;
  
}
int main (int argc, char **argv)
{
#define MAX_BUFFER 65536

  uint8_t buffer[MAX_BUFFER];
  uint32_t buffer_on, buffer_size;
  uint64_t bytes = 0;
  FILE *m_file;
  h264_decode_t dec, prevdec;
  bool have_prevdec = false;
  memset(&dec, 0, sizeof(dec));
#if 0
  uint8_t count = 0;
  // this prints out the 8-bit to # of zero bit array that we use
  // to decode ue(v)
  for (uint32_t ix = 0; ix <= 255; ix++) {
    uint8_t ij;
    uint8_t bit = 0x80;
    for (ij = 0;
	 (bit & ix) == 0 && ij < 8; 
	 ij++, bit >>= 1);
    printf("%d, ", ij);
    count++;
    if (count > 16) {
      printf("\n");
      count = 0;
    }
  }
  printf("\n");
#endif
  argc--;
  argv++;

  m_file = fopen(*argv, FOPEN_READ_BINARY);

  if (m_file == NULL) {
    fprintf(stderr, "file %s not found\n", *argv);
    exit(-1);
  }

  buffer_on = buffer_size = 0;
  while (!feof(m_file)) {
    bytes += buffer_on;
    if (buffer_on != 0) {
      buffer_on = buffer_size - buffer_on;
      memmove(buffer, &buffer[buffer_size - buffer_on], buffer_on);
    }
    buffer_size = fread(buffer + buffer_on, 
			1, 
			MAX_BUFFER - buffer_on, 
			m_file);
    buffer_size += buffer_on;
    buffer_on = 0;

    bool done = false;
    CBitstream ourbs;
    do {
      uint32_t ret;
      ret = h264_find_next_start_code(buffer + buffer_on, 
				      buffer_size - buffer_on);
      if (ret == 0) {
	done = true;
	if (buffer_on == 0) {
	  fprintf(stderr, "couldn't find start code in buffer from 0\n");
	  exit(-1);
	}
      } else {
	// have a complete NAL from buffer_on to end
	if (ret > 3) {
	  printf("Nal length %d\n", ret);
	  ourbs.init(buffer + buffer_on, ret * 8);
	  uint8_t type = h264_parse_nal(&dec, &ourbs);
	  if (type >= 1 && type <= 5) {
	    if (have_prevdec) {
	      // compare the 2
	      bool ret;
	      ret = compare_boundary(&prevdec, &dec);
	      printf("Nal is %s\n", ret ? "part of last picture" : "new picture");
	    }
	    memcpy(&prevdec, &dec, sizeof(dec));
	    have_prevdec = true;
	  } else if (type >= 9 && type <= 11) {
	    have_prevdec = false; // don't need to check
	  }
	}
#if 0
	printf("buffer on "X64" "X64" %u len %u %02x %02x %02x %02x\n",
	       bytes + buffer_on, 
	       bytes + buffer_on + ret,
	       buffer_on, 
	       ret,
	       buffer[buffer_on],
	       buffer[buffer_on+1],
	       buffer[buffer_on+2],
	       buffer[buffer_on+3]);
#endif
	buffer_on += ret; // buffer_on points to next code
      }
    } while (done == false);
  }
  fclose(m_file);
  return 0;
}
