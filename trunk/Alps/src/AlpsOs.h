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
 * Copyright (C) 2001-2007, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

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
