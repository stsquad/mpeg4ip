
#include "mpeg4ip.h"
#include "fposrec.h"

CFilePosRecorder::CFilePosRecorder (void)
{
  m_first = m_last = NULL;
}

CFilePosRecorder::~CFilePosRecorder (void)
{
  frame_file_pos_t *ptr;

  while (m_first != NULL) {
    ptr = m_first;
    m_first = m_first->next;
    free(ptr);
  }
  m_last = NULL;
}

void CFilePosRecorder::record_point (uint64_t file_position,
				     uint64_t ts, 
				     uint64_t frame)
{
  frame_file_pos_t *ptr;

  if (m_first == NULL) {
    m_first = m_last = (frame_file_pos_t *)malloc(sizeof(frame_file_pos_t));
    ptr = m_first;
    ptr->next = NULL;
  } else {
    if (ts == m_last->timestamp) {
      return;
    } else if (ts > m_last->timestamp) {
      m_last->next = (frame_file_pos_t *)malloc(sizeof(frame_file_pos_t));
      m_last = m_last->next;
      ptr = m_last;
      ptr->next = NULL;
    } else if (ts == m_first->timestamp) {
      return;
    } else if (ts < m_first->timestamp) {
      ptr = (frame_file_pos_t *)malloc(sizeof(frame_file_pos_t));
      ptr->next = m_first;
      m_first = ptr;
    } else {
      frame_file_pos_t *p, *q;
      ptr = NULL;
      q = m_first;
      p = m_first->next;
      while (ptr == NULL) {
	if (ts == p->timestamp)
	  return;
	if (ts < p->timestamp) {
	  ptr = (frame_file_pos_t *)malloc(sizeof(frame_file_pos_t));
	  q->next = ptr;
	  ptr->next = p;
	} else {
	  q = p;
	  p = p->next;
	}
      }
    }
  }
  ptr->timestamp = ts;
  ptr->file_position = file_position;
  ptr->frames = frame;
}

const frame_file_pos_t *CFilePosRecorder::find_closest_point (uint64_t ts)
{
  frame_file_pos_t *p, *q;
  if (m_first == NULL) {
    return NULL;
  }

  if (m_last->timestamp <= ts) {
    return m_last;
  }

  if (m_first->timestamp >= ts) return m_first;

  q = m_first;
  p = m_first->next;
  while (p != NULL) {
    if (ts < p->timestamp) {
      return q;
    }
    q = p;
    p = p->next;
  }
  return NULL;
}
