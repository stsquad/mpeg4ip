#ifndef __OUR_ASSERT_H__
#define __OUR_ASSERT_H__

#ifdef _DEBUG
#define ASSERT(a) if (!(a)) our_assert(#a);
#else
#define ASSERT(a)
#endif

#ifdef __cplusplus
extern "C" {
#endif
void our_assert(const char *);

#ifdef __cplusplus
}
#endif
#endif

