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

#ifndef AbcSolution_h
#define AbcSolution_h

#include "AlpsSolution.h"

#include "AbcModel.h"

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
  
    /** The method that encodes the solution into a encoded object. */
    virtual AlpsEncoded* encode() const;
  
    /** The method that decodes the solution from a encoded object. */
    virtual AlpsKnowledge* decode(AlpsEncoded&) const;
};

#endif
