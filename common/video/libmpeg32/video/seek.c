#include "../mpeg3private.h"
#include "../mpeg3protos.h"
#include "mpeg3video.h"
#include <stdlib.h>
#include <string.h>

unsigned int mpeg3bits_next_startcode(mpeg3_bits_t* stream)
{
  /* Perform forwards search */
  mpeg3bits_byte_align(stream);


//printf("mpeg3bits_next_startcode 1 %lld %lld\n", 
//	stream->demuxer->titles[0]->fs->current_byte, 
//	stream->demuxer->titles[0]->fs->total_bytes);


//mpeg3_read_next_packet(stream->demuxer);
//printf("mpeg3bits_next_startcode 2 %d %d\n", 
//	stream->demuxer->titles[0]->fs->current_byte, 
//	stream->demuxer->titles[0]->fs->total_bytes);

  if (mpeg3bits_eof(stream)) return 0xffffffff;

/* Perform search */
  while(1)
    {
      unsigned int code = mpeg3bits_showbits32_noptr(stream);

      //printf("mpeg3bits_next_startcode 2 %lx\n", code);
      if((code >> 8) == MPEG3_PACKET_START_CODE_PREFIX) break;
      if(mpeg3bits_eof(stream)) return 0xffffffff;


      mpeg3bits_getbyte_noptr(stream);

      /*
       * printf("mpeg3bits_next_startcode 3 %08x %d %d\n", 
       * 	mpeg3bits_showbits32_noptr(stream), 
       * 	stream->demuxer->titles[0]->fs->current_byte, 
       * 	stream->demuxer->titles[0]->fs->total_bytes);
       */
    }
  //printf("mpeg3bits_next_startcode 4 %d %d\n", 
  //	stream->demuxer->titles[0]->fs->current_byte, 
  //	stream->demuxer->titles[0]->fs->total_bytes);
  return mpeg3bits_showbits32_noptr(stream);
}
#if 0
/* Line up on the beginning of the next code. */
int mpeg3video_next_code(mpeg3_bits_t* stream, unsigned int code)
{
  while(!mpeg3bits_eof(stream) && 
	mpeg3bits_showbits32_noptr(stream) != code)
    {
      mpeg3bits_getbyte_noptr(stream);
    }
  return mpeg3bits_eof(stream);
}
#endif


long mpeg3video_goptimecode_to_frame(mpeg3video_t *video)
{
  /*  printf("mpeg3video_goptimecode_to_frame %d %d %d %d %f\n",  */
  /*  	video->gop_timecode.hour, video->gop_timecode.minute, video->gop_timecode.second, video->gop_timecode.frame, video->frame_rate); */
  return (long)(video->gop_timecode.hour * 3600 * video->frame_rate + 
		video->gop_timecode.minute * 60 * video->frame_rate +
		video->gop_timecode.second * video->frame_rate +
		video->gop_timecode.frame) - 1 - video->first_frame;
}

int mpeg3video_match_refframes(mpeg3video_t *video)
{
  unsigned char *dst, *src;
  int i, j, size;

  for(i = 0; i < 3; i++)
    {
      if(video->newframe[i])
	{
	  if(video->newframe[i] == video->refframe[i])
	    {
	      src = video->refframe[i];
	      dst = video->oldrefframe[i];
	    }
	  else
	    {
	      src = video->oldrefframe[i];
	      dst = video->refframe[i];
	    }

	  if(i == 0)
	    size = video->coded_picture_width * video->coded_picture_height + 32 * video->coded_picture_width;
	  else 
	    size = video->chrom_width * video->chrom_height + 32 * video->chrom_width;

	  memcpy(dst, src, size);
	}
    }
  return 0;
}

