#ifdef PLUGIN

#include <windows.h>
#include <stdio.h>
#include "filestream.h"

#include "in2.h"
#include "faad.h"
#include "all.h"

// post this to the main window at end of file (after playback as stopped)
#define WM_WA_AAC_EOF WM_USER+2

// raw configuration
#define NCH 2
#define SAMPLERATE 44100
#define BPS 16

faadAACInfo fInfo;

In_Module mod; // the output module (declared near the bottom of this file)
char lastfn[1024]; // currently playing file (used for getting info on the current file)
int file_length; // file length, in bytes
int decode_pos_ms; // current decoding position, in milliseconds
int paused; // are we paused?
int seek_needed; // if != -1, it is the point that the decode thread should seek to, in ms.
char sample_buffer[1024*NCH*(BPS/8)]; // sample buffer
int semephore=0;
int headerDecoded = 0;
extern int stop_now;
int killPlayThread=0;					// the kill switch for the decode thread
HANDLE play_thread_handle=INVALID_HANDLE_VALUE;	// the handle to the decode thread
DWORD WINAPI PlayThread(void *b); // the decode thread procedure

void config(HWND hwndParent)
{
	MessageBox(hwndParent,
		"AAC files without an ADIF or ADTS header must be 44100kHz\n"
		"and must be encoded using MAIN or LOW profile.\n"
		"When an ADIF or ADTS header is present, configuration\n"
		"will be done automatically.",
		"Configuration",MB_OK);
}
void about(HWND hwndParent)
{
	MessageBox(hwndParent,"Freeware AAC Player v0.6\nhttp://www.audiocoding.com","About AAC Player",MB_OK);
}

void init() { }

void quit() { }

int isourfile(char *fn)
{
	return 0;
} 

int play(char *fn) 
{ 
	int maxlatency;
	int thread_id;
    int nchannels;

	stop_now = 0;
    semephore = 1;
    headerDecoded = 0;

	if(strncmp(fn, "http://", 7) == 0)
		mod.is_seekable = 0;
	else
		mod.is_seekable = 1;

    if(aac_decode_init(&fInfo, fn))
	{
        semephore = 0;
		return -1;
    }

    /// We need to decode one frame and rewind the file...
    /// since it after one frame decoded we know the number of channels
	aac_decode_frame((short*)sample_buffer);
    nchannels = mc_info.nch; //fInfo.channels is not known until after decoding one frame.
    aac_decode_free();

	aac_decode_init(&fInfo, fn);
    fInfo.channels = nchannels;
    headerDecoded = 1;
	strcpy(lastfn,fn);
	paused=0;
	decode_pos_ms=0;
	seek_needed=-1;

	maxlatency = mod.outMod->Open(fInfo.sampling_rate,fInfo.channels, BPS, -1,-1);
	
	if (maxlatency < 0) // error opening device
	{
        semephore = 0;
		return -1;
	}

	// initialize vis stuff
	mod.SAVSAInit(maxlatency, fInfo.sampling_rate);
	mod.VSASetInfo(fInfo.sampling_rate, fInfo.channels);
	
	mod.SetInfo(fInfo.bit_rate/1000, fInfo.sampling_rate/1000, fInfo.channels,1);			

	mod.outMod->SetVolume(-666); // set the output plug-ins default volume

	killPlayThread = 0;
	play_thread_handle = (HANDLE) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE) PlayThread,(void *) &killPlayThread,0,&thread_id);
	return 0; 
}

void pause() { paused=1; mod.outMod->Pause(1); }
void unpause() { paused=0; mod.outMod->Pause(0); }
int ispaused() { return paused; }

void stop()
{
	if (play_thread_handle != INVALID_HANDLE_VALUE)
	{
		killPlayThread=1;
		Sleep(10);

		CloseHandle(play_thread_handle);
		play_thread_handle = INVALID_HANDLE_VALUE;
	}

    semephore = 0;
	aac_decode_free();
	mod.outMod->Close();
	mod.SAVSADeInit();
}

int getlength()
{
	return fInfo.length;
}

int getoutputtime()
{ 
	return decode_pos_ms+(mod.outMod->GetOutputTime()-mod.outMod->GetWrittenTime()); 
}

