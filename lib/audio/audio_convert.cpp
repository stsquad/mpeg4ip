#include  "audio_convert.h"
// Note - this is from a52dec.  It seems to be a fast way to
// convert floats to int16_t.  However, simply casting, then comparing
// would probably work as well
static inline int16_t convert_float (int32_t i)
{
  if (i > 0x43c07fff)
    return INT16_MAX;
  else if (i < 0x43bf8000)
    return INT16_MIN;
  else
    return i - 0x43c00000;
}

/*
 * convert signed 8 bit to signed 16 bit.  Basically, just shift
 */
static void audio_convert_s8_to_s16 (int16_t *to,
				     const uint8_t *from,
				     uint32_t samples)
{
  if (to == NULL) return;
  for (uint32_t ix = 0; ix < samples; ix++) {
    int16_t diff = (*from++ << 8);
    *to++ = diff;
  }
}


/*
 * convert unsigned 8 bit to signed 16.  Shift, then subtrack 0x7fff
 */
static void audio_convert_u8_to_s16 (int16_t *to,
				     const uint8_t *from,
				     uint32_t samples)
{
  if (to == NULL) return;
  for (uint32_t ix = 0; ix < samples; ix++) {
    int16_t diff = (*from++ << 8);
    diff -= INT16_MAX;
    *to++ = diff;
  }
}

/*
 * convert unsigned 16 bit to signed 16 bit - subtract 0x7fff
 */
static void audio_convert_u16_to_s16 (int16_t *to,
				      const uint16_t *from,
				      uint32_t samples)
{
  if (to == NULL) return;

  for (uint32_t ix = 0; ix < samples; ix++) {
    int32_t diff = *from++;
    diff -= INT16_MAX;
    *to++ = diff;
  }
}

static uint16_t swapit (uint16_t val)
{
#ifndef WORDS_BIGENDIAN
   return ntohs(val);
#else
    uint8_t *p = (uint8_t *)&val, temp;
    temp = p[0];
    p[0] = p[1];
    p[1] = temp;
    return val;
#endif
}
/*
 * audio_convert_to_sys - convert MSB or LSB codes to the native
 * format
 */
static void audio_convert_to_sys (uint16_t *conv, 
				  uint32_t samples)
{
  for (uint32_t ix = 0; ix < samples; ix++) {
    *conv = swapit(*conv);
    conv++;
  }
}

/*
 * audio_upconvert_chans - convert from a lower amount of channels to a 
 * larger amount.  It involves copying the channels, then 0'ing out the 
 * unused channels.
 */
void audio_upconvert_chans (uint8_t *to, 
			    const uint8_t *from,
			    uint32_t samples,
			    uint32_t src_chans,
			    uint32_t dst_chans)
{
  uint32_t ix;
  uint32_t src_bytes;
  uint32_t dst_bytes;
  uint32_t blank_bytes;
  uint32_t bytes_per_sample = sizeof(uint16_t);

  src_bytes = src_chans * bytes_per_sample;
  dst_bytes = dst_chans * bytes_per_sample;

  blank_bytes = dst_bytes - src_bytes;
  if (src_chans == 1) {
    blank_bytes -= src_bytes;
  }

  for (ix = 0; ix < samples; ix++) {
    memcpy(to, from, src_bytes);
    to += src_bytes;

    if (src_chans == 1) {
      memcpy(to, from, src_bytes);
      to += src_bytes;
    }
    memset(to, 0, blank_bytes);
    to += blank_bytes;

    from += src_bytes;
  }
}

/*
 * audio_downconvert_chans_remove_chans - convert from a larger 
 * amount to a smaller amount by just dropping the upper channels
 * We do this to drop the LFE, for the most part.
 */
static void audio_downconvert_chans_remove_chans (uint8_t *to, 
						  const uint8_t *from,
						  uint32_t samples,
						  uint32_t src_chans,
						  uint32_t dst_chans)
{
  uint32_t bytes_per_sample = sizeof(int16_t);
  uint32_t src_bytes;
  uint32_t dst_bytes;
  src_bytes = src_chans * bytes_per_sample;
  dst_bytes = dst_chans * bytes_per_sample;

  for (uint32_t ix = 0; ix < samples; ix++) {
    memcpy(to, from, dst_bytes);
    to += dst_bytes;
    from += src_bytes;
  }
}

/*
 * convert_s16 - cap the values at INT16_MAX and INT16_MIN for sums
 */
static inline int16_t convert_s16 (int32_t val)
{
  if (val > INT16_MAX) {
    return INT16_MAX;
  }
  if (val < INT16_MIN) {
    return INT16_MIN;
  }
  return val;
}

/*
 * audio_downconvert_chans_s16 - change a higher amount to a lower
 * amount
 */
