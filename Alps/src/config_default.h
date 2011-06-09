/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Eclipse Public License as part of the       *
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

/* include the COIN-OR-wide system specific configure header */
#include "configall_system.h"

/* include the public project specific macros */
#include "config_alps_default.h"

/***************************************************************************/
/*             HERE DEFINE THE PROJECT SPECIFIC MACROS                     */
/*    These are only in effect in a setting that doesn't use configure     */
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

