/* TBD HEADER */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <avilib.h>
#include <math.h>
#include <mpeg4ip.h>


/* globals */
char* progName;

/*
 * avi2raw
 * required arg1 should be the input AVI file
 * required arg2 should be the output RAW file
 */ 
int main(int argc, char** argv)
{
	/* configurable variables from command line */
	bool extractVideo = TRUE;	/* FALSE implies extract audio */
	u_int32_t start = 0;		/* secs, start offset */
	u_int32_t duration = 0;		/* secs, 0 implies entire file */

	/* internal variables */
	char* aviFileName = NULL;
	char* rawFileName = NULL;
	avi_t* aviFile = NULL;
	FILE* rawFile = NULL;
	u_int32_t numBytes;

	/* begin process command line */
	progName = argv[0];
	while (1) {
		int c = -1;
		int option_index = 0;
		static struct option long_options[] = {
			{ "audio", 0, 0, 'a' },
			{ "length", 1, 0, 'l' },
			{ "start", 1, 0, 's' },
			{ "video", 0, 0, 'v' },
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "al:s:v",
			long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 'a': {
			extractVideo = FALSE;
			break;
		}
		case 'l': {
			/* --length=<secs> */
			u_int i;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr, 
					"%s: bad length specified: %s\n",
					 progName, optarg);
			} else {
				duration = i;
			}
			break;
		}
		case 's': {
			/* --start=<secs> */
			u_int i;
			if (sscanf(optarg, "%u", &i) < 1) {
				fprintf(stderr, 
					"%s: bad start specified: %s\n",
					 progName, optarg);
			} else {
				start = i;
			}
			break;
		}
		case 'v': {
			extractVideo = TRUE;
			break;
		}
		case '?':
			break;
		default:
			fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
				progName, c);
		}
	}

	/* check that we have at least two non-option arguments */
	if ((argc - optind) < 2) {
		fprintf(stderr, 
			"usage: %s <avi-file> <raw-file>\n",
			progName);
		exit(1);
	}

	/* point to the specified file names */
	aviFileName = argv[optind++];
	rawFileName = argv[optind++];

	/* warn about extraneous non-option arguments */
	if (optind < argc) {
		fprintf(stderr, "%s: unknown options specified, ignoring: ");
		while (optind < argc) {
			fprintf(stderr, "%s ", argv[optind++]);
		}
		fprintf(stderr, "\n");
	}

	/* end processing of command line */

	/* open the AVI file */
	aviFile = AVI_open_input_file(aviFileName, TRUE);
	if (aviFile == NULL) {
		fprintf(stderr, 
			"%s: error %s: %s\n",
			progName, aviFileName, AVI_strerror());
		exit(4);
	}

	/* open the RAW file */
	rawFile = fopen(rawFileName, "w");
	if (rawFile == NULL) {
		fprintf(stderr,
			"%s: error opening %s: %s\n",
			progName, rawFileName, strerror(errno));
		exit(5);
	}

	if (extractVideo) {

		double videoFrameRate = AVI_frame_rate(aviFile);
		u_int32_t numDesiredVideoFrames = floor(duration * videoFrameRate);
		u_int32_t numVideoFrames = AVI_video_frames(aviFile);
		u_int32_t fileDuration = ceil(numVideoFrames / videoFrameRate);
		u_int32_t videoFramesRead = 0;

		/* get a buffer large enough to handle a frame of raw SDTV */
		u_char* buf = (u_char*)malloc(768 * 576 * 4);

		if (buf == NULL) {
			fprintf(stderr,
				"%s: error allocating memory: %s\n",
				progName, strerror(errno));
			exit(6);
		}

		/* check that start offset is valid */
		if (start > fileDuration) {
			fprintf(stderr,
				"%s: specified start is past the end of the file\n",
				progName);
			exit(7);
		}

		if (AVI_seek_start(aviFile)) {
			fprintf(stderr,
				"%s: bad seek: %s\n",
				progName, AVI_strerror());
			exit(8);
		}
		if (AVI_set_video_position(aviFile, ROUND(start * videoFrameRate))) {
			fprintf(stderr,
				"%s: bad seek: %s\n",
				progName, AVI_strerror());
			exit(9);
		}

		while ((numBytes = AVI_read_frame(aviFile, buf)) > 0) {
			if (fwrite(buf, 1, numBytes, rawFile) != numBytes) {
				fprintf(stderr,
					"%s: error writing %s: %s\n",
					progName, rawFileName, strerror(errno));
				break;
			}

			videoFramesRead++;
			if (duration && videoFramesRead >= numDesiredVideoFrames) {
				break;
			}
		}

		if (duration && videoFramesRead < numDesiredVideoFrames) {
			fprintf(stderr,
				"%s: warning: could only extract %u seconds of video\n",
				progName, ceil(videoFramesRead / videoFrameRate));
		}

		/* cleanup */
		free(buf);

	} else {
		/* extract audio */
		u_int32_t numAudioBytes = AVI_audio_bytes(aviFile);
		u_int32_t audioBytesPerSec = AVI_audio_rate(aviFile) * 	
			((AVI_audio_bits(aviFile) + 7) / 8) * AVI_audio_channels(aviFile);
		u_int32_t fileDuration = ceil(numAudioBytes / audioBytesPerSec);
		u_int32_t numDesiredAudioBytes = duration * audioBytesPerSec;
		u_int32_t audioBytesRead = 0;
		u_char buf[8*1024];

		/* check that start offset is valid */
		if (start > fileDuration) {
			fprintf(stderr,
				"%s: specified start is past the end of the file\n",
				progName);
			exit(7);
		}

		if (AVI_seek_start(aviFile)) {
			fprintf(stderr,
				"%s: bad seek: %s\n",
				progName, AVI_strerror());
			exit(8);
		}
		if (AVI_set_audio_position(aviFile, start * audioBytesPerSec)) {
			fprintf(stderr,
				"%s: bad seek: %s\n",
				progName, AVI_strerror());
			exit(9);
		}

		while ((numBytes = AVI_read_audio(aviFile, buf, sizeof(buf))) > 0) {
			if (fwrite(buf, 1, numBytes, rawFile) != numBytes) {
				fprintf(stderr,
					"%s: error writing %s: %s\n",
					progName, rawFileName, strerror(errno));
				break;
			}

			audioBytesRead += numBytes;
			if (numDesiredAudioBytes 
			  && audioBytesRead >= numDesiredAudioBytes) {
				break;
			}
		}

		if (duration && audioBytesRead < numDesiredAudioBytes) {
			fprintf(stderr,
				"%s: warning: could only extract %u seconds of audio\n",
				progName, audioBytesRead / audioBytesPerSec);
		}
	}

	/* cleanup */
	AVI_close(aviFile);
	fclose(rawFile);

	exit(0);
}

