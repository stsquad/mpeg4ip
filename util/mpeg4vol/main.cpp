


#include "mpeg4ip.h"
#include "mp4v2/mp4.h"
#include "mpeg4ip_bitstream.h"
#include "mpeg4ip_getopt.h"

extern void decode_audio (uint8_t *vol, uint32_t len);

static uint8_t tohex (char a)
{ 
  if (isdigit(a))
    return (a - '0');
  return (tolower(a) - 'a' + 10);
}

void CheckMarker (CBitstream *bs)
{
  if (bs->GetBits(1) != 1) {
    printf("Invalid marker bit\n");
  }
}

uint32_t CheckUserData (uint32_t header, CBitstream *bs)
{
  char *buffer;
  uint32_t in_buffer = 0;

  if (header != 0x000001b2) {
    return header;
  }
  printf("User Data\n  ");

  buffer = (char *)malloc(((bs->bits_remain()  + 7) / 8) + 1);
  bool done = false;
  while (done == false) {
    if (bs->bits_remain() < 8) {
      done = true;
      continue;
    }
    if (bs->bits_remain() > 24) 
      if (bs->PeekBits(24) == 0x000001) {
	done = true;
	continue;
      }
    buffer[in_buffer] = bs->GetBits(8);
    printf("%02x ", buffer[in_buffer]);
    if (!isprint(buffer[in_buffer])) {
      buffer[in_buffer] = ' ';
    }
    in_buffer++;
  }
  buffer[in_buffer] = '\0';
  printf("\n");
  
  printf("  \"%s\"\n", buffer);
  free(buffer);
  if (bs->bits_remain() >= 32)
    return bs->GetBits(32);
  return 0;
}
void LoadQuantTable (CBitstream *bs) 
{
  uint32_t temp;
  for (int q = 0; q < 64; q++) {
    temp = bs->GetBits(8);
    if (temp == 0) q = 64;
    printf("%u ", temp);
    if (((q + 1) % 16) == 0) printf("\n");
  }
  printf("\n");
}

