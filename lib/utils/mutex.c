#include "mpeg4ip_sdl_includes.h"
#if 0
#include <SDL/SDL.h>
#include <SDL/SDL_mutex.h>
#include <SDL/SDL_thread.h>
#endif

#include "mpeg4ip.h"
#include "mutex.h"

struct mutex_ {
  SDL_mutex *mutex;
};

mutex_t MutexCreate (void)
{
  mutex_t ret = MALLOC_STRUCTURE(struct mutex_);
  ret->mutex = SDL_CreateMutex();
  return ret;
}

int MutexLock (mutex_t mut)
{
  return SDL_LockMutex(mut->mutex);
}

int MutexUnlock (mutex_t mut) 
{
  return SDL_UnlockMutex(mut->mutex);
}

void MutexDestroy (mutex_t mut)
{
  SDL_DestroyMutex(mut->mutex);
  free(mut);
}

struct thread_ {
  SDL_Thread *thread;
};

thread_t ThreadCreate (thread_func_f func, void *ud)
{
  thread_t ret = MALLOC_STRUCTURE(struct thread_);
  ret->thread = SDL_CreateThread(func, ud);
  return ret;
}

int ThreadWait (thread_t thread)
{
  int ret;
  SDL_WaitThread(thread->thread, &ret);
  free(thread);
  return ret;
}

void ThreadSleep (uint msec)
{
  SDL_Delay(msec);
}
