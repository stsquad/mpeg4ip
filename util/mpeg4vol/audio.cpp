#include "mpeg4ip.h"
#include "mp4v2/mp4.h"
#include <mpeg4ip_bitstream.h>

void CheckProgramConfigElement (CBitstream *bs) {
  uint32_t temp, i, num_front_channel_elements, num_side_channel_elements, num_back_channel_elements;
  uint32_t num_lfe_channel_elements, num_assoc_data_elements, num_valid_cc_elements;
  
  printf("  Program Config Element\n");
  temp = bs->GetBits(4);
  printf("    element_instance_tag - 0x%x\n", temp);

  temp = bs->GetBits(2);
  printf("    object_type - 0x%x\n", temp);

  temp = bs->GetBits(4);
  printf("    sampling_frequency_index - 0x%x\n", temp);

  num_front_channel_elements = bs->GetBits(4);
  printf("    num_front_channel_elements - 0x%x\n", num_front_channel_elements);

  num_side_channel_elements = bs->GetBits(4);
  printf("    num_side_channel_elements - 0x%x\n", num_side_channel_elements);

  num_back_channel_elements = bs->GetBits(4);
  printf("    num_back_channel_elements - 0x%x\n", num_back_channel_elements);

  num_lfe_channel_elements = bs->GetBits(2);
  printf("    num_lfe_channel_elements - 0x%x\n", num_lfe_channel_elements);

  num_assoc_data_elements = bs->GetBits(3);
  printf("    num_assoc_data_elements - 0x%x\n", num_assoc_data_elements);

  num_valid_cc_elements = bs->GetBits(4);
  printf("    num_valid_cc_elements - 0x%x\n", num_valid_cc_elements);

  temp = bs->GetBits(1);
  printf("    mono_mixdown_present - 0x%x\n", temp);
  if(temp) {
    temp = bs->GetBits(4);
    printf("      mono_mixdown_element_number - 0x%x\n", temp);
  }

  temp = bs->GetBits(1);
  printf("    stereo_mixdown_present - 0x%x\n", temp);
  if(temp) {
    temp = bs->GetBits(4);
    printf("      stereo_mixdown_element_number - 0x%x\n", temp);
  }

  temp = bs->GetBits(1);
  printf("    matrix_mixdown_idx_present - 0x%x\n", temp);
  if(temp) {
    temp = bs->GetBits(2);
    printf("      matrix_mixdown_idx - 0x%x\n", temp);

    temp = bs->GetBits(1);
    printf("      pseudo_surround_enable - 0x%x\n", temp);
  }

  for (i = 0; i < num_front_channel_elements; i++) {
    printf("    front_channel_element - %i\n", i);
    temp = bs->GetBits(1);
    printf("      front_element_is_cpei? - 0x%x\n", temp);
    temp = bs->GetBits(4);
    printf("      front_element_tag_selecti? - 0x%x\n", temp);
  }

  for (i = 0; i < num_side_channel_elements; i++) {  
    printf("    side_channel_element - %i\n", i);
    temp = bs->GetBits(1);
    printf("      side_element_is_cpei? - 0x%x\n", temp);
    temp = bs->GetBits(4);
    printf("      side_element_tag_selecti? - 0x%x\n", temp);
  }

  for (i = 0; i < num_back_channel_elements; i++) {  
    printf("    back_channel_element - %i\n", i);
    temp = bs->GetBits(1);
    printf("      back_element_is_cpei? - 0x%x\n", temp);
    temp = bs->GetBits(4);
    printf("      back_element_tag_selecti? - 0x%x\n", temp);
  } 

  for (i = 0; i < num_lfe_channel_elements; i++) {
    printf("    lfe_channel_element - %i\n", i);
    temp = bs->GetBits(4);
    printf("      lfe_element_tag_selecti? - 0x%x\n", temp);
  }
  
  for (i = 0; i < num_assoc_data_elements; i++) {
    printf("    assoc_data_element - %i\n", i);
    temp = bs->GetBits(4);
    printf("      assoc_data_element_tag_selecti? - 0x%x\n", temp);
  }
  
  for (i = 0; i < num_valid_cc_elements; i++) {  
    printf("    valid_cc__element - %i\n", i);
    temp = bs->GetBits(1);
    printf("      cc_element_is_ind_swi? - 0x%x\n", temp);
    temp = bs->GetBits(4);
    printf("      valid_cc_element_tag_selecti? - 0x%x\n", temp);
  }

  bs->byte_align();
  uint32_t comment_field_bytes = bs->GetBits(8);
  printf("    comment_field_bytes - 0x%x\n", comment_field_bytes);
  if(comment_field_bytes > 0) {
    char *comment_field = (char *)malloc(comment_field_bytes+1);
    printf("    comment_field: (0x");
    for (i = 0; i < comment_field_bytes; i++) {
      temp = bs->GetBits(8);
      printf("%x", temp);
      comment_field[i] = temp;
    }
    comment_field[comment_field_bytes] = '\0';
    printf(") %s\n", comment_field);
    free(comment_field);
  }

  printf("  End of Program Config Element\n");
}

