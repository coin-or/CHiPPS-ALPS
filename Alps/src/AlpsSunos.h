/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Common Public License as part of the        *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors: Yan Xu, SAS Institute Inc.                                       *
 *          Ted Ralphs, Lehigh University                                    *
 *          Laszlo Ladanyi, IBM T.J. Watson Research Center                  *
 *          Matthew Saltzman, Clemson University                             *
 *                                                                           * 
 *                                                                           *
 * Copyright (C) 2001-2006, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

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
