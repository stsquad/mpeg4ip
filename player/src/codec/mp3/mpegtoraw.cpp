/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Mpegtoraw.cc
// Server which get mpeg format and put raw format.

#include "mpeg4ip.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef DEBUG_AUDIO
#include <stdio.h>
#endif

#include "MPEGaudio.h"
#if 0
#include "MPEGstream.h"
#endif

#if defined(_WIN32)
#include <windows.h>
#endif

#define MY_PI 3.14159265358979323846

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define _KEY 0
#else
#define _KEY 3
#endif

int MPEGaudio::getbits( int bits )
{
    union
    {
        char store[4];
        int current;
    } u;
    int bi;

    if( ! bits )
        return 0;

    u.current = 0;
    bi = (bitindex & 7);
    u.store[ _KEY ] = _buffer[ bitindex >> 3 ] << bi;
    bi = 8 - bi;
    bitindex += bi;

    while( bits )
    {
        if( ! bi )
        {
            u.store[ _KEY ] = _buffer[ bitindex >> 3 ];
            bitindex += 8;
            bi = 8;
        }

        if( bits >= bi )
        {
            u.current <<= bi;
            bits -= bi;
            bi = 0;
        }
        else
        {
            u.current <<= bits;
            bi -= bits;
            bits = 0;
        }
    }
    bitindex -= bi;

    return( u.current >> 8 );
}


// Convert mpeg to raw
// Mpeg headder class
void MPEGaudio::initialize()
{
  static bool initialized = false;

  register int i;
  register REAL *s1,*s2;
  REAL *s3,*s4;

  forcetomonoflag = false;
  forcetostereoflag = false;
  downfrequency = 0;

  scalefactor=SCALE;
  calcbufferoffset=15;
  currentcalcbuffer=0;

  s1 = calcbufferL[0];
  s2 = calcbufferR[0];
  s3 = calcbufferL[1];
  s4 = calcbufferR[1];
  for(i=CALCBUFFERSIZE-1;i>=0;i--)
  {
    calcbufferL[0][i]=calcbufferL[1][i]=
    calcbufferR[0][i]=calcbufferR[1][i]=0.0;
  }

  if( ! initialized )
  {
    for(i=0;i<16;i++) hcos_64[i] = (float)
			(1.0/(2.0*cos(MY_PI*double(i*2+1)/64.0)));
    for(i=0;i< 8;i++) hcos_32[i] = (float)
			(1.0/(2.0*cos(MY_PI*double(i*2+1)/32.0)));
    for(i=0;i< 4;i++) hcos_16[i] = (float)
			(1.0/(2.0*cos(MY_PI*double(i*2+1)/16.0)));
    for(i=0;i< 2;i++) hcos_8 [i] = (float)
			(1.0/(2.0*cos(MY_PI*double(i*2+1)/ 8.0)));
    hcos_4 = (float)(1.0f / (2.0f * cos( MY_PI * 1.0 / 4.0 )));
    initialized = true;
  }

  layer3initialize();

#if WMAY_OUT
#ifdef THREADED_AUDIO
  decode_thread = NULL;
  ring = NULL;
#endif
  Rewind();
  ResetSynchro(0);
#endif
};

