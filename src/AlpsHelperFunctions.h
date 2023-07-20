/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Eclipse Public License as part of the       *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors:                                                                  *
 *                                                                           *
 *          Yan Xu, Lehigh University                                        *
 *          Aykut Bulut, Lehigh University                                   *
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
 * Copyright (C) 2001-2023, Lehigh University, Yan Xu, Aykut Bulut, and      *
 *                          Ted Ralphs.                                      *
 * All Rights Reserved.                                                      *
 *===========================================================================*/


#ifndef AlpsHelperFunctions_h_
#define AlpsHelperFunctions_h_

#if defined(NF_DEBUG)
#include <iostream>
#endif
#include <cmath>

#include "CoinTime.hpp"

#include "AlpsTreeNode.h"

//#############################################################################

/** A functor class used in calulating total workload in a node pool. */
class TotalWorkload : public std::unary_function<AlpsTreeNode*, void> {

 private:
    double totalLoad_;
    double incVal_;
    double rho_;

 public:
    TotalWorkload(const double incVal, const double rho)
        :
        totalLoad_(0.0),
        incVal_(incVal),
        rho_(rho)
        {}

    void operator()(AlpsTreeNode*& node) {
        totalLoad_ += pow(fabs(incVal_ - node->getQuality()), rho_);
    }

    double result() const { return totalLoad_; }
};

//#############################################################################
/** */
struct DeletePtrObject
{
    template<class T>
    void operator()(const T* ptr) const
        {
            delete ptr;
        }
};

//#############################################################################
/** Delay for the specified seconds. */
inline void AlpsSleep(double sec)
{
    double start = CoinCpuTime();
    while ( (CoinCpuTime() - start) < sec) { };
}
#endif
