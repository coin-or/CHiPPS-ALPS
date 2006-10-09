#include "AlpsLicense.h"

#ifndef AlpsSunos_h
#define AlpsSunos_h

// AlpsSunos.h is modified from BCP_sunos.hpp
// This file is fully docified.
// There's nothing to docify...

typedef int AlpsIndexType;

#if defined(__GNUC__)

#  include <sys/time.h>     // for gettimeofday()
#  include <sys/resource.h> // for setpriority()
#  include <unistd.h>       // to get gethostname() from unistd.h
#  define NEED_IMPLICIT_TEMPLATE_FUNCTIONS 1
#  ifdef __OPTIMIZE__
#    define NEED_IMPLICIT_TEMPLATE_CLASSES 1
#  endif
#  define NEED_IMPLICIT_TEMPLATE_CLASSES 1
#  define AlpsPtrDiff        int
#  define ALPS_CONSTRUCT     construct
#  define ALPS_DESTROY       destroy
#  define ALPS_DESTROY_RANGE destroy

#endif

#endif
