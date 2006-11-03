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
 *===========================================================================*/

#ifndef AlpsCompareBase_h_
#define AlpsCompareBase_h_

//#############################################################################
// This file is modified from SbbCompareBase.hpp
//#############################################################################

class AlpsModel;

template<class T>
class AlpsCompareBase {
 protected:
    double weight_;
    
 public:
    // Default Constructor 
    AlpsCompareBase (): weight_(-1.0) {}

  // This allows any method to change behavior as it is called
  // after each solution
  virtual void newSolution(AlpsModel * model) {}

  // This Also allows any method to change behavior as it is called
  // after each solution
  virtual void newSolution(AlpsModel * model,
			   double objectiveAtContinuous,
			   int numberInfeasibilitiesAtContinuous) {}

  // This allows any method to change behavior as it is called
  // after every 1000 nodes
  virtual void every1000Nodes(AlpsModel * model,int numberNodes) {}

  virtual ~AlpsCompareBase() {}

  /// This is test function
  virtual bool test (T x, T y) {return true;}
  
  bool operator() (T x, T y) {
    return test(x,y);
  }
  
  inline const double getWeight() const { return weight_; }
  inline void setWeight(double nw) { weight_ = nw; }
  
};

//#############################################################################

template<class T>
class AlpsCompare {
 public:
    AlpsCompareBase<T>* test_;
    
 public:
    // Default Constructor 
    AlpsCompare () : 
	test_(0)
	{}
    virtual ~AlpsCompare() {}

    void setComareBase(AlpsCompareBase<T>* t) { 
	test_ = t; 
    }
    
    bool operator() (T x, T y) {
	return test_->test(x,y);
    }
};
#endif
