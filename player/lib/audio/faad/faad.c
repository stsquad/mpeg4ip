#ifndef PLUGIN

#ifdef _WIN32
	#include <windows.h>
#else
	#include <time.h>
#endif

#include <stdlib.h>
#include "faad.h"
#include "faad_all.h"
#include "wav.h"

#define MAX_CHANNELS 8

static char *get_filename(char *pPath)
{
    char *pT;

    for (pT = pPath; *pPath; pPath++) {
        if ((pPath[0] == '\\' || pPath[0] == ':') && pPath[1] && (pPath[1] != '\\'))
            pT = pPath + 1;
    }

    return pT;
}

static void combine_path(char *path, char *dir, char *file)
{
	/* Should be a bit more sophisticated
	   This code assumes that the path exists */

	/* Assume dir is allocated big enough */
	if (dir[strlen(dir)-1] != '\\')
		strcat(dir, "\\");
	strcat(dir, get_filename(file));
	strcpy(path, dir);
}

static void usage(void)
{
	printf("Usage:\n");
	printf("faad.exe -options file ...\n");
	printf("Options:\n");
	printf(" -h    Shows this help screen.\n");
	printf(" -tX   Set output file type, X can be \".wav\",\".aif\" or \".au\".\n");
	printf(" -oX   Set output directory.\n");
	printf(" -w    Write output to stdio instead of a file.\n");
	printf(" file  Multiple aac files can be given as well as wild cards.\n");
	printf("Example:\n");
	printf("       faad.exe -oc:\\aac\\ *.aac\n");
	return;
}


int main(int argc, char *argv[])
{
	short       sample_buffer[1024*MAX_CHANNELS];
	int         i, writeToStdio = 0, FileCount = 0, out_dir_set = 0;
	int					showHelp = 0;
	char        *argp, *FileNames[200], out_dir[255], type[] = ".wav";
	faadAACInfo fInfo;
	FILE     *sndfile = NULL;

/* System dependant types */
#ifdef _WIN32
	long        begin, end;
#endif

	/* Process the command line */
	if (argc == 1) {
		usage();
		return 1;
	}

	for (i = 1; i < argc; i++)
	{
	  if (argv[i][0] != '-') { // &&(argv[i][0] != '/')) {
			if (strchr("-/", argv[i][0]))
				argp = &argv[i][1];
			else  argp = argv[i];

			if (!strchr(argp, '*') && !strchr(argp, '?'))
			{
				FileNames[FileCount] = (char *)malloc((strlen(argv[i])+10)*sizeof(char));
				strcpy(FileNames[FileCount], argv[i]);
				FileCount++;
			} else {
#ifdef _WIN32
				HANDLE hFindFile;
				WIN32_FIND_DATA fd;

				char path[255], *p;

				if (NULL == (p = strrchr(argp, '\\')))
					p = strrchr(argp, '/');
				if (p)
				{
					char ch = *p;

					*p = 0;
					lstrcat(strcpy(path, argp), "\\");
					*p = ch;
				}
				else
					*path = 0;

				if (INVALID_HANDLE_VALUE != (hFindFile = FindFirstFile(argp, &fd)))
				{
					do
					{
						FileNames[FileCount] = malloc(strlen(fd.cFileName)
							+ strlen(path) + 2);
						strcat(strcpy(FileNames[FileCount], path), fd.cFileName);
						FileCount++;
					} while (FindNextFile(hFindFile, &fd));
					FindClose(hFindFile);
				}
#else
				printf("Wildcards not yet supported on systems other than WIN32\n");
#endif
			}
		} else {
			switch(argv[i][1]) {
			case 'o':
			case 'O':
				out_dir_set = 1;
				strcpy(out_dir, &argv[i][2]);
				break;
			case 't':
			case 'T':
				strcpy(type, &argv[i][2]);
				break;
			case 'h':
			case 'H':
				showHelp=1;
				break;
			case 'w':
			case 'W':
				writeToStdio = 1;
				break;
			}
		}
	}

	if(!writeToStdio)	{
		/* Only write stuff if we are dumping output to stdio */
		printf("FAAD cl 0.6 (Freeware AAC Decoder)\n");
		printf("FAAD homepage: %s\n", "http://www.audiocoding.com");
	}

	if((!writeToStdio) && (showHelp)) {
		/* Print help if requested and not dumping to stdout */
		usage();
		return 1;
	}
	else if (showHelp)
		/* just return */
		return 1;

	if ((FileCount == 0) && (!writeToStdio)) {
		/* if there are no files print help if not fumping to stdout */
		usage();
		return 1;
	}
	else if (FileCount == 0)
		/* just return */
		return 1;

	for (i = 0; i < FileCount; i++)
	{
		char audio_fn[1024];
		char *fnp;
		int bits;
                int first_time = 1;

		#ifdef _WIN32
		begin = GetTickCount();
		#endif
		aac_decode_init_filestream(FileNames[i]);
		aac_decode_init(&fInfo, 1);

		if(!writeToStdio)
			printf("Busy decoding %s\n", FileNames[i]);

		/* Only calculate the path and open the file for writing if we are not writing to stdout. */
		if(!writeToStdio)
		{
			if (out_dir_set)
				combine_path(audio_fn, out_dir, FileNames[i]);
			else
				strcpy(audio_fn, FileNames[i]);

			if(strncmp("http://", audio_fn, 7) == 0)
			{
				int j;
				/* URL */

				for(j=strlen(FileNames[i]); j > 0; j--)
				{
					if(FileNames[i][j] == '/')
						break;
				}

				j++;

				memset(audio_fn, 0, 1024);
				strncpy(audio_fn, FileNames[i] + j, strlen(FileNames[i] + j));
			}

			fnp = (char *)strrchr(audio_fn,'.');

			if (fnp)
		  		fnp[0] = '\0';

			strcat(audio_fn, ".wav");
		}

		first_time = 1;

		do
		{
			bits = aac_decode_frame(sample_buffer, 0);

			if (bits > 0)
			{
				if(first_time)
				{
					first_time = 0;

					if(!writeToStdio)
					{
						fInfo.channels = mc_info.nch;
						sndfile = fopen(audio_fn, "w+b"); 

						if (sndfile==NULL)
						{
							printf("Unable to create the file << %s >>.\n", FileNames[i]);
							continue;
						}

						CreateWavHeader(sndfile, fInfo.channels, /*fInfo.sampling_rate*/44100, 16);
					}
					else
					{
						sndfile = stdout;
					}
				}

				fwrite(sample_buffer, sizeof(short), 1024*fInfo.channels, sndfile);
			}
		} while (bits > 0);

		aac_decode_free();

		if(!writeToStdio)
			UpdateWavHeader(sndfile);

		/* Only close if we are not writing to stdout */
		if(!writeToStdio)
			if(sndfile)
				fclose(sndfile);

		#ifdef _WIN32
			if(!writeToStdio)
			{
				end = GetTickCount();
				printf("Decoding %s took: %d sec.\n", FileNames[i], (end-begin)/1000);
			}
		#else
			/* clock() grabs time since the start of the app, and since
			 * we want the total processign time, we jsut need 1 call */
			if(!writeToStdio)
				printf("Decoding %s took: %5.2f sec.\n", FileNames[i],
					(float)(clock())/(float)CLOCKS_PER_SEC);
		#endif
	}

	return 0;
}

#endif // PLUGIN