static void decode (uint8_t *vol, uint32_t len)
{
  uint32_t ix;
  uint32_t temp;
#if 1
  for (ix = 0; ix < len; ix++) {
    printf("%02x ", vol[ix]);
  }
  printf("\n");
#endif

  uint32_t verid = 1;
  CBitstream *bs = new CBitstream(vol, len * 8);
  //bs->set_verbose(1);
  try {
    temp = bs->GetBits(32);
    if (temp == 0x000001b0) {
      printf("Video Object Sequence\n");
      printf("  profile_and_level_indication - 0x%x\n", bs->GetBits(8));
      temp = bs->GetBits(32);
    } else {
      printf("No Video Object Sequence\n");
    }
    do {
      temp = CheckUserData(temp, bs);
    } while (temp == 0x000001b2);
    if (temp == 0x000001b5) {
      printf("Visual Object\n");
      temp = bs->GetBits(1);
      printf(" is_visual_object_identifier - %u\n", temp);
      if (temp) {
	verid = bs->GetBits(4);
	printf("   visual_object_verid - %u\n", verid);
	printf("   visual_object_prioriy - %u\n", bs->GetBits(3));
      }
      temp = bs->GetBits(4);
      printf(" visual object type - %u\n", temp);
      if (temp == 0x01 || temp == 0x02) {
	// visual object type of video ID or Still texture ID
	temp = bs->GetBits(1);
	printf(" video_signal_type - %u\n", temp);
	if (temp) {
	  printf("   video_format - %u\n", bs->GetBits(3));
	  printf("   video_range - %u\n", bs->GetBits(1));
	  temp = bs->GetBits(1);
	  printf("   colour_description - %u\n", temp);
	  if (temp) {
	    printf("     colour_primaries - %u\n", bs->GetBits(8));
	    printf("     transfer_characteristics - %u\n", bs->GetBits(8));
	    printf("     matrix_coefficients - %u\n", bs->GetBits(8));
	  }
	}
      }
      bs->byte_align();
      temp = bs->GetBits(32);
    }
    do {
      temp = CheckUserData(temp, bs);
    } while (temp == 0x000001b2);
    if ((temp & 0xfffffff0) == 0x00000100) {
      printf("Video Object - %u\n", temp & 0xf);
      temp = bs->GetBits(32);
    } else {
      printf("No Video Object\n");
    }
      
    do {
      temp = CheckUserData(temp, bs);
    } while (temp == 0x000001b2);
    if ((temp & 0xfffffff0) != 0x00000120) {
      fprintf(stderr, "illegal start code %08x\n", temp);
      exit(1);
    }
    printf("Video Object Layer\n");
    printf("  random_accessible_vol - %u\n", bs->GetBits(1));
    uint8_t video_object_type;
    video_object_type = bs->GetBits(8);
    printf("  video object type - %u\n", video_object_type);
    if (video_object_type == 0x12) {
      printf("  fine granularity scalable not implemented\n");
      exit(1);
    }
    temp = bs->GetBits(1);
    printf("  is_object_layer_identifier - %u\n", temp);
    if (temp) {
      
      verid = bs->GetBits(4);
      printf("    verid - %u\n", verid);
      printf("    vol_priority - %u\n", bs->GetBits(3));
    }
    temp = bs->GetBits(4);
    printf("  aspect_ratio_info %u\n", temp);
    if (temp == 0xf) {
      printf("    par_width - %u\n", bs->GetBits(8));
      printf("    par_height - %u\n", bs->GetBits(8));
    }
    temp = bs->GetBits(1);
    printf("  vol_control_parameters - %u\n", temp);
    if (temp) {
      printf("    chroma format - %u\n", bs->GetBits(2));
      printf("    low delay - %u\n", bs->GetBits(1));
      temp = bs->GetBits(1);
      printf("    vbv_parameters - %u\n", temp); 
      if (temp) {
	temp = bs->GetBits(15) << 15;
	CheckMarker(bs);
	temp |= bs->GetBits(15);
	printf("        bit rate %u\n", temp);
	CheckMarker(bs);
	temp = bs->GetBits(15) << 3;
	CheckMarker(bs);
	temp |= bs->GetBits(3);
	printf("        vbv buffer size %u\n", temp);
	temp = bs->GetBits(11) << 15;
	CheckMarker(bs);
	temp |= bs->GetBits(15);
	printf("        vbv occupancy %u\n", temp);
	CheckMarker(bs);
      }
    }
    uint8_t vol_shape = bs->GetBits(2);
    uint8_t vol_shape_ext = 0;
#define VOL_SHAPE_RECT 0x0
#define VOL_SHAPE_BIN 0x1
#define VOL_SHAPE_BIN_ONLY 0x2
#define VOL_SHAPE_GRAYSCALE 0x3
    printf("  video object layer shape - %u\n", vol_shape);
    if (vol_shape == VOL_SHAPE_GRAYSCALE && verid != 1) {
      vol_shape_ext = bs->GetBits(4);
      printf("    vol_shape_extension - %u\n", vol_shape_ext);
      if (vol_shape_ext > 12) {
	fprintf(stderr, "Vol shape extension has wrong value\n");
	abort();
      }
    }
      
    CheckMarker(bs);
    uint32_t vop_time =  bs->GetBits(16);
    printf("  vop_time_increment_resolution - %u\n", vop_time);
    CheckMarker(bs);
    temp = bs->GetBits(1);
    printf("  fixed_vop_rate - %u\n", temp);
    if (temp) {
      u_int32_t powerOf2 = 1;
      int ix;
      for (ix = 0; ix < 16; ix++) {
	if (vop_time < powerOf2) {
	  break;
	}
	powerOf2 <<= 1;
      }
      printf("   fixed_vop_time_increment - bits %u %u\n", ix, bs->GetBits(ix));
    }
    if (vol_shape != VOL_SHAPE_BIN_ONLY) { // binary only
      if (vol_shape == VOL_SHAPE_RECT) { // rectangular
	CheckMarker(bs);
	printf("  vol width - %u\n", bs->GetBits(13));
	CheckMarker(bs);
	temp = bs->GetBits(13);
	printf("  vol height - %u\n", temp);
	CheckMarker(bs);
      }
      printf("  interlaced - %u\n", bs->GetBits(1));
      printf("  obmc_disable - %u\n", bs->GetBits(1));
      uint8_t sprite;
      sprite = bs->GetBits(verid == 1 ? 1 : 2);
      printf("  sprite - %u\n", sprite);
      if (sprite == 1 || sprite == 2) {
	if (sprite == 1) { // static 1, not GMC 2
	  printf("    width - %u\n", bs->GetBits(13));
	  CheckMarker(bs);
	  printf("    height - %u\n", bs->GetBits(13));
	  CheckMarker(bs);
	  printf("    left coordinate - %u\n", bs->GetBits(13));
	  CheckMarker(bs);
	  printf("    top coordinate - %u\n", bs->GetBits(13));
	  CheckMarker(bs);
	}
	printf("     no_of_sprite_warping_points - %u\n", bs->GetBits(6));
	printf("     sprite_warping_accuracy - %u\n", bs->GetBits(2));
	printf("     sprite brightness change - %u\n", bs->GetBits(1));
	
	if (sprite == 1) {
	  printf("     low_latency_sprite_enable - %u\n", bs->GetBits(1));
	}
      }
      if (verid != 1 &&
	  vol_shape != VOL_SHAPE_RECT) {
	printf("  sadct_disable - %u\n", bs->GetBits(1));
      }
      temp = bs->GetBits(1);
      printf("  not_8_bit - %u\n", temp);
      if (temp) {
	printf("    quant_precision - %u\n", bs->GetBits(4));
	printf("    bits_per_pixel - %u\n", bs->GetBits(4));
      }
      if (vol_shape == VOL_SHAPE_GRAYSCALE) {
	// greyscale
	printf("  no_gray_quant_update - %u\n", bs->GetBits(1));
	printf("  composition_method - %u\n", bs->GetBits(1));
	printf("  linear_composition - %u\n", bs->GetBits(1));
      }
      temp = bs->GetBits(1);
      printf("  quant_type - %u\n", temp);
      if (temp) {
	temp = bs->GetBits(1);
	printf("    load_intra_quant_mat - %u\n", temp);
	if (temp) {
	  LoadQuantTable(bs);
	}
	temp = bs->GetBits(1);
	printf("    load_nonintra_quant_mat - %u\n", temp);
	if (temp) {
	  LoadQuantTable(bs);
	}
	if (vol_shape == VOL_SHAPE_GRAYSCALE) {
	  static const uint8_t aux_comp[] = {
	    1, 1, 2, 2, 3, 1, 2, 1, 1, 2, 3, 2 ,3
	  };
	  for (ix = 0; ix < aux_comp[vol_shape_ext]; ix++) {
	    temp = bs->GetBits(1);
	    printf("    load_intra_quant_mat_grayscale %d - %u\n", ix, 
		   temp);
	    if (temp) {
	      LoadQuantTable(bs);
	    }
	    temp = bs->GetBits(1);
	    printf("    load_nonintra_quant_mat_grayscale %d - %u\n", ix, 
		   temp);
	    if (temp) {
	      LoadQuantTable(bs);
	    }
	  }
	}
      }
      
      if (verid != 1) {
	printf("  quarter sample - %u\n", bs->GetBits(1));
      }
      temp = bs->GetBits(1);
      printf("  complexity estimation - %u\n", temp);
      if (!temp) {
	uint8_t est_method;
	est_method = bs->GetBits(2);
	printf("     estimation_method - %u\n", est_method);
	if (est_method == 0 || est_method == 1) {
	  temp = bs->GetBits(1);
	  printf("     shape_complexity_estimation_disable - %u\n", temp);
	  if (!temp) {
	    printf("       opaque - %u\n", bs->GetBits(1));
	    printf("       transparent - %u\n", bs->GetBits(1));
	    printf("       intra_cae - %u\n", bs->GetBits(1));
	    printf("       inter_cae - %u\n", bs->GetBits(1));
	    printf("       no_update - %u\n", bs->GetBits(1));
	    printf("       upsampling - %u\n", bs->GetBits(1));
	  }
	  temp = bs->GetBits(1);
	  printf("     texture_complexity_estimation_set_1_disable - %u\n", temp);
	  if (!temp) {
	    printf("       intra_blocks - %u\n", bs->GetBits(1));
	    printf("       inter_blocks - %u\n", bs->GetBits(1));
	    printf("       inter4v_blocks - %u\n", bs->GetBits(1));
	    printf("       not_coded_blocks - %u\n", bs->GetBits(1));
	  }
	  CheckMarker(bs);
	  temp = bs->GetBits(1);
	  printf("     texture_complexity_estimation_set_2_disable - %u\n", temp);
	  if (!temp) {
	    printf("       dct_coefs - %u\n", bs->GetBits(1));
	    printf("       dct_lines - %u\n", bs->GetBits(1));
	    printf("       vlc_symbols - %u\n", bs->GetBits(1));
	    printf("       vlc_bits - %u\n", bs->GetBits(1));
	  }
	  CheckMarker(bs);
	  temp = bs->GetBits(1);
	  printf("     motion_compensation_complexity_disable - %u\n", temp);
	  if (!temp) {
	    printf("       apm - %u\n", bs->GetBits(1));
	    printf("       npm - %u\n", bs->GetBits(1));
	    printf("       interpolate_mc_q - %u\n", bs->GetBits(1));
	    printf("       forw_back_mc_q - %u\n", bs->GetBits(1));
	    printf("       halfpel2 - %u\n", bs->GetBits(1));
	    printf("       halfpel4 - %u\n", bs->GetBits(1));
	  }
	  CheckMarker(bs);
	  if (est_method == 1) {
	    temp = bs->GetBits(1);
	    printf("     version2_complexity_estimation_disable - %u\n", temp);
	    if (!temp) {
	      printf("       sadct - %u\n", bs->GetBits(1));
	      printf("       quarterpel - %u\n", bs->GetBits(1));
	    }
	  }
	}
      } // done with complexity estimation
      printf("  resync_marker_disable - %u\n", bs->GetBits(1));
      temp = bs->GetBits(1);
      printf("  data partitioned - %u\n", temp);
      if (temp) {
	printf("     reversible_vlc - %u\n", bs->GetBits(1));
      }
      if (verid != 1) {
	temp = bs->GetBits(1);
	printf("  newpred_enable - %u\n", temp);
	if (temp) {
	  printf("     requested_upstream_message_type - %u\n", bs->GetBits(2));
	  printf("     newpred_segment_type - %u\n", bs->GetBits(1));
	}
	printf("  reduced_resolution_vop_enable - %u\n", bs->GetBits(1));
      }
      temp = bs->GetBits(1);
      printf("  scalability - %u\n", temp);
      if (temp) {
	uint8_t heirarchy_type = bs->GetBits(1);
	printf("    heirarchy_type - %u\n", heirarchy_type);
	printf("    ref_layer_id - %u\n", bs->GetBits(4));
	printf("    ref_layer_sampling_direc - %u\n", bs->GetBits(1));
	printf("    hor_sampling_factor_n - %u\n", bs->GetBits(5));
	printf("    hor_sampling_factor_m - %u\n", bs->GetBits(5));
	printf("    vert_sampling_factor_n - %u\n", bs->GetBits(5));
	printf("    vert_sampling_factor_m - %u\n", bs->GetBits(5));
	printf("    enhancement_type - %u\n", bs->GetBits(1));
	if (vol_shape == VOL_SHAPE_BIN &&
	    heirarchy_type == 0) {
	  printf("    use_ref_shape - %u\n", bs->GetBits(1));
	  printf("    use_ref_texture - %u\n", bs->GetBits(1));
	  printf("    shape_hor_sampling_factor_n - %u\n", bs->GetBits(5));
	  printf("    shape_hor_sampling_factor_m - %u\n", bs->GetBits(5));
	  printf("    shape_vert_sampling_factor_n - %u\n", bs->GetBits(5));
	  printf("    shape_vert_sampling_factor_m - %u\n", bs->GetBits(5));
	}
      }
    } else {
      // binary only
      if (verid != 1) {
	temp = bs->GetBits(1);
	printf("  scalability - %u\n", temp);
	if (temp) {
	  printf("    shape_hor_sampling_factor_n - %u\n", bs->GetBits(5));
	  printf("    shape_hor_sampling_factor_m - %u\n", bs->GetBits(5));
	  printf("    shape_vert_sampling_factor_n - %u\n", bs->GetBits(5));
	  printf("    shape_vert_sampling_factor_m - %u\n", bs->GetBits(5));
	}
	printf("  resync_marker_disable - %u\n", bs->GetBits(1));
      }
    }
    bs->byte_align();
    if (bs->bits_remain() > 32) {
      temp = bs->GetBits(32);
      do {
	temp = CheckUserData(temp, bs);
      } while (temp == 0x000001b2);
    }
  } catch (...) {
    fprintf(stderr, "bitstream read error\n");
  }
  return;
}

  
int main (int argc, char *argv[])
{
  bool audio_decode = false;
  int len = 0;
  char *allargs = NULL, *step;

  static struct option orig_options[] = {
    { "latm", 0, 0, 'l' },
    { "help", 0, 0, 'h' },
    { NULL, 0, 0, 0 }
  };
  opterr = 0;
  while (true) {
    int c = -1;
    int option_index = 0;
    
    c = getopt_long_only(argc, argv, "ah", orig_options, &option_index);

    if (c == -1)
      break;
    
    switch (c) {
    case 'l':	// -l <latm config>
      audio_decode = true;
      break;
    case 'h':
      fprintf(stderr, 
	      "Usage: %s [-f config_file] [--automatic] [--headless] [--sdp] [--<config variable>=<value>]\n",
	      argv[0]);
      fprintf(stderr, "Use [--config-vars] to dump configuration variables\n");
      exit(-1);
    }
  }

  fprintf(stderr, "argc %d optind %d\n", argc, optind);
  if(argc > optind) {
    // There are non-option args left, probably file names
    argv += optind;
    argc -= optind;

    printf("processing %s\n", *argv);
    while (argc > 0 && strcasestr(*argv, ".mp4") != NULL) {
      MP4FileHandle mp4File;
	
      if (audio_decode) {
	fprintf(stderr, "No decode of audio from mpeg4 files\n");
	return -1;
      }
      mp4File = MP4Read(*argv, MP4_DETAILS_ERROR);
      if (MP4_IS_VALID_FILE_HANDLE(mp4File)) {
	MP4TrackId tid;
	uint32_t ix = 0;
	do {
	  uint32_t verb = MP4GetVerbosity(mp4File);
	  MP4SetVerbosity(mp4File, verb & ~(MP4_DETAILS_ERROR));
	  tid = MP4FindTrackId(mp4File, ix, MP4_VIDEO_TRACK_TYPE);
	  MP4SetVerbosity(mp4File, verb);
	  if (MP4_IS_VALID_TRACK_ID(tid)) {
	    uint8_t type = MP4GetTrackEsdsObjectTypeId(mp4File, tid);
	    if (type == MP4_MPEG4_VIDEO_TYPE) {
	      uint8_t *foo;
	      uint32_t bufsize;
	      MP4GetTrackESConfiguration(mp4File, tid, &foo, &bufsize);
	      if (foo != NULL && bufsize != 0) {
		printf("%s\n", *argv);
		decode(foo, bufsize);
		free(foo);
	      } else {
		fprintf(stderr, "%s - track %d - can't find esds\n", *argv, tid);
	      }
	    } else {
	      fprintf(stderr, "%s - track %d is not MPEG4 - type %u\n", 
		      *argv, tid, type);
	    }
	  }
	  ix++;
	} while (MP4_IS_VALID_TRACK_ID(tid));
      } else {
	fprintf(stderr, "%s is not a valid mp4 file\n", *argv);
      }
      argc--;
      argv++;
    }
  }
  if (argc > 0) {
    len = 1;
    while (argc > 0) {
      len += strlen(*argv);
      if (allargs == NULL) {
	allargs = (char *)malloc(len);
	allargs[0] = '\0';
      } else 
	allargs = (char *)realloc(allargs, len);
      strcat(allargs, *argv);
      argv++;
      argc--;
    }
    if ((len - 1) & 0x1) {
      fprintf(stderr, "odd length VOL\n");
      exit(1);
    }
    len /= 2;
    uint8_t *vol = (uint8_t *)malloc(len), *write;
    write = vol;
    step = allargs;
    int ix;
    for (ix = 0; ix < len; ix++) {
      *write = 0;
      *write = tohex(*step) << 4;
      step++;
      *write |= tohex(*step);
      step++;
      write++;
    }
    
    printf("decoding vol \"%s\"\n", allargs);
    if (audio_decode) {
      decode_audio(vol, len);
    } else {
      decode(vol, len);
    }
  }

  return(0);
}
    
    