void setoutputtime(int time_in_ms)
{
	if (adts_header_present)
		seek_needed=time_in_ms;
	else
		seek_needed = -1;
}

void setvolume(int volume) { mod.outMod->SetVolume(volume); }
void setpan(int pan) { mod.outMod->SetPan(pan); }

unsigned long bitbuf;
unsigned int subbitbuf;
int bitcount;
#define CHAR_BIT 8
#define BITBUFSIZ (CHAR_BIT * sizeof(bitbuf))


void fillbuf(int n, FILE_STREAM *file)
{
	bitbuf <<= n;

	while (n > bitcount)
	{
		bitbuf |= subbitbuf << (n -= bitcount);
		subbitbuf = (unsigned char) read_byte_filestream(file);
		bitcount = CHAR_BIT;
	}

	bitbuf |= subbitbuf >> (bitcount -= n);
}

unsigned int TMPgetbits(int n, FILE_STREAM *file)
{
	unsigned int x;

	x = bitbuf >> (BITBUFSIZ - n);
	fillbuf(n, file);
	return x;
}

void init_getbits(FILE_STREAM *file)
{
	bitbuf = 0;  subbitbuf = 0;  bitcount = 0;
	fillbuf(BITBUFSIZ, file);
}

int infoDlg(char *fn, HWND hwnd)
{
	char Info[500];
	char *strProfile;
	char adxx_id[5];
	FILE_STREAM *file;
	int i;
	int unused,nchannels;
	int bitstream, bitrate, profile, sr_idx, srate;
	static int SampleRates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000};

    //To get the number of channels we need to decode one channel...
    if(semephore == 1) {
        while(!headerDecoded)
            Sleep(500);
        nchannels = fInfo.channels;
        srate = fInfo.sampling_rate;
	    bitrate = fInfo.bit_rate;
        wsprintf(Info,
			"%s\n\n"
			"AAC info:\n"
			"Bitrate: %dbps\n"
			"Sampling rate: %dHz\n"
			"Channels: %d\n",
			fn, bitrate, srate, nchannels);
		MessageBox(hwnd, Info, "AAC file info box",MB_OK);
        return 0;
    } else {
        if(aac_decode_init(&fInfo, fn))
		    return -1;
	    aac_decode_frame((short*)sample_buffer);
        nchannels = mc_info.nch;
        aac_decode_free();

	    if ((file = open_filestream(fn)) == NULL)
		    return 1;

		read_buffer_filestream(file, adxx_id, 4);

		/* rageomatic: Filestream hack :D */
		file->buffer_offset = 0;

	    adxx_id[5-1] = 0;

	    if (strncmp(adxx_id, "ADIF", 4) == 0)
		{
			/* ADIF header */
		    init_getbits(file);

		    /* copyright string */	
		    if ((TMPgetbits(LEN_COPYRT_PRES, file)) == 1)
			{
			    for (i=0; i<LEN_COPYRT_ID; i++)
				    unused = TMPgetbits(LEN_BYTE, file); 
		    }

		    unused = TMPgetbits(LEN_ORIG, file);
		    unused = TMPgetbits(LEN_HOME, file);
		    bitstream = TMPgetbits(LEN_BS_TYPE, file);
		    bitrate = TMPgetbits(LEN_BIT_RATE, file);

		    /* program config elements */ 
		    TMPgetbits(LEN_NUM_PCE, file);
		    unused = (bitstream == 0) ? TMPgetbits(LEN_ADIF_BF, file) : 0;
		    TMPgetbits(LEN_TAG, file);

		    profile = TMPgetbits(LEN_PROFILE, file);
		    sr_idx = TMPgetbits(LEN_SAMP_IDX, file);
		    if (profile == Main_Profile)
			    strProfile = "Main";
		    else if (profile == LC_Profile)
			    strProfile = "Low Complexity";
		    else if (profile == 2 /* SSR */)
			    strProfile = "SSR (unsupported)";

		    srate = SampleRates[sr_idx];

		    wsprintf(Info,
			    "%s\n\n"
			    "ADIF header info:\n"
			    "Profile: %s\n"
			    "Bitrate: %dbps\n"
			    "Sampling rate: %dHz\n"
			    "Channels: %d\n",
			    fn, strProfile, bitrate, srate, nchannels);
		    MessageBox(hwnd, Info, "AAC file info box",MB_OK);
	    }
		else if (((int) ( adxx_id[0] == (char) 0xFF)) &&
		    ((int) ( (adxx_id[1] & (char) 0xF0) == (char) 0xF0)))
		{
			/* ADTS header */
			/* rageomatic: Filestream hack ...again... */
			file->buffer_offset = 0;//fseek(file, 0, SEEK_SET);
		    init_getbits(file);
		    unused = TMPgetbits(12, file); // 12 bit SYNCWORD
		    unused = TMPgetbits(1, file);
		    unused = TMPgetbits(2, file);
		    unused = TMPgetbits(1, file);
		    profile = TMPgetbits(2, file);
		    sr_idx = TMPgetbits(4, file);

		    if (profile == Main_Profile)
			    strProfile = "Main";
		    else if (profile == LC_Profile)
			    strProfile = "Low Complexity";
		    else if (profile == 2 /* SSR */)
			    strProfile = "SSR (unsupported)";

		    srate = SampleRates[sr_idx];

		    wsprintf(Info,
			    "%s\n\n"
			    "ADTS header info:\n"
			    "Profile: %s\n"
    //			"Bitrate: %dbps\n"
			    "Sampling rate: %dHz\n"
			    "Channels: %d\n",
			    fn, strProfile, /*bitrate,*/ srate, nchannels);
		    MessageBox(hwnd, Info, "AAC file info box",MB_OK);
	    } 
		else
		{
		    wsprintf(Info,
			    "%s\n\n"
			    "Headerless AAC file\n"
			    "Assumed configuration:\n"
			    "Profile: Main\n"
			    "Bitrate: 128kbps\n"
			    "Sampling rate: 44100Hz\n"
			    "Channels: %d\n",
			    fn,nchannels);
		    MessageBox(hwnd, Info, "AAC file info box",MB_OK);
	    }

	    close_filestream(file);

	    return 0;
    }
}

