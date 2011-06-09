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
 * Copyright (C) 2001-2011, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#ifndef AlpsAix43_h
#define AlpsAix43_h

// AlpsAix43.h is modified from BCP_aix43.hpp
// This file is fully docified.
// There's nothing to docify...

#include <sys/time.h>         // for gettimeofday()
#include <sys/resource.h>     // for getrusage()

#include <unistd.h>           // for setpriority() and gethostname()
typedef int AlpsIndexType;

#if defined(__GNUC__)

#  define NEED_IMPLICIT_TEMPLATE_FUNCTIONS 1
#  define NEED_IMPLICIT_TEMPLATE_CLASSES 1
#  define AlpsPtrDiff       long
#  define ALPS_CONSTRUCT     std::construct
#  define ALPS_DESTROY       std::destroy
#  define ALPS_DESTROY_RANGE std::destroy

#elif defined(__IBMCPP__) && (__IBMCPP__ >= 5)

#  define AlpsPtrDiff       long
#  define ALPS_CONSTRUCT     std::_Construct
#  define ALPS_DESTROY       std::_Destroy
#  define ALPS_DESTROY_RANGE(first, last)	\
          if (first != last)			\
             do {				\
                std::_Destroy(--last);		\
	     } while (first != last);
#endif

#endif
