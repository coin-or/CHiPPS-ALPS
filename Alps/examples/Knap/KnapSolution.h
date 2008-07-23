/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Common Public License as part of the        *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors: Yan Xu, Lehigh University                                       *
 *          Ted Ralphs, Lehigh University                                    *
 *          Laszlo Ladanyi, IBM T.J. Watson Research Center                  *
 *          Matthew Saltzman, Clemson University                             *
 *                                                                           * 
 *                                                                           *
 * Copyright (C) 2001-2008, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#ifndef KnapSolution_h
#define KnapSolution_h

#include "AlpsSolution.h"

#include "KnapModel.h"


class KnapSolution : public AlpsSolution {
 private:
  /** The solution (indicator vector for the items) and its value */
  int  size_;
  int* solution_;
  int  value_;
  /** To access model data. */
  // I guess it is necessary to add a pointer to model (origin, prosolved)
  const KnapModel* model_;

 public:
  KnapSolution(const KnapModel* m) 
    : 
    size_(0), 
    solution_(0), 
    value_(0), 
    model_(m) 
    {}
  KnapSolution(int s, int*& sol, int v, const KnapModel* m) 
    : 
    size_(s), 
    solution_(sol), 
    value_(v),
    model_(m)
    { sol = 0; }
  ~KnapSolution() { 
    if (solution_ != 0) {
      delete [] solution_; 
      solution_ = 0;
    }
  }
  
  /** Get the best solution value */
  double getObjValue() const { return value_; }
  
  virtual double getQuality() const { return getObjValue(); }

  /** Get the size of the solution */
  int getSize() const { return size_; }

  /** Get item i in the solution vector */
  int getSolution(int i) const { return solution_[i]; }

  /** Get model data. */
  const KnapModel* getModel() const { return model_; }
  
  /** Print out the solution.*/
  virtual void print(std::ostream& os) const;
  
  /** The method that encodes the node into a encoded object. */
  virtual AlpsEncoded* encode() const;
  
  /** The method that decodes the node from a encoded object. */
  // virtual AlpsKnowledge* decode(const AlpsEncoded&) const;
  virtual AlpsKnowledge* decode(AlpsEncoded&) const;
};

#endif