LPTSTR PathFindFileName(LPCTSTR pPath)
{
    LPCTSTR pT;

    for (pT = pPath; *pPath; pPath = CharNext(pPath)) {
        if ((pPath[0] == TEXT('\\') || pPath[0] == TEXT(':')) && pPath[1] && (pPath[1] != TEXT('\\')))
            pT = pPath + 1;
    }

    return (LPTSTR)pT;   // const -> non const
}

int getbitrate(char *fn)
{
	FILE_STREAM *file;
	char adif_id[5];
	int i, unused, bitrate;

	if ((file = open_filestream(fn)) == NULL)
		return 1;

	read_buffer_filestream(file, adif_id, 4);
	adif_id[5-1] = 0;

	if (strncmp(adif_id, "ADIF", 4) == 0)
	{
		init_getbits(file);

		/* copyright string */	
		if ((TMPgetbits(LEN_COPYRT_PRES, file)) == 1) {
			for (i=0; i<LEN_COPYRT_ID; i++)
				unused = TMPgetbits(LEN_BYTE, file); 
		}
		unused = TMPgetbits(LEN_ORIG, file);
		unused = TMPgetbits(LEN_HOME, file);
		unused = TMPgetbits(LEN_BS_TYPE, file);
		bitrate = TMPgetbits(LEN_BIT_RATE, file);
	}
	else
		bitrate = 128000;

	close_filestream(file);

	return bitrate;
}

void getfileinfo(char *filename, char *title, int *length_in_ms)
{
	if (!filename || !*filename)  // currently playing file
	{
		if (length_in_ms) *length_in_ms=getlength();
		if (title) 
		{
			char *p=lastfn+strlen(lastfn);
			while (*p != '\\' && p >= lastfn) p--;
			strcpy(title,++p);
			if (title[lstrlen(title)-4] == '.')
				title[lstrlen(title)-4] = '\0';
		}
	}
	else // some other file
	{
		if (length_in_ms)
		{
			FILE_STREAM *hFile;
			*length_in_ms=-1000;

			hFile = open_filestream(filename);

			if (hFile != NULL)
			{
				int bitrate = getbitrate(filename);
				*length_in_ms = (filelength_filestream(hFile)/(((bitrate*8)/1000)*BPS))*1000;
			}

			close_filestream(hFile);
		}
		if (title) 
		{
			char *p=filename+strlen(filename);
			while (*p != '\\' && p >= filename) p--;
			strcpy(title,++p);
			if (title[lstrlen(title)-4] == '.')
				title[lstrlen(title)-4] = '\0';
		}
	}
}

