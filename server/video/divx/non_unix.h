
/******************************************************************************/
#ifndef _NON_UNIX_H_
#define _NON_UNIX_H_

#include "momusys.h"

#if defined(__POWERPC__)
#   define _RAWIO_
#   define _OLDINTRADC_
#   define __STDC__	1
#endif

#if defined(WIN32)
#   define _RAWIO_
#   define _OLDINTRADC_
#   define __STDC__ 1
#endif

#endif /* _NON_UNIX_H_ */
