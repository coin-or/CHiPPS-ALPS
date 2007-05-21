/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Common Public License as part of the        *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors:                                                                  *
 *                                                                           *
 *          Yan Xu, Lehigh University                                        *
 *          Ted Ralphs, Lehigh University                                    *
 *                                                                           *
 * Conceptual Design:                                                        *
 *                                                                           *
 *          Yan Xu, Lehigh University                                        *
 *          Ted Ralphs, Lehigh University                                    *
 *          Laszlo Ladanyi, IBM T.J. Watson Research Center                  *
 *          Matthew Saltzman, Clemson University                             *
 *                                                                           * 
 *                                                                           *
 * Copyright (C) 2001-2006, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#ifndef AlpsCygwin_h
#define AlpsCygwin_h

// This file is fully docified.
// There's nothing to docify...

typedef int AlpsIndexType;

#if defined(__GNUC__)

#  include <sys/time.h>     // for gettimeofday()
// # include <sys/resource.h> // for setpriority()
#  define setpriority(x,y,z) 
#  define ALPS_USE_RUSAGE 0
#  ifndef __USE_BSD
#    define __USE_BSD  // to get gethostname() from unistd.h
#    include <unistd.h>
#    undef __USE_BSD
#  else
#    include <unistd.h>
#  endif
#  define AlpsPtrDiff       int

#  if (__GNUC__ >= 3)

#    define NEED_TEMPLATE_CLASSES
#    define NEED_TEMPLATE_FUNCTIONS
// #    define NEED_STD_TEMPLATE_FUNCTIONS
// #    define NEED_IMPLICIT_TEMPLATE_CLASSES
// #    define NEED_IMPLICIT_TEMPLATE_FUNCTIONS
#    define ALPS_CONSTRUCT     std::_Construct
#    define ALPS_DESTROY       std::_Destroy
#    define ALPS_DESTROY_RANGE std::_Destroy

#  else

#    define NEED_TEMPLATE_CLASSES
#    define NEED_TEMPLATE_FUNCTIONS
// #    define NEED_STD_TEMPLATE_FUNCTIONS
// #    define NEED_IMPLICIT_TEMPLATE_CLASSES
// #    define NEED_IMPLICIT_TEMPLATE_FUNCTIONS
#    define ALPS_CONSTRUCT     std::construct
#    define ALPS_DESTROY       std::destroy
#    define ALPS_DESTROY_RANGE std::destroy

#  endif

#endif

#endif
