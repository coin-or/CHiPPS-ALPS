/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Common Public License as part of the        *
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
 * Copyright (C) 2001-2007, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

//#############################################################################
// This file is modified from SbbNode.hpp
//#############################################################################

#ifndef AbcNodeDesc_h_
#define AbcNodeDesc_h_

#include "CoinHelperFunctions.hpp"
#include "CoinWarmStartBasis.hpp"

#include "AlpsNodeDesc.h"
#include "AbcModel.h"

class OsiSolverInterface;

class OsiCuts;
class OsiRowCut;
class OsiRowCutDebugger;

class AbcModel;
class AbcNode;

//#############################################################################


class AbcNodeDesc : public AlpsNodeDesc {
  
 private:
    
    /* Here, we need to fill in what the node description will look
       like. For now, we will not use differencing -- just explicitly
       represent it. Probably this means that we will just store the
       original problem data and a list of the variables that have been
       fixed. */
    
    /** */
    double* lowerBounds_;
    /** */
    double* upperBounds_;
    
    /** Number of rows in problem (before these cuts).  This
	means that for top of chain it must be rows at continuous */
    int numberRows_;
    ///
    int numberCols_;

    /** The index of the branching variable */
    int branchedOn_;

    /** The solution value (non-integral) of the branching variable. */
    double branchedOnVal_;

    /** Branching direction */
    int branchedDir_;    
 public:
    AbcNodeDesc()
	:
	AlpsNodeDesc(0),
	lowerBounds_(0), 
	upperBounds_(0),
	numberRows_(0),
	numberCols_(0),
	branchedOn_(-8),
	branchedOnVal_(0),
	branchedDir_(1)
	{
	}

    AbcNodeDesc(AbcModel* m) 
	:
	AlpsNodeDesc(m),
	lowerBounds_(0), 
	upperBounds_(0),
	numberRows_(0),
	numberCols_(0),
	branchedOn_(-9),
	branchedOnVal_(0),
	branchedDir_(1)
	{
	}  
  
    AbcNodeDesc(AbcModel* m, const double* lb, const double* ub) 
	:
	AlpsNodeDesc(m),
	branchedOn_(-10),
	branchedOnVal_(0),
	branchedDir_(1)
	{
	    numberRows_ = m->solver()->getNumRows();
	    numberCols_ = m->solver()->getNumCols();
	    assert(numberRows_ && numberCols_);
	    lowerBounds_ = new double [numberCols_];
	    upperBounds_ = new double [numberCols_];
	    memcpy(lowerBounds_, lb, sizeof(double)*numberCols_);
	    memcpy(upperBounds_, ub, sizeof(double)*numberCols_);
	}
  
    virtual ~AbcNodeDesc() { 
	if (lowerBounds_ != 0) {
	    delete [] lowerBounds_;
	    lowerBounds_ = 0;
	}
	if (upperBounds_ != 0) {
	    delete [] upperBounds_;
	    upperBounds_ = 0;
	}
    }

    double* lowerBounds()
	{
	    if(lowerBounds_ == 0) {
		assert(model_);
		AbcModel* m = dynamic_cast<AbcModel*>(model_);
		const int num = m->getNumCols();
		const double* lb = m->getColLower();
		lowerBounds_ = new double [num];
		memcpy(lowerBounds_, lb, sizeof(double)*num);
//		std::cout << "AbcNodeDesc::lowerBounds--num=" << num 
//			  <<std::endl;
	    }    
	    return lowerBounds_;
	}
    
    void setLowerBounds(const double* lb, const int size) 
	{
	    if(!lowerBounds_) {
		lowerBounds_ = new double [size];
	    }
	    CoinCopyN(lb, size, lowerBounds_);
	}
    
    void setLowerBound(const int index, const double lb)
	{
	    if(!lowerBounds_) {
		AbcModel * model = dynamic_cast<AbcModel*>(model_);
		const int numCols = model->getNumCols();    
		assert(numCols > index);
		lowerBounds_ = new double [numCols];
	    }

	    lowerBounds_[index] = lb;
	}
    
    double* upperBounds()
	{
	    if(upperBounds_ == 0) {
		assert(model_);
		AbcModel* m = dynamic_cast<AbcModel*>(model_);
		const int num = m->getNumCols();
		const double* ub = m->getColUpper();
		upperBounds_ = new double [num];
		memcpy(upperBounds_, ub, sizeof(double)*num);
	    }
	    return upperBounds_;
	}
    
    void setUpperBounds(const double* ub, const int size) 
	{
	    if(!upperBounds_) {
		upperBounds_ = new double [size];
	    }
	    CoinCopyN(ub, size, upperBounds_);
	}
    
    void setUpperBound(const int index, const double ub)
	{
	    if (!upperBounds_) {
		AbcModel * model = dynamic_cast<AbcModel*>(model_);
		const int numCols = model->getNumCols();    
		assert(numCols > index);
		upperBounds_ = new double [numCols];
	    }
	    
	    upperBounds_[index] = ub;
	}
    ///
    void setBranchedOn(int b) { branchedOn_ = b; }
    ///
    void setBranchedDir(int d) { branchedDir_ = d; }
    ///
    void setBranchedOnValue(double b) { branchedOnVal_ = b; }
    ///
    int getBranchedOn() const { return branchedOn_; }
    ///
    int getBranchedDir() const { return branchedDir_; }
    ///
    double getBranchedOnValue() const { return branchedOnVal_; }        
};

#endif