bool MPEGaudio::loadheader(void)
{
    register unsigned char c;
    bool flag;
    int sampling_freq;

    flag = false;
    do
    {
		if (fillbuffer(4) == false)
			return false;
        c = _buffer[0];
		
		_buffer++;
		_buflen--;

        if( c == 0xff )
        {
            while( ! flag )
            {
				c = _buffer[0];
				_buflen--;
				_buffer++;
                if( (c & 0xe0) == 0xe0 )
                {
                    flag = true;
                    break;
                }
                else if( c != 0xff )
                {
					return false;
                } 
            }
        } else {
			return false;
		}
    } while( ! flag );



    // Analyzing

    if ((c & 0x10) == 0)
      _mpeg25 = true;
    else 
      _mpeg25 = false;
    c &= 0xf;
    protection = c & 1;
    layer = 4 - ((c >> 1) & 3);
    if (_mpeg25 == false)
      version = (_mpegversion) ((c >> 3) ^ 1);
    else 
      version = mpeg2;

#if 0
    c = mpeg->copy_byte() >> 1;
#else
    c = _buffer[0] >> 1;
	_buffer++;
	_buflen--;
#endif
    padding = (c & 1);
    c >>= 1;
    frequency = (_frequency) (c&3);
    if (frequency == 3)
        return false;
    c >>= 2;
    bitrateindex = (int) c;
    if( bitrateindex == 15 )
        return false;
    sampling_freq = frequency + version * 3;
    if (_mpeg25) sampling_freq += 3;

#if 0
    c = ((unsigned int)mpeg->copy_byte()) >> 4;
#else
    c = _buffer[0] >> 4;
	_buffer++;
	_buflen--;
#endif
    extendedmode = c & 3;
    mode = (_mode) (c >> 2);


    // Making information

    inputstereo = (mode == single) ? 0 : 1;
#if 0
    forcetomonoflag = (!stereo && inputstereo);
    forcetostereoflag = (stereo && !inputstereo);
#else
    forcetomonoflag = false;
    forcetostereoflag = false;
#endif

    if(forcetomonoflag)
        outputstereo=0;
    else
        outputstereo=inputstereo;

    channelbitrate=bitrateindex;
    if(inputstereo)
    {
        if(channelbitrate==4)
            channelbitrate=1;
        else
            channelbitrate-=4;
    }

    if(channelbitrate==1 || channelbitrate==2)
        tableindex=0;
    else
        tableindex=1;

  if(layer==1)
      subbandnumber=MAXSUBBAND;
  else
  {
    if(!tableindex)
      if(frequency==frequency32000)subbandnumber=12; else subbandnumber=8;
    else if(frequency==frequency48000||
        (channelbitrate>=3 && channelbitrate<=5))
      subbandnumber=27;
    else subbandnumber=30;
  }

  if(mode==single)stereobound=0;
  else if(mode==joint)stereobound=(extendedmode+1)<<2;
  else stereobound=subbandnumber;

  if(stereobound>subbandnumber)stereobound=subbandnumber;

  // framesize & slots
  if(layer==1)
  {
    framesize=(12000*bitrate[version][0][bitrateindex])/
              frequencies[sampling_freq];
    if(frequency==frequency44100 && padding)framesize++;
    framesize<<=2;
  }
  else
  {
    framesize=(144000*bitrate[version][layer-1][bitrateindex])/
      (frequencies[sampling_freq]<<version);
    if(padding)framesize++;
    if(layer==3)
    {
      if(version)
    layer3slots=framesize-((mode==single)?9:17)
                         -(protection?0:2)
                         -4;
      else
    layer3slots=framesize-((mode==single)?17:32)
                         -(protection?0:2)
                         -4;
    }
  }

#ifdef DEBUG_AUDIO
  fprintf(stderr, "MPEG %d audio layer %d (%d kbps), at %d Hz %s [%d]\n", version+1, layer,  bitrate[version][layer-1][bitrateindex], frequencies[sampling_freq], (mode == single) ? "mono" : "stereo", framesize);
#endif


  return true;
}

#if 0
bool MPEGaudio::run( int frames, double *timestamp)
{
    double last_timestamp = -1;
    int totFrames = frames;

    for( ; frames; frames-- )
    {
        if( loadheader() == false ) {
	  return false;	  
        }

        if (frames == totFrames  && timestamp != NULL)
            if (last_timestamp != mpeg->timestamp){
		if (mpeg->timestamp_pos <= _buffer_pos)
		    last_timestamp = *timestamp = mpeg->timestamp;
	    }
            else
                *timestamp = -1;

        if     ( layer == 3 ) extractlayer3();
        else if( layer == 2 ) extractlayer2();
        else if( layer == 1 ) extractlayer1();

        /* Handle expanding to stereo output */
        if ( forcetostereoflag ) {
            Sint16 *in, *out;

            in = rawdata+rawdatawriteoffset;
            rawdatawriteoffset *= 2;
            out = rawdata+rawdatawriteoffset;
            while ( in > rawdata ) {
                --in;
                *(--out) = *in;
                *(--out) = *in;
            }
        }

        ++decodedframe;
#ifndef THREADED_AUDIO
        ++currentframe;
#endif
    }

    return(true);
}

#ifdef THREADED_AUDIO
int Decode_MPEGaudio(void *udata)
{
    MPEGaudio *audio = (MPEGaudio *)udata;
    double timestamp;

#if defined(_WIN32)
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#endif

    while ( audio->decoding && ! audio->mpeg->eof() ) {
        audio->rawdata = (Sint16 *)audio->ring->NextWriteBuffer();

        if ( audio->rawdata ) {
            audio->rawdatawriteoffset = 0;
            audio->run(1, &timestamp);

	    if((Uint32)audio->rawdatawriteoffset*2 <= audio->ring->BufferSize())
	      audio->ring->WriteDone(audio->rawdatawriteoffset*2, timestamp);
        }
    }

    audio->decoding = false;
    audio->decode_thread = NULL;
    return(0);
}
#endif /* THREADED_AUDIO */

