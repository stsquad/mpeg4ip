#include "emms.h"
#include "../portab.h"

emmsFuncPtr emms;
void emms_c() {}
void emms_mmx() { EMMS(); }