// See ISO  14496-3:2001 section 1.6.2
void CheckAudioSpecificConfig (CBitstream *bs) {

  uint32_t temp;
	uint32_t audioObjectType, channelConfiguration, extensionFlag;
  uint32_t i, j;

  printf("AudioSpecificConfig\n");

  // Audio Object Type
  audioObjectType = bs->GetBits(5);
  printf("Audio object type (5 bit) - 0x%x - ", audioObjectType);
  switch(audioObjectType) {
    case 0:
      printf("null\n");
      break;
    case 1:
      printf("AAC Main\n");
      break;
    case 2:
      printf("AAC LC\n");
      break;
    case 3:
      printf("AAC SSR\n");
      break;
    case 4:
      printf("AAC LTP\n");
      break;
    case 5:
      printf("reserved\n");
      break;
    case 6:
      printf("AAC Scalable\n");
      break;
    case 7:
      printf("TwinVQ\n");
      break;
    case 8:
      printf("CELP\n");
      break;
    case 9:
      printf("HVXC\n");
      break;
    case 10:
      printf("reserved\n");
      break;
    case 11:
      printf("reserved\n");
      break;
    case 12:
      printf("TTSI\n");
      break;
    case 13:
      printf("Main synthetic\n");
      break;
    case 14:
      printf("Wavetable synthesis\n");
      break;
    case 15:
      printf("General MIDI\n");
      break;
    case 16:
      printf("Algorithmic Syntesis and Audio FX\n");
      break;
    case 17:
      printf("ER AAC LC\n");
      break;
    case 18:
      printf("ER AAC SSR\n");
      break;
    case 19:
      printf("ER AAC LTP\n");
      break;
    case 20:
      printf("ER AAC scaleable\n");
      break;
    case 21:
      printf("ER TwinVQ\n");
      break;
    case 22:
      printf("ER Fine Granule Audio\n");
      break;
    case 23:
      printf("ER AAC LD\n");
      break;
    case 24:
      printf("ER CELP\n");
      break;
    case 25:
      printf("ER HVXC\n");
      break;
    case 26:
      printf("ER HILN\n");
      break;
    case 27:
      printf("ER Parametric\n");
      break;
    case 28:
      printf("reserved\n");
      break;
    case 29:
      printf("reserved\n");
      break;
    case 30:
      printf("reserved\n");
      break;
    case 31:
      printf("reserved\n");
      break;
    default:
      printf("unknown\n");
  }

  // Sample Frequency Index
  temp = bs->GetBits(4);
  printf("Sampling frequency Index (4 bit) - 0x%x\n", temp);
  if(temp == 0xf) {
    temp = bs->GetBits(24);
    printf("Sampling frequency (24 bit) - 0x%x\n", temp);
  }

  // Channel configuration
  channelConfiguration = bs->GetBits(4);
  printf("Channel configuration (4 bit) - 0x%x\n", channelConfiguration);

  if ( audioObjectType == 1 || audioObjectType == 2 ||
       audioObjectType == 3 || audioObjectType == 4 ||
       audioObjectType == 6 || audioObjectType == 7 ||
      /* the following Objects are Amendment 1 Objects */
       audioObjectType == 17 || audioObjectType == 19 ||
       audioObjectType == 20 || audioObjectType == 21 ||
       audioObjectType == 22 || audioObjectType == 23 ) {
    // GASpecificConfig
    printf("GASpecificConfig\n");
    temp = bs->GetBits(1);
    printf("  FrameLength (1 bit) - 0x%x\n", temp);
    temp = bs->GetBits(1);
    printf("  DependsOnCoreCoder (1 bit) - 0x%x\n", temp);
    if(temp) {
      temp = bs->GetBits(14);
      printf("    CoreCoderDelay (14 bit) - 0x%x\n", temp);
    }
    extensionFlag = bs->GetBits(1);
    printf("  ExtensionFlag (1 bit) - 0x%x\n", extensionFlag);

    if ( !channelConfiguration ) {
      //program_config_element();
      CheckProgramConfigElement(bs);
    }
    if ( extensionFlag ) {
      if ( audioObjectType == 22 ) {
        temp = bs->GetBits(5);
        printf("  NumOfSubFrame (5 bit) - 0x%x\n", temp);
        temp = bs->GetBits(11);
        printf("  Layer_length (11 bit) - 0x%x\n", temp);
      }
      if( audioObjectType==17 || audioObjectType == 18 ||
          audioObjectType == 19 || audioObjectType == 20 ||
          audioObjectType == 21 || audioObjectType == 23) {
        temp = bs->GetBits(1);
        printf("  AacSectionDataResilienceFlag (1 bit) - 0x%x\n", temp);
        temp = bs->GetBits(1);
        printf("  AacScalefactorDataResilienceFlag (1 bit) - 0x%x\n", temp);
        temp = bs->GetBits(1);
        printf("  AacSpectralDataResilienceFlag (1 bit) - 0x%x\n", temp);
      }
      temp = bs->GetBits(1);
      printf("  ExtensionFlag3 (1 bit) - 0x%x\n", temp);
      if ( temp ) {
        /* tbd in version 3 */
      }
    }
    printf("End of GASpecificConfig\n");
  }

  if ( audioObjectType == 8 ) {
    // CelpSpecificConfig(), defined in ISO/IEC 14496-3 subpart 3
    printf("  CelpSpecificConfig\n");
    uint32_t excitationMode = bs->GetBits(1);
    printf("    excitationMode (1 bit) - 0x%x\n", excitationMode);

    temp = bs->GetBits(1);
    printf("    sampleRateMode (1 bit) - 0x%x\n", temp);

    temp = bs->GetBits(1);
    printf("    fineRateMode (1 bit) - 0x%x\n", temp);

    if(excitationMode == 1) {
    	printf("    excitationMode  == RegularPulseExc\n");
      temp = bs->GetBits(3);
      printf("    RPE_Configuration (3 bit) - 0x%x\n", temp);
    } else {
    	printf("    excitationMode  == MultiPulseExc\n");
      temp = bs->GetBits(5);
      printf("      MPE_Configuration (5 bit) - 0x%x\n", temp);

      temp = bs->GetBits(2);
      printf("      numEnhLayers (2 bit) - 0x%x\n", temp);

      temp = bs->GetBits(1);
      printf("      bandwidthScalabilityMode (1 bit) - 0x%x\n", temp);
    }
    printf("  End of CelpSpecificConfig\n");
  }

  if ( audioObjectType == 9 ) {
    //HvxcSpecificConfig();
    printf("  HvxcSpecificConfig not implemented yet, defined in ISO/IEC 14496-3 subpart 2.\n");
  }

  if ( audioObjectType == 12 ) {
    // TTSSpecificConfig();
    printf("  TTSSpecificConfig not implemented yet, defined in ISO/IEC 14496-3 subpart 6.\n");
  }

  if ( audioObjectType == 13 || audioObjectType == 14 || audioObjectType == 15 || audioObjectType==16) {
    // StructuredAudioSpecificConfig();
    printf("  StructuredAudioSpecificConfig not implemented yet, defined in ISO/IEC 14496-3 subpart 5.\n");
  }

  if ( audioObjectType == 24) {
    // ErrorResilientCelpSpecificConfig();
    printf("  ErrorResilientCelpSpecificConfig not implemented yet, defined in ISO/IEC 14496-3 subpart 3.\n");
  }

  if ( audioObjectType == 25) {
    temp = bs->GetBits(1);
    printf("  HVXCvarMode (1 bit) - 0x%x\n", temp);
    temp = bs->GetBits(2);
    printf("  HVXCreateMode (2 bit) - 0x%x\n", temp);
    temp = bs->GetBits(1);
    printf("  extensionFlag (1 bit) - 0x%x\n", temp);
    if(temp) {
      temp = bs->GetBits(1);
      printf("    var_ScalableFlag (1 bit)- 0x%x\n", temp);
    }
  }

  if ( audioObjectType == 26 || audioObjectType == 27) {
    // ParametricSpecificConfig();
    printf("  ParametricSpecificConfig not implemented yet, defined in ISO/IEC 14496-3 subpart 7.\n");
  }

  // This is different in amd 1, which to follow?
  if ( audioObjectType == 17 || audioObjectType == 19 ||
       audioObjectType == 20 || audioObjectType == 21 ||
       audioObjectType == 22 || audioObjectType == 23 ||
       audioObjectType == 24 || audioObjectType == 25 ||
       audioObjectType == 26 || audioObjectType == 27 ) {
    uint32_t epConfig = bs->GetBits(2);
    printf("  epConfig (2 bit) - 0x%x\n", temp);
    if ( epConfig == 2 || epConfig == 3 ) {
      // ErrorProtectionSpecificConfig();

      uint32_t number_of_predefined_set = bs->GetBits(8);
      printf("    number_of_predefined_set (8 bit) - 0x%x\n", number_of_predefined_set);

      uint32_t interleave_type = bs->GetBits(2);
      printf("    interleave_type (2 bit) - 0x%x\n", interleave_type);

      temp = bs->GetBits(3);
      printf("    bit_stuffing (3 bit) - 0x%x\n", temp);

      uint32_t number_of_concatenated_frame = bs->GetBits(3);
      printf("    number_of_concatenated_frame (3 bit) - 0x%x\n", number_of_concatenated_frame);

      for ( i = 0; i < number_of_predefined_set; i++ ) {
        printf("    predefined_set - %i\n", i);

        uint32_t number_of_class = bs->GetBits(6);
        printf("      number_of_class[%i] (6 bit) - 0x%x\n", i, number_of_class);

        for ( j = 0; j < number_of_class; j++) {
          printf("    class - %i\n", j);
          uint32_t length_escape = bs->GetBits(1);
          printf("      length_escape[%i][%i] (1 bit) - 0x%x\n", i, j, length_escape);
          uint32_t rate_escape = bs->GetBits(1);
          printf("      rate_escape[%i][%i] (1 bit) - 0x%x\n", i, j, rate_escape);
          uint32_t crclen_escape = bs->GetBits(1);
          printf("      crclen_escape[%i][%i] (1 bit) - 0x%x\n", i, j, crclen_escape);

          if ( number_of_concatenated_frame != 1) {
            temp = bs->GetBits(1);
            printf("      concatenate_flag[%i][%i] (1 bit) - 0x%x\n", i, j, temp);
          }
          uint32_t fec_type = bs->GetBits(2);
          printf("      fec_type[%i][%i] (2 bit) - 0x%x\n", i, j, fec_type);
          if( fec_type == 0) {
            temp = bs->GetBits(1);
            printf("      termination_switch[%i][%i] (1 bit) - 0x%x\n", i, j, temp);
          }
          if (interleave_type == 2) {
            temp = bs->GetBits(2);
            printf("      interleave_switch[%i][%i] (2 bit) - 0x%x\n", i, j, temp);
          }
          temp = bs->GetBits(1);
          printf("      class_optional (1 bit) - 0x%x\n", temp);

          if ( length_escape == 1 ) { /* ESC */
            temp = bs->GetBits(4);
            printf("      number_of_bits_for_length[%i][%i] (4 bit) - 0x%x\n", i, j, temp);
          } else {
            temp = bs->GetBits(16);
            printf("      class_length[%i][%i] (16 bit) - 0x%x\n", i, j, temp);
          }

          if ( rate_escape != 1 ) { /* not ESC */
            if(fec_type) {
              temp = bs->GetBits(7);
              printf("      class_rate[%i][%i] (7 bit) - 0x%x\n", i, j, temp);
            } else {
              temp = bs->GetBits(5);
              printf("      class_rate[%i][%i] (5 bit) - 0x%x\n", i, j, temp);
            }
          }

          if ( crclen_escape != 1 ) { /* not ESC */
            temp = bs->GetBits(5);
            printf("      class_crclen[%i][%i] - 0x%x\n", i, j, temp);
          }
        }
          
        temp = bs->GetBits(1);
        printf("    class_recorded_output (1 bit) - 0x%x\n", temp);
        if ( temp == 1 ) {
          for ( j = 0; j < number_of_class; j++ ) {
            printf("    class - %i\n", j);
            temp = bs->GetBits(6);
            printf("      class_output_order[%i][%i] (6 bit) - 0x%x\n", i, j, temp);
          }
        }
      }
      temp = bs->GetBits(1);
      printf("    header_protection (1 bit) - 0x%x\n", temp);
      if(temp == 1) {
        temp = bs->GetBits(5);
        printf("    header_rate (5 bit) - 0x%x\n", temp);
        temp = bs->GetBits(5);
        printf("    header_crclen (5 bit) - 0x%x\n", temp);
      }
      temp = bs->GetBits(7);
      printf("    rs_fec_capability (7 bit) - 0x%x\n", temp);
    }
    if ( epConfig == 3 ) {
      temp = bs->GetBits(1);
      printf("  directMapping (1 bit) - 0x%x\n", temp);
      if ( ! temp ) {
        /* tbd */
      }
    }
  }
  printf("End of AudioSpecificConfig\n");
}