void audio_downconvert_chans_s16 (int16_t *to,
				  const int16_t *from,
				  uint32_t src_chans,
				  uint32_t dst_chans,
				  uint32_t samples)
{
  uint32_t ix, jx;

  switch (dst_chans) {
  case 5:
  case 4:
    // we're doing 6 to 5 or 6 or 5 to 4 (5 to 4 should probably combine
    // the center with the left and right.
    audio_downconvert_chans_remove_chans((uint8_t *)to, 
					 (uint8_t *)from, 
					 samples,
					 src_chans, 
					 dst_chans);
    break;
  case 2:
    // downconvert to stereo
    for (ix = 0; ix < samples; ix++) {
      // we have 4, 5 or 6 chans
      int32_t l, r;
      l = from[0];
      l += from[2]; // add L, LR
      r = from[1];
      r = from[3];  // add R, RR
      if (src_chans > 4) {
	l += from[4]; // add center to both l and r
	r += from[4];
	l /= 3;
	r /= 3;
      } else {
	l /= 2; // no center
	r /= 2;
      }
      *to++ = convert_s16(l);
      *to++ = convert_s16(r);
      from += src_chans;
    }
    break;
  case 1: {
    // everything to mono - sum L, LR, R, RR and C, if they exist
    uint32_t add_chans;
    if (src_chans == 6) add_chans = 5;
    else add_chans = src_chans;
    for (ix = 0; ix < samples; ix++) {
      int32_t sum = 0;
      for (jx = 0; jx < add_chans; jx++) {
	sum += from[jx];
      }
      sum /= add_chans;
      *to++ = convert_s16(sum);
      from += src_chans;
    }
    break;
  }
  }
}


extern "C" void audio_convert_format(void *to_buffer,
				     const void *from_buffer,
				     uint32_t samples,
				     audio_format_t from_format,
				     uint32_t to_channels,
				     uint32_t from_channels)
{
  uint32_t ix;
  uint32_t src_chan_samples;
  bool convert_fmt;
  int16_t *format_buffer = NULL;

  if (to_buffer == NULL) {
    return;
  }

  switch (from_format) {
  case AUDIO_FMT_S16:
#ifdef WORDS_BIGENDIAN
  case AUDIO_FMT_S16MSB:
#else
  case AUDIO_FMT_S16LSB:
#endif
    convert_fmt = false;
    break;
  default:
    convert_fmt = true;
    break;
  }

  src_chan_samples = samples * from_channels;

  if (convert_fmt) {
    format_buffer = (int16_t *)malloc(samples * from_channels * sizeof(int16_t));
    // if we're here, convert everything to S16
    switch (from_format) {
    case AUDIO_FMT_FLOAT: {
      // convert float to S16 
      int32_t *ffrom = (int32_t *)from_buffer;
      int16_t *to;
      if (to_channels == from_channels) {
	to = (int16_t *)to_buffer;
      } else {
	to = (int16_t *)format_buffer;
      }
      
      for (ix = 0; ix < src_chan_samples; ix++) {
	*to++ = convert_float(ffrom[ix]);
      }
      if (to_channels == from_channels) {
      return;
      }
      from_buffer = format_buffer;
      break;
    }
    case AUDIO_FMT_U8:
      audio_convert_u8_to_s16(format_buffer, (uint8_t *)from_buffer, src_chan_samples);
      from_buffer = format_buffer;
      break;
    case AUDIO_FMT_S8:
      audio_convert_s8_to_s16(format_buffer, (uint8_t *)from_buffer, src_chan_samples);
      from_buffer = format_buffer;
      break;
    case AUDIO_FMT_U16MSB:
#ifndef WORDS_BIGENDIAN
      audio_convert_to_sys((uint16_t *)from_buffer, src_chan_samples);
#endif
      audio_convert_u16_to_s16(format_buffer, (uint16_t *)from_buffer, src_chan_samples);
      from_buffer = format_buffer;
      break;
    case AUDIO_FMT_S16MSB:
#ifndef WORDS_BIGENDIAN
      audio_convert_to_sys((uint16_t *)from_buffer, src_chan_samples);
#endif
      break;
  case AUDIO_FMT_U16LSB:
#ifdef WORDS_BIGENDIAN
    audio_convert_to_sys((uint16_t *)from_buffer, src_chan_samples);
#endif
    audio_convert_u16_to_s16(format_buffer, (uint16_t *)from_buffer, src_chan_samples);
    from_buffer = format_buffer;
    break;
    case AUDIO_FMT_S16LSB:
#ifdef WORDS_BIGENDIAN
      audio_convert_to_sys((uint16_t *)from_buffer, src_chan_samples);
#endif
      break;
    case AUDIO_FMT_U16:
      audio_convert_u16_to_s16(format_buffer, (uint16_t *)from_buffer, src_chan_samples);
      from_buffer = format_buffer;
      break;
    case AUDIO_FMT_S16:
      break;
    case AUDIO_FMT_HW_AC3:
      abort();
    }
  }

  // at this point - from_buffer points to a buffer of all S16, system based
  // ordering.  We need to downconvert channels
  if (from_channels == to_channels) {
    memcpy(to_buffer, from_buffer, src_chan_samples * sizeof(int16_t));
  } else if (from_channels > to_channels) {
    audio_downconvert_chans_s16((int16_t *)to_buffer, 
				(int16_t *)from_buffer, 
				from_channels, 
				to_channels,
				samples);
  } else {
    
  audio_upconvert_chans((uint8_t *)to_buffer, 
			(uint8_t *)from_buffer, 
			samples, 
			from_channels, 
			to_channels);
  }
  CHECK_AND_FREE(format_buffer);
}
