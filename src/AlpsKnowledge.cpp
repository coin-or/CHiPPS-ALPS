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


#include "AlpsKnowledge.h"

AlpsKnowledge::AlpsKnowledge(AlpsKnowledgeType type,
                             AlpsKnowledgeBroker * broker)
  : type_(type), broker_(broker) {
}

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
