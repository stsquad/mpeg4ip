#include "media_feeder.h"

CMediaFeeder::CMediaFeeder (void)
{
  m_pSinksMutex = SDL_CreateMutex();
  if (m_pSinksMutex == NULL) {
    debug_message("CreateMutex error");
  }
  for (int i = 0; i < MAX_SINKS; i++) {
    m_sinks[i] = NULL;
  }
}

CMediaFeeder::~CMediaFeeder (void)
{
  SDL_DestroyMutex(m_pSinksMutex);
  m_pSinksMutex = NULL;


}
bool CMediaFeeder::AddSink(CMediaSink* pSink) 
{
  bool rc = false;
  int i;
  if (SDL_LockMutex(m_pSinksMutex) == -1) {
    debug_message("AddSink LockMutex error");
    return rc;
  }
  for (i = 0; i < MAX_SINKS; i++) {
    if (m_sinks[i] == pSink) {
      SDL_UnlockMutex(m_pSinksMutex);
      return true;
    }
  }
  for (i = 0; i < MAX_SINKS; i++) {
    if (m_sinks[i] == NULL) {
      m_sinks[i] = pSink;
      rc = true;
      break;
    }
  }
  if (SDL_UnlockMutex(m_pSinksMutex) == -1) {
    debug_message("UnlockMutex error");
  }
  return rc;
}

void CMediaFeeder::StartSinks (void)
{
  if (SDL_LockMutex(m_pSinksMutex) == -1) {
    debug_message("Start Sinks LockMutex error");
    return;
  }
  for (int i = 0; i < MAX_SINKS; i++) {
    if (m_sinks[i] != NULL) {
      m_sinks[i]->Start();
    }
  }
  if (SDL_UnlockMutex(m_pSinksMutex) == -1) {
    debug_message("UnlockMutex error");
  }
}  

void CMediaFeeder::StopSinks (void)
{
  if (SDL_LockMutex(m_pSinksMutex) == -1) {
    debug_message("Start Sinks LockMutex error");
    return;
  }
  for (int i = 0; i < MAX_SINKS; i++) {
    if (m_sinks[i] != NULL) {
      m_sinks[i]->StopThread();
    }
  }
  if (SDL_UnlockMutex(m_pSinksMutex) == -1) {
    debug_message("UnlockMutex error");
  }
}  
void CMediaFeeder::RemoveSink(CMediaSink* pSink) 
{
  if (SDL_LockMutex(m_pSinksMutex) == -1) {
    debug_message("RemoveSink LockMutex error");
    return;
  }
  for (int i = 0; i < MAX_SINKS; i++) {
    if (m_sinks[i] == pSink) {
      int j;
      for (j = i; j < MAX_SINKS - 1; j++) {
	m_sinks[j] = m_sinks[j+1];
      }
      m_sinks[j] = NULL;
      break;
    }
  }
  if (SDL_UnlockMutex(m_pSinksMutex) == -1) {
    debug_message("UnlockMutex error");
  }
}

void CMediaFeeder::RemoveAllSinks(void) 
{
  if (SDL_LockMutex(m_pSinksMutex) == -1) {
    debug_message("RemoveAllSinks LockMutex error");
    return;
  }
  for (int i = 0; i < MAX_SINKS; i++) {
    if (m_sinks[i] == NULL) {
      break;
    }
    m_sinks[i] = NULL;
  }
  if (SDL_UnlockMutex(m_pSinksMutex) == -1) {
    debug_message("UnlockMutex error");
  }
}

void CMediaFeeder::ForwardFrame(CMediaFrame* pFrame)
{
  if (SDL_LockMutex(m_pSinksMutex) == -1) {
    debug_message("ForwardFrame LockMutex error");
    return;
  }

  for (int i = 0; i < MAX_SINKS; i++) {
    if (m_sinks[i] == NULL) {
      break;
    }
    m_sinks[i]->EnqueueFrame(pFrame);
    //debug_message("forward frame type %d", pFrame->GetType());
  }

  if (SDL_UnlockMutex(m_pSinksMutex) == -1) {
    debug_message("UnlockMutex error");
  }
  if (pFrame->RemoveReference()) delete pFrame;
  return;
}