// Helper function for SDL audio
void Play_MPEGaudio(void *udata, Uint8 *stream, int len)
{
    MPEGaudio *audio = (MPEGaudio *)udata;
    int volume;
    long copylen;

    /* Bail if audio isn't playing */
    if ( audio->Status() != MPEG_PLAYING ) {
        return;
    }
    volume = audio->volume;

    /* Increment the current play time (assuming fixed frag size) */
    switch (audio->frags_playing++) {
      // Vivien: Well... the theorical way seems good to me :-)
        case 0:        /* The first audio buffer is being filled */
            break;
        case 1:        /* The first audio buffer is starting playback */
            audio->frag_time = SDL_GetTicks();
            break;
        default:    /* A buffer has completed, filling a new one */
            audio->frag_time = SDL_GetTicks();
            audio->play_time += ((double)len)/audio->rate_in_s;
            break;
    }

    /* Copy the audio data to output */
#ifdef THREADED_AUDIO
    Uint8 *rbuf;
    assert(audio);
    assert(audio->ring);
    do {
	/* this is empirical, I don't realy know how to find out when
	   a certain piece of audio has finished playing or even if
	   the timestamps refer to the time when the frame starts
	   playing or then the frame ends playing, but as is works
	   quite right */
        copylen = audio->ring->NextReadBuffer(&rbuf);
        if ( copylen > len ) {
            SDL_MixAudio(stream, rbuf, len, volume);
            audio->ring->ReadSome(len);
            len = 0;
	    for (int i=0; i < N_TIMESTAMPS -1; i++)
		audio->timestamp[i] = audio->timestamp[i+1];
	    audio->timestamp[N_TIMESTAMPS-1] = audio->ring->ReadTimeStamp();
        } else {
            SDL_MixAudio(stream, rbuf, copylen, volume);
            ++audio->currentframe;
            audio->ring->ReadDone();
//fprintf(stderr, "-");
            len -= copylen;
            stream += copylen;
        }
	if (audio->timestamp[0] != -1){
	    double timeshift = audio->Time() - audio->timestamp[0];
	    double correction = 0;
	    assert(audio->timestamp >= 0);
	    if (fabs(timeshift) > 1.0){
	        correction = -timeshift;
#ifdef DEBUG_TIMESTAMP_SYNC
                fprintf(stderr, "audio jump %f\n", timeshift);
#endif
            } else
	        correction = -timeshift/100;
#ifdef USE_TIMESTAMP_SYNC
	    audio->play_time += correction;
#endif
#ifdef DEBUG_TIMESTAMP_SYNC
	    fprintf(stderr, "\raudio: time:%8.3f shift:%8.4f",
                    audio->Time(), timeshift);
#endif
	    audio->timestamp[0] = -1;
	}
    } while ( copylen && (len > 0) && ((audio->currentframe < audio->decodedframe) || audio->decoding));
#else
    /* The length is interpreted as being in samples */
    len /= 2;

    /* Copy in any saved data */
    if ( audio->rawdatawriteoffset > 0 ) {
        copylen = (audio->rawdatawriteoffset-audio->rawdatareadoffset);
        assert(copylen >= 0);
        if ( copylen >= len ) {
            SDL_MixAudio(stream, (Uint8 *)&audio->spillover[audio->rawdatareadoffset],
                                                       len*2, volume);
            audio->rawdatareadoffset += len;
            return;
        }
        SDL_MixAudio(stream, (Uint8 *)&audio->spillover[audio->rawdatareadoffset],
                                                       copylen*2, volume);
        len -= copylen;
        stream += copylen*2;
    }

    /* Copy in any new data */
    audio->rawdata = (Sint16 *)stream;
    audio->rawdatawriteoffset = 0;
    audio->run(len/audio->samplesperframe);
    len -= audio->rawdatawriteoffset;
    stream += audio->rawdatawriteoffset*2;

    /* Write a save buffer for remainder */
    audio->rawdata = audio->spillover;
    audio->rawdatawriteoffset = 0;
    if ( audio->run(1) ) {
        assert(audio->rawdatawriteoffset > len);
        SDL_MixAudio(stream, (Uint8 *) audio->spillover, len*2, volume);
        audio->rawdatareadoffset = len;
    } else {
        audio->rawdatareadoffset = 0;
    }
#endif
}
#endif
// EOF
