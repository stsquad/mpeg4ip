
typedef struct mp3_header_t {
  int mpeg25;
  int protection; // FALSE means protection - TRUE means not protected
  int layer;
  int version;
  int padding;
  int frequencyidx;
  int frequency;
  int bitrateindex;
  int extendedmode;
  int mode;
  int inputstereo;
  int framesize;
} mp3_header_t;

#define SCALEBLOCK 12
#ifdef __cplusplus
extern "C" {
#endif

int decode_mp3_header(unsigned char *buffer, mp3_header_t *mp3);

int mp3_get_backpointer(mp3_header_t *mp3, const unsigned char *buf);

int mp3_sideinfo_size(mp3_header_t *mp3);

int mp3_get_samples_per_frame (int layer, int version);
  
#ifdef __cplusplus
}
#endif
