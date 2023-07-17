// Stop clangd from complaining
#ifdef CLANGD
#define __GLIBC_PREREQ(...) 0
#endif

#include "int.h"
