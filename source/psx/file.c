#include "../common.h"

#ifdef _PCDRV
#include "file_pcdrv.c"
#else
#include "file_cd.c"
#endif
