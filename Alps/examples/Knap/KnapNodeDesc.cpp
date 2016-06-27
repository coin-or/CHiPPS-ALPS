/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Eclipse Public License as part of the       *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors: Yan Xu, Lehigh University                                       *
 *          Ted Ralphs, Lehigh University                                    *
 *          Laszlo Ladanyi, IBM T.J. Watson Research Center                  *
 *          Matthew Saltzman, Clemson University                             *
 *                                                                           *
 *                                                                           *
 * Copyright (C) 2001-2015, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#include "KnapNodeDesc.h"
#include "KnapModel.h"

KnapNodeDesc::KnapNodeDesc(KnapModel * model): AlpsNodeDesc(),
                                               model_(model),
                                               usedCapacity_(0),
                                               usedValue_(0) {
  int n = model_->getNumItems();
  varStatus_ = new KnapVarStatus[n];
  std::fill(varStatus_, varStatus_ + n, KnapVarFree);
}

KnapNodeDesc::KnapNodeDesc(KnapModel * model, KnapVarStatus *& st, int cap,
                           int val)
  : AlpsNodeDesc(),
    model_(model),
    varStatus_(0),
    usedCapacity_(cap),
    usedValue_(val) {
  std::swap(varStatus_, st);
}

KnapNodeDesc::~KnapNodeDesc() {
  if (varStatus_) {
    delete[] varStatus_;
    varStatus_ = NULL;
  }
}

void KnapNodeDesc::setVarStatus(const int i, const KnapVarStatus status) {
  varStatus_[i] = status;
}

KnapVarStatus KnapNodeDesc::getVarStatus(const int i) {
  return varStatus_[i];
}

KnapVarStatus const * KnapNodeDesc::getVarStati() const {
  return varStatus_;
}

/// Pack this node description into the given #AlpsEncoded object.
AlpsReturnStatus KnapNodeDesc::encode(AlpsEncoded * encoded) const {
  int n = model_->getNumItems();
  encoded->writeRep(usedCapacity_);
  encoded->writeRep(usedValue_);
  encoded->writeRep(varStatus_, n);
  return AlpsReturnStatusOk;
}

/// Unpack fields from the given #AlpsEncoded object.
AlpsReturnStatus KnapNodeDesc::decodeToSelf(AlpsEncoded & encoded) {
  encoded.readRep(usedCapacity_);
  encoded.readRep(usedValue_);
  if (varStatus_) {
    delete[] varStatus_;
    varStatus_ = NULL;
  }
  // todo(aykut) do we need to allocate memory?
  int n = model_->getNumItems();
  if (varStatus_) {
    delete[] varStatus_;
    varStatus_ = NULL;
  }
  encoded.readRep(varStatus_, n);
  return AlpsReturnStatusOk;
}

AlpsNodeDesc * KnapNodeDesc::decode(AlpsEncoded & encoded) const {
  std::cerr << "Not implemented!" << std::endl;
  throw std::exception();
}