void CheckStreamMuxConfig (CBitstream *bs) {
  uint32_t temp;

  printf("StreamMuxConfig\n");
  temp = bs->GetBits(1);
  printf("  audioMuxVersion (1 bit) - 0x%x\n", temp);
  if(temp == 0) {
    uint32_t streamCnt=0;
    uint32_t progSIndx[255];
    uint32_t laySIndx[255];
    uint32_t streamID[255][255];
    
    uint32_t allStreamsSameTimeFraming = bs->GetBits(1);
    printf("  allStreamsSameTimeFraming (1 bit) - 0x%x\n", allStreamsSameTimeFraming);

    uint32_t numSubFrames = bs->GetBits(6);
    printf("  numSubFrames (6 bit) - 0x%x\n", numSubFrames);
        
    uint32_t numProgram = bs->GetBits(4);
    printf("  numProgram (4 bit) - 0x%x\n", numProgram);

    for ( uint32_t prog = 0; prog <= numProgram; prog++ ) {
      uint32_t numLayer = bs->GetBits(3);
      printf("  numLayer (3 bit) - 0x%x\n", numLayer);

      for ( uint32_t lay = 0; lay <= numLayer; lay++ ) {
        progSIndx[streamCnt] = prog;
        laySIndx[streamCnt] = lay;
        streamID [ prog][ lay] = streamCnt++;
        if ( prog == 0 && lay == 0 ) {
          CheckAudioSpecificConfig(bs);
        } else {
          temp = bs->GetBits(1);
          printf("  useSameConfig (1 bit) - 0x%x\n", temp);
          if ( !temp )
            CheckAudioSpecificConfig(bs);
        }

        temp = bs->GetBits(3);
        printf("  frameLengthType[%i] (3 bit) - 0x%x\n", streamID[prog][lay], temp);
        if ( temp == 0 ) {
          temp = bs->GetBits(8);
          printf("  latmBufferFullness[%i] (8 bit) - 0x%x\n", streamID[prog][lay], temp);
          if ( !allStreamsSameTimeFraming ) {

            /*
            if ((AudioObjectType[lay]==6 || AudioObjectType[lay]== 20) &&
                (AudioObjectType[lay-1]==8 || AudioObjectType[lay-1]==24)) {
              temp = bs->GetBits(6);
              printf("  coreFrameOffset (6 bit) - 0x%x\n", temp);
            }*/
          }
        } else if( temp == 1 ) {
          temp = bs->GetBits(9);
          printf("  frameLength[%i] (9 bit) - 0x%x\n", streamID[prog][lay], temp);
        } else if ( temp == 4 ||
                    temp == 5 ||
                    temp == 3 ) {
          temp = bs->GetBits(6);
          printf("  CELPframeLengthTableIndex[%i] (6 bit) - 0x%x\n", streamID[prog][lay], temp);
        } else if ( temp == 6 ||
                    temp == 7 ) {
          temp = bs->GetBits(1);
          printf("  HVXCframeLengthTableIndex[%i] (1 bit) - 0x%x\n", streamID[prog][lay], temp);
        }
      }
    }
    temp = bs->GetBits(1);
    printf("  otherDataPresent (1 bit) - 0x%x\n", temp);
    if (temp) {
      uint32_t otherDataLenBits = 0; /* helper variable 32bit */
      uint32_t otherDataLenEsc, otherDataLenTmp;
      do {
        otherDataLenBits = otherDataLenBits * 2^8;
        otherDataLenEsc = bs->GetBits(1);
        printf("  otherDataLenEsc (1 bit) - 0x%x\n", otherDataLenEsc);
        otherDataLenTmp = bs->GetBits(8);
        printf("  otherDataLenTmp (8 bit) - 0x%x\n", otherDataLenTmp);
        otherDataLenBits = otherDataLenBits + otherDataLenTmp;
      } while (otherDataLenEsc);
    }
    temp = bs->GetBits(1);
    printf("  crcCheckPresent (1 bit) - 0x%x\n", temp);
    if( temp ) {
      temp = bs->GetBits(8);
      printf("  crcCheckSum (8 bit) - 0x%x\n", temp);
    }
  } else {
    /* tbd */
  }
  printf("End of StreamMuxConfig\n");
}

void decode_audio (uint8_t *vol, uint32_t len) {
  uint32_t ix;

#if 1
  for (ix = 0; ix < len; ix++) {
    printf("%02x ", vol[ix]);
  }
  printf("\n");
#endif

  CBitstream *bs = new CBitstream(vol, len * 8);
  //bs->set_verbose(1);
  try {
    CheckStreamMuxConfig(bs);
		int bits_remain = bs->bits_remain();
		if(bits_remain >0) {
      printf("%i bits remain\n", bits_remain);
		}
  } catch (...) {
    fprintf(stderr, "bitstream read error\n");
  }
  return;
}