void eq_set(int on, char data[10], int preamp) 
{ 
}

DWORD WINAPI PlayThread(void *b)
{
	int done=0;
	int framebits = 0;
	int tmp_bits;
	int framecount = 0;
	int frame = 0;
	int last_frame = 0;
    int l = 1024*2*fInfo.channels;

	while (! *((int *)b) ) 
	{
		if (seek_needed != -1 && adts_header_present != 0)
		{
			int seconds;

			// Round off to a second
			seconds = seek_needed - (seek_needed%1000);
			mod.outMod->Flush(decode_pos_ms);
			seek_needed = -1;
			done=0;
			aac_decode_seek(seconds);
			decode_pos_ms = seconds;
		}

		if (done)
		{
			mod.outMod->CanWrite();
			if (!mod.outMod->IsPlaying())
			{
				PostMessage(mod.hMainWindow,WM_WA_AAC_EOF,0,0);
                semephore = 0;
                headerDecoded = 0;
				return 0;
			}
			Sleep(10);
		}
        //assume that max channels is 2.
		else if (mod.outMod->CanWrite() >= ((1024*fInfo.channels*2)<<(mod.dsp_isactive()?1:0)))
		{
			if (last_frame) 
			{
				done=1;
			}
			else
			{
				tmp_bits = aac_decode_frame((short*)sample_buffer);

				if (tmp_bits < 0)
					last_frame = 1;

				framebits += tmp_bits;
				framecount++;

				/*
				if (framecount==(int)(fInfo.sampling_rate/8192))
				{
				mod.SetInfo((int)(((framebits*8)/1000)*1.08+0.5),fInfo.sampling_rate/1000,fInfo.channels,1);
					
					framecount = 0;
					framebits = 0;
				}
				*/

				if ((frame > 0)&&(!killPlayThread)&&(!stop_now))
				{
					mod.SAAddPCMData(sample_buffer,fInfo.channels,BPS,decode_pos_ms);
					mod.VSAAddPCMData(sample_buffer,fInfo.channels,BPS,decode_pos_ms);
					decode_pos_ms+=(1024*1000)/fInfo.sampling_rate;
					if (mod.dsp_isactive()) 
                        l=mod.dsp_dosamples((short *)sample_buffer,l/fInfo.channels/(BPS/8),BPS,fInfo.channels,fInfo.sampling_rate)*(fInfo.channels*(BPS/8));
					mod.outMod->Write(sample_buffer,l);
				}
				frame++;
			}
		}
		else Sleep(10);
	}

    semephore =0;
    headerDecoded = 0;
	return 0;
}

void CommonWarning(char *message)
{
	MessageBox(mod.hMainWindow, message, "Warning...",MB_OK);
}

void CommonExit(int errorcode, char *message)
{
	MessageBox(mod.hMainWindow, message, "Error...", MB_OK);
	killPlayThread = 1;
}

In_Module mod = 
{
	IN_VER,
	"Freeware AAC Decoder v0.5",
	0,	// hMainWindow
	0,  // hDllInstance
	"AAC\0AAC File (*.AAC)\0"
	,
	1, // is_seekable
	1, // uses output
	config,
	about,
	init,
	quit,
	getfileinfo,
	infoDlg,
	isourfile,
	play,
	pause,
	unpause,
	ispaused,
	stop,
	
	getlength,
	getoutputtime,
	setoutputtime,

	setvolume,
	setpan,

	0,0,0,0,0,0,0,0,0, // vis stuff


	0,0, // dsp

	eq_set,

	NULL,		// setinfo

	0 // out_mod
};

__declspec( dllexport ) In_Module * winampGetInModule2()
{
	return &mod;
}

#endif // PLUGIN
