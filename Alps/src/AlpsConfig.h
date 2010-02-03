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
 * Copyright (C) 2006-2010, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

/*
 * Include file for the configuration of Alps.
 *
 * On systems where the code is configured with the configure script
 * (i.e., compilation is always done with HAVE_CONFIG_H defined), this
 * header file includes the automatically generated header file, and
 * undefines macros that might configure with other Config.h files.
 *
 * On systems that are compiled in other ways (e.g., with the
 * Developer Studio), a header files is included to define those
 * macros that depend on the operating system and the compiler.  The
 * macros that define the configuration of the particular user setting
 * (e.g., presence of other COIN packages or third party code) are set
 * here.  The project maintainer needs to remember to update this file
 * and choose reasonable defines.  A user can modify the default
 * setting by editing this file here.
 *
 */

#ifndef __ALPSCONFIG_H__

#ifdef HAVE_CONFIG_H
#include "config_alps.h"

/* undefine macros that could conflict with those in other config.h
   files */
#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef VERSION

#else /* HAVE_CONFIG_H */

/* include the COIN-wide system specific configure header */
#include "configall_system.h"

/***************************************************************************/
/*             HERE DEFINE THE CONFIGURATION SPECIFIC MACROS               */
/***************************************************************************/

/* Define to the debug sanity check level (0 is no test) */
#define COIN_ALPS_CHECKLEVEL 0

/* Define to the debug verbosity level (0 is no output) */
#define COIN_ALPS_VERBOSITY 0

/* Define to 1 if the ALPS package is used */
#define COIN_HAS_ALPS 1

/* Define to 1 if the CoinUtils package is used */
#define COIN_HAS_COINUTILS 1

/* Define to 1 if the Clp package is used */
#define COIN_HAS_CLP 1

/* Define to 1 if the Mpi package is used */
/* #define COIN_HAS_MPI 1 */

#endif /* HAVE_CONFIG_H */

#endif /*__HAVE_ALPS_CONFIG_H__*/
