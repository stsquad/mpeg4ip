
#ifndef __MP4AV_H264_H__
#define __MP4AV_H264_H__ 1

#define H264_START_CODE 0x000001
#define H264_PREVENT_3_BYTE 0x000003

#define H264_NAL_TYPE_SEQ_PARAM 0x7
#define H264_NAL_TYPE_PIC_PARAM 0x8

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

  uint32_t pic_width, pic_height;
} h264_decode_t;

#ifdef __cplusplus
extern "C" {
#endif


uint32_t h264_find_next_start_code(uint8_t *pBuf, 
				   uint32_t bufLen);

  uint8_t h264_nal_unit_type(uint8_t *buffer);

  int h264_nal_unit_type_is_slice(uint8_t nal_type);
  uint8_t h264_nal_ref_idc(uint8_t *buffer);
  int h264_detect_boundary(uint8_t *buffer, 
			   uint32_t buflen, 
			   h264_decode_t *decode);


#ifdef __cplusplus
 }
#endif

#endif
