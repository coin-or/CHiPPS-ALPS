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

#ifndef AbcBranchActual_h_
#define AbcBranchActual_h_

//#############################################################################
// This file is modified from SbbBranchActual.hpp
//#############################################################################

#include "AbcBranchBase.h"

//#############################################################################
/** Branching decision default class

  This class implements a simple default algorithm
  (betterBranch()) for choosing a branching variable. 
*/
class AbcBranchDefaultDecision : public AbcBranchDecision {

 public:
    // Default Constructor 
    AbcBranchDefaultDecision();

    // Copy constructor 
    AbcBranchDefaultDecision(const AbcBranchDefaultDecision &);
    
    virtual ~AbcBranchDefaultDecision();

    /// Clone
    virtual AbcBranchDecision * clone() const;

    /// Initialize, <i>e.g.</i> before the start of branch selection at a node
    virtual void initialize(AbcModel * model);

    /** \brief Compare two branching objects. Return nonzero if \p thisOne is
	better than \p bestSoFar.

	The routine compares branches using the values supplied in 
	\p numInfUp and \p numInfDn until a solution is found by search, 
	after which it uses the values supplied in \p changeUp and 
	\p changeDn. The best branching object seen so far and the 
	associated parameter values are remembered in the
	\c AbcBranchDefaultDecision object. The nonzero return value is +1 
	if the up branch is preferred, -1 if the down branch is preferred.
	
	As the names imply, the assumption is that the values supplied for
	\p numInfUp and \p numInfDn will be the number of infeasibilities 
	reported by the branching object, and \p changeUp and \p changeDn 
	will be the estimated change in objective. Other measures can be 
	used if desired.

	Because an \c AbcBranchDefaultDecision object remembers the current 
	best branching candidate (#bestObject_) as well as the values used 
	in the comparison, the parameter \p bestSoFar is redundant, hence 
	unused.
    */
    virtual int betterBranch(int thisOne,
			     int bestSoFar,
			     double changeUp, int numInfUp,
			     double changeDn, int numInfDn);

 private:
  
    /// Illegal Assignment operator 
    AbcBranchDefaultDecision & operator=(const AbcBranchDefaultDecision& rhs);

    /// data
    /// Point to the model
    AbcModel * model_;

    /// "best" so far
    double bestCriterion_;

    /// Change up for best
    double bestChangeUp_;

    /// Number of infeasibilities for up
    int bestNumberUp_;

    /// Change down for best
    double bestChangeDown_;

    /// Number of infeasibilities for down
    int bestNumberDown_;

    /// Index of the best branching integer variable
    int bestObject_;
};


class AbcPseudocost 
{
 public:
    int colInd_;
    double upCost_;
    int upNum_;
    double downCost_;
    int downNum_;

 public:
    AbcPseudocost() 
	: 
	colInd_(-1), 
	upCost_(0.0), upNum_(0), 
	downCost_(0.0), downNum_(0) {}

    AbcPseudocost(const int ind, 
		  const double uc, 
		  const int un,
		  const double dc, 
		  const int dn)
	:
	colInd_(ind), 
	upCost_(uc), upNum_(un),
	downCost_(dc), downNum_(dn) {}
    
    void update(const int dir,
		const double parentObjValue,
		const double objValue,
		const double solValue);
    
};

#endif
