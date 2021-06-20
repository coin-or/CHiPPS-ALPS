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


#ifndef KnapSolution_h
#define KnapSolution_h

#include "AlpsSolution.h"

class KnapSolution : public AlpsSolution {
  KnapModel * model_;
  /** The solution (indicator vector for the items) and its value */
  int  size_;
  int* solution_;
  int  value_;

public:
  KnapSolution(KnapModel * model);
  KnapSolution(KnapModel * model, int s, int*& sol, int v);
  virtual ~KnapSolution();

  /** Get the best solution value */
  double getObjValue() const { return value_; }

  virtual double getQuality() const { return getObjValue(); }

  /** Get the size of the solution */
  int getSize() const { return size_; }

  /** Get item i in the solution vector */
  int getSolution(int i) const { return solution_[i]; }

  /** Print out the solution.*/
  virtual void print(std::ostream& os) const;

  using AlpsSolution::encode;
  /// Encode this into the given AlpsEncoded object.
  virtual AlpsReturnStatus encode(AlpsEncoded * encoded) const;
  /// Decode the given AlpsEncoded object into this.
  virtual AlpsReturnStatus decodeToSelf(AlpsEncoded & encoded) {
    std::cerr << "Not implemented!" << std::endl;
    throw std::exception();
  }
  /// Decode the given AlpsEncoded object into a new KnapSolution
  /// and return a pointer to it.
  virtual AlpsKnowledge * decode(AlpsEncoded & encoded) const;
private:
  /// Disable copy constructor
  KnapSolution(KnapSolution const & other);
  /// Disable copy assignment operator
  KnapSolution & operator=(KnapSolution const & rhs);
};

#endif
