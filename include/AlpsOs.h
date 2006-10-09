#include "AlpsLicense.h"

#ifndef AlpsOs_h
#define AlpsOs_h

// AlpsOs.h is modified from BCP_os.hpp
// This file is fully docified.
// There's nothing to docify...

#if (defined(__GNUC__) && defined(__linux__))
#  include "AlpsLinux.h"
#endif

#if defined(__CYGWIN__) && defined(__GNUC__) 
#  include "AlpsCygwin.h"
#endif

#if defined(_AIX43)
#  include "AlpsAix43.h"
#endif

#if defined(__GNUC__) && defined(__sparc) && defined(__sun)
#  include "AlpsSunos.h"
#endif

#if defined(__MACH__) && defined(__GNUC__)
#  include "AlpsMACH.h"
#endif

#endif
