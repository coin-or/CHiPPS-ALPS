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
    AlpsEncoded* encoded = 
	new AlpsEncoded(const_cast<char*>(typeid(*this).name()));
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
