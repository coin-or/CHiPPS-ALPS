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
 * Copyright (C) 2001-2017, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#include "AlpsKnowledge.h"

// Initialize static member.
//std::map<const char*, const AlpsKnowledge*, AlpsStrLess>*
//AlpsKnowledge::decodeMap_ = new std::map<const char*, const AlpsKnowledge*,
//  AlpsStrLess>;


// encodes this into a new object and returns a pointer to it.
AlpsEncoded * AlpsKnowledge::encode() const {
  AlpsEncoded * encoded = new AlpsEncoded(type_);
  AlpsReturnStatus status = encode(encoded);
  return encoded;
}

// encodes this into the given AlpsEncoded object.
AlpsReturnStatus AlpsKnowledge::encode(AlpsEncoded * encoded) const {
  encoded->writeRep(*this);
  return AlpsReturnStatusOk;
}

AlpsReturnStatus AlpsKnowledge::decodeToSelf(AlpsEncoded & encoded) {
  encoded.readRep(*this);
  return AlpsReturnStatusOk;
}
