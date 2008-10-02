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
 * Copyright (C) 2001-2008, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

//#############################################################################
// This file is modified from SbbBranchActual.cpp
//#############################################################################

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif
#include <cassert>
#include <cmath>
#include <cfloat>

#include "CoinSort.hpp"
#include "OsiSolverInterface.hpp"

#include "AbcModel.h"
#include "AbcMessage.h"
#include "AbcBranchActual.h"

//#############################################################################
//#############################################################################

// Default Constructor 
AbcBranchDefaultDecision::AbcBranchDefaultDecision()
    :
    AbcBranchDecision()
{
    model_ = NULL;
    bestCriterion_ = 0.0;
    bestChangeUp_ = 0.0;
    bestNumberUp_ = 0;
    bestChangeDown_ = 0.0;
    bestNumberDown_ = 0;
    bestObject_ = -1;
}

// Copy constructor 
AbcBranchDefaultDecision::AbcBranchDefaultDecision (
    const AbcBranchDefaultDecision & rhs)
    :
    AbcBranchDecision()
{
    model_ = rhs.model_;
    bestCriterion_ = rhs.bestCriterion_;
    bestChangeUp_ = rhs.bestChangeUp_;
    bestNumberUp_ = rhs.bestNumberUp_;
    bestChangeDown_ = rhs.bestChangeDown_;
    bestNumberDown_ = rhs.bestNumberDown_;
    bestObject_ = rhs.bestObject_;
}

AbcBranchDefaultDecision::~AbcBranchDefaultDecision()
{
}

// Clone
AbcBranchDecision * 
AbcBranchDefaultDecision::clone() const
{
    return new AbcBranchDefaultDecision(*this);
}

// Initialize i.e. before start of choosing at a node
void 
AbcBranchDefaultDecision::initialize(AbcModel * model)
{
    model_ = model;
    bestCriterion_ = 0.0;
    bestChangeUp_ = 0.0;
    bestNumberUp_ = 0;
    bestChangeDown_ = 0.0;
    bestNumberDown_ = 0;
    bestObject_ = -1;
}

// Simple default decision algorithm. Compare based on infeasibility 
// (numInfUp, numInfDn) until a solution is found by search, then switch 
// to change in objective (changeUp, changeDn). Note that bestSoFar is 
// remembered in bestObject_, so the parameter bestSoFar is unused.
int
AbcBranchDefaultDecision::betterBranch(int thisOne, int bestSoFar,
				       double changeUp, int numInfUp,
				       double changeDn, int numInfDn)
{
    bool beforeSolution = model_->getSolutionCount() ==
	model_->getNumberHeuristicSolutions();
    int betterWay = 0;
    if (beforeSolution) {
	if (bestObject_ < 0) {
	    bestNumberUp_ = COIN_INT_MAX;
	    bestNumberDown_ = COIN_INT_MAX;
	}
	
	// before solution - choose smallest number could add in depth as well
	int bestNumber = std::min(bestNumberUp_, bestNumberDown_);
	if (numInfUp < numInfDn) {
	    if (numInfUp < bestNumber) {
		betterWay = 1;
	    } else if (numInfUp == bestNumber) {
		if (changeUp < bestCriterion_)
		    betterWay = 1;
	    }
	} else if (numInfUp > numInfDn) {
	    if (numInfDn < bestNumber) {
		betterWay = -1;
	    } else if (numInfDn == bestNumber) {
		if (changeDn < bestCriterion_)
		    betterWay = -1;
	    }
	} else {
	    // up and down have same number
	    bool better = false;
	    if (numInfUp < bestNumber) {
		better = true;
	    } else if (numInfUp == bestNumber) {
		if (std::min(changeUp, changeDn) < bestCriterion_)
		    better = true;
	    }
	    if (better) {
		// see which way
		if (changeUp <= changeDn)
		    betterWay = 1;
		else
		    betterWay = -1;
	    }
	}
    } 
    else {        // got a solution
	if (bestObject_ < 0) {
	    bestCriterion_ = -1.0;
	}
	if (changeUp <= changeDn) {
	    if (changeUp > bestCriterion_)
		betterWay = 1;
	} 
	else {
	    if (changeDn > bestCriterion_)
		betterWay = -1;
	}
    }

    if (betterWay) {
	bestCriterion_ = std::min(changeUp, changeDn);
	bestChangeUp_ = changeUp;
	bestNumberUp_ = numInfUp;
	bestChangeDown_ = changeDn;
	bestNumberDown_ = numInfDn;
	bestObject_ = thisOne;
    }

    return betterWay;
}

void 
AbcPseudocost::update(const int dir,
		      const double parentObjValue,
		      const double objValue,
		      const double solValue)
{
    double fraction = solValue - floor(solValue);
    double objDiff = objValue - parentObjValue;
    double cost;
    
    if (dir == 1) {
	cost = objDiff / (fraction + 1.0e-9);
	upCost_ = (upCost_ * upNum_ + cost) / (upNum_ + 1);
	++upNum_;
    }
    else if (dir == -1) {
	cost = objDiff / (1.0 - fraction + 1.0e-9);
	downCost_ = (downCost_ * downNum_ + cost) / (downNum_ + 1);
	++downNum_;
    }
    else {
	abort();
    }
}
