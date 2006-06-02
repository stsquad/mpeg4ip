

#ifndef __GENERIC_MUTEX_H__
#define __GENERIC_MUTEX_H__ 1

typedef struct mutex_ *mutex_t;
typedef struct thread_ *thread_t;

typedef int (*thread_func_f)(void *);


mutex_t MutexCreate(void);
int MutexLock(mutex_t);
int MutexUnlock(mutex_t);
void MutexDestroy(mutex_t);

thread_t ThreadCreate(thread_func_f, void *);
int ThreadWait(thread_t);

#endif
