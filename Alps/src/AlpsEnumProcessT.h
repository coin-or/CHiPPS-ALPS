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
 * Copyright (C) 2001-2004, International Business Machines                  *
 * Corporation, Lehigh University, Yan Xu, Ted Ralphs, Matthew Salzman and   *
 * others. All Rights Reserved.                                              *
 *===========================================================================*/


#ifndef AlpsEnumProcessT_H
#define AlpsEnumProcessT_H

//#############################################################################

/** This enumerative constant describes the various process types. */

enum AlpsProcessType {
  /** */
  AlpsProcessTypeMaster,
  /** */
  AlpsProcessTypeHub,
  /** */
  AlpsProcessTypeWorker,
  /** */
  AlpsProcessTypeCG,
  /** */
  AlpsProcessTypeVG,
  /** */
  AlpsProcessTypeCP,
  /** */
  AlpsProcessTypeVP,
  /** */
  AlpsProcessTypeAny
};

#endif
