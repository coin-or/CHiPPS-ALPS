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


#ifndef AbcSolution_h
#define AbcSolution_h

#include "AlpsSolution.h"

#include "AbcModel.h"

#include <algorithm>

/** This class holds a MIP feasible primal solution. */
class AbcSolution : public AlpsSolution {
private:
  int size_;
  double* value_;
  double objective_;

public:
  AbcSolution()
    :
    size_(0),
    value_(0),
    objective_()
  {}
  AbcSolution(const int s, const double* val, const double obj)
    :
    size_(s)
  {
    if (size_ >= 0) {
      value_ = new double [size_];
      memcpy(value_, val, sizeof(double) * size_);
    }
  }

  ~AbcSolution() {
    if (value_ != 0) {
      delete [] value_;
      value_ = 0;
    }
  }

  /** Get the objective value value */
  double getObjValue() const { return objective_; }

  void setObj(double obj) { objective_=obj; }

  void setValue(int size, double const * value) {
    if (value_) {
      delete[] value_;
    }
    size_ = size;
    value_ = new double[size];
    std::copy(value, value+size, value_);
  }

  virtual double getQuality() const { return getObjValue(); }

  /** Get the size of the solution */
  int getSize() const { return size_; }

  /** Get the column solution */
  const double* getColSolution() const
  { return value_; }

  /** Get item i in the solution vector */
  double getColSolution(int i) const { return value_[i]; }

  /** Print out the solution.*/
  virtual void print(std::ostream& os) const;

  using AlpsSolution::encode;
  /** The method that encodes the solution into a encoded object. */
  virtual AlpsReturnStatus encode(AlpsEncoded * encoded) const;

  virtual AlpsReturnStatus decodeToSelf(AlpsEncoded & encoded) {
    std::cerr << "Not implemented!" << std::endl;
    throw std::exception();
  }
  /** The method that decodes the solution from a encoded object. */
  virtual AlpsKnowledge * decode(AlpsEncoded &) const;
};

#endif
