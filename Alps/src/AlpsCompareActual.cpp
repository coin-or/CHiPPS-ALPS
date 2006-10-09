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
 * Corporation, Lehigh University, Yan Xu, Ted Ralphs, Matthew Salzman and   *
 *===========================================================================*/

#include "AlpsCompareActual.h"

//#############################################################################

bool 
AlpsCompareSubTreeBreadth::test(AlpsSubTree * x, AlpsSubTree * y)
{
    return x->getRoot()->getDepth() > y->getRoot()->getDepth();
}

//#############################################################################

bool 
AlpsCompareSubTreeBest::test(AlpsSubTree * x, AlpsSubTree * y) 
{
    return x->getQuality() < y->getQuality();
}

//#############################################################################

bool 
AlpsCompareSubTreeQuantity::test(AlpsSubTree * x, AlpsSubTree * y) 
{
    return x->getNumNodes() < y->getNumNodes();
}

//#############################################################################
