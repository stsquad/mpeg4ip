
#include "our_assert.h"

extern "C" {
void our_assert (const char *err)
{
	throw err;
}
}