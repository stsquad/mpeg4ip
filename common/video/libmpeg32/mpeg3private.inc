#ifndef LIBMPEG3_INC
#define LIBMPEG3_INC



#define MPEG3_FLOAT32 float

#define MPEG3_TOC_PREFIX                 0x544f4320
#define MPEG3_ID3_PREFIX                 0x494433
#define MPEG3_IFO_PREFIX                 0x44564456
#define MPEG3_IO_SIZE                    0x800        /* Bytes read by mpeg3io at a time */
#define MPEG3_RIFF_CODE                  0x52494646
#define MPEG3_PROC_CPUINFO               "/proc/cpuinfo"
#define MPEG3_RAW_SIZE                   0x100000     /* Largest possible packet */
#define MPEG3_TS_PACKET_SIZE             188
#define MPEG3_DVD_PACKET_SIZE            0x800
#define MPEG3_SYNC_BYTE                  0x47
#define MPEG3_PACK_START_CODE            0x000001ba
#define MPEG3_SEQUENCE_START_CODE        0x000001b3
#define MPEG3_SEQUENCE_END_CODE          0x000001b7
#define MPEG3_SYSTEM_START_CODE          0x000001bb
#define MPEG3_STRLEN                     1024
#define MPEG3_PIDMAX                     20             /* Maximum number of PIDs in one stream */
#define MPEG3_PROGRAM_ASSOCIATION_TABLE  0x00
#define MPEG3_CONDITIONAL_ACCESS_TABLE   0x01
#define MPEG3_PACKET_START_CODE_PREFIX   0x000001
#define MPEG3_PRIVATE_STREAM_2           0xbf
#define MPEG3_PADDING_STREAM             0xbe
#define MPEG3_GOP_START_CODE             0x000001b8
#define MPEG3_PICTURE_START_CODE         0x00000100
#define MPEG3_EXT_START_CODE             0x000001b5
#define MPEG3_USER_START_CODE            0x000001b2
#define MPEG3_SLICE_MIN_START            0x00000101
#define MPEG3_SLICE_MAX_START            0x000001af
#define MPEG3_AC3_START_CODE             0x0b77
#define MPEG3_PCM_START_CODE             0x0180
#define MPEG3_MAX_CPUS                   256
#define MPEG3_MAX_STREAMS                0x10000
#define MPEG3_MAX_PACKSIZE               262144
#define MPEG3_CONTIGUOUS_THRESHOLD       10  /* Positive difference before declaring timecodes discontinuous */
#define MPEG3_PROGRAM_THRESHOLD          5   /* Minimum number of seconds before interleaving programs */
#define MPEG3_SEEK_THRESHOLD             16  /* Number of frames difference before absolute seeking */
#define MPEG3_AUDIO_CHUNKSIZE            0x10000 /* Size of chunk of audio in table of contents */
#define MPEG3_LITTLE_ENDIAN              ((*(uint32_t*)"x\0\0\0") & 0x000000ff)

/* Values for audio format */
#define AUDIO_UNKNOWN 0
#define AUDIO_MPEG 1
#define AUDIO_AC3  2
#define AUDIO_PCM  3
#define AUDIO_AAC  4
#define AUDIO_JESUS  5


/* Table of contents */
#define FILE_TYPE_PROGRAM 0x0
#define FILE_TYPE_TRANSPORT 0x1
#define FILE_TYPE_AUDIO 0x2
#define FILE_TYPE_VIDEO 0x3

#define STREAM_AUDIO 0x4
#define STREAM_VIDEO 0x5

#define OFFSETS_AUDIO 0x6
#define OFFSETS_VIDEO 0x7

#define ATRACK_COUNT 0x8
#define VTRACK_COUNT 0x9

#define TITLE_PATH 0x2



#endif
