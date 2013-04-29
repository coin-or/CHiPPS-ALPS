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
 * Copyright (C) 2001-2013, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#include "AlpsKnowledge.h"

//#############################################################################
// Initialize static member. 
//std::map<const char*, const AlpsKnowledge*, AlpsStrLess>*
//AlpsKnowledge::decodeMap_ = new std::map<const char*, const AlpsKnowledge*, 
//  AlpsStrLess>;

//#############################################################################

AlpsEncoded* 
AlpsKnowledge::encode() const
{
    AlpsEncoded* encoded = new AlpsEncoded(type_);
    encoded->writeRep(*this);
    return encoded;
}

//#############################################################################

AlpsKnowledge* 
AlpsKnowledge::decode(AlpsEncoded& encoded) const
{
    AlpsKnowledge* kl = new AlpsKnowledge;
    encoded.readRep(*kl);
    return kl;
}

//#############################################################################
