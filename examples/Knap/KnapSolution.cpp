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
 * Copyright (C) 2001-2019, Lehigh University, Yan Xu, Aykut Bulut, and      *
 *                          Ted Ralphs.                                      *
 * All Rights Reserved.                                                      *
 *===========================================================================*/


#include <iostream>
#include <set>

#include "KnapModel.h"
#include "KnapSolution.h"

//#############################################################################

KnapSolution::KnapSolution(KnapModel * model): model_(model), size_(0),
                                               solution_(0), value_(0) {
}

KnapSolution::KnapSolution(KnapModel * model, int s, int*& sol, int v)
  : model_(model), size_(s), solution_(sol), value_(v) {
  sol = 0;
}

KnapSolution::~KnapSolution() {
  if (solution_) {
    delete [] solution_;
    solution_ = 0;
  }
}

void KnapSolution::print(std::ostream& os) const {
  const int* seq = model_->getSequence();

  int i;
  std::set<int> solu;
  for (i = 0; i < size_; ++i) {
    if (solution_[i] == 1)
      solu.insert( seq[i]+1 );
  }

  i = 0;
  std::set<int>::iterator pos;
  //os << "Items in knapsack are:\n\n";
  for (pos = solu.begin(); pos != solu.end(); ++pos) {
    os << *pos;
    if (i != 0 && !((++i)%5))
      os << "\n";
    else
      os << "\t";
  }
  os << std::endl;
}

//#############################################################################

AlpsReturnStatus KnapSolution::encode(AlpsEncoded * encoded) const  {
  encoded->writeRep(value_);
  encoded->writeRep(size_);     // Base operand of `->' has non-pointer type
  encoded->writeRep(solution_, size_);
  return AlpsReturnStatusOk;
}

//#############################################################################

// Note: write and read sequence MUST same!
AlpsKnowledge * KnapSolution::decode(AlpsEncoded& encoded) const {
  int s, v;
  int* sol = 0;
  // sol = new int[s];       // By default, don't need to allocate memory
  encoded.readRep(v);
  encoded.readRep(s);        // s must immediately before sol
  encoded.readRep(sol, s);
  return new KnapSolution(model_, s, sol, v);
}

//#############################################################################
