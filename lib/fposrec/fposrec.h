
#ifndef __FPOSREC_H__
#define __FPOSREC_H__ 1

typedef struct frame_file_pos_t
{
  struct frame_file_pos_t *next;
  uint64_t timestamp;
  uint64_t file_position;
  uint64_t frames;
} frame_file_pos_t;

class CFilePosRecorder
{
 public:
  CFilePosRecorder(void);
  ~CFilePosRecorder(void);

  void record_point(uint64_t file_position, uint64_t ts, uint64_t frame);
  const frame_file_pos_t *find_closest_point(uint64_t ts);
 private:
  frame_file_pos_t *m_first;
  frame_file_pos_t *m_last;
};

#endif
