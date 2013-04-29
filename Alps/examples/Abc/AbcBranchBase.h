/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Eclipse Public License as part of the       *
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
 * Copyright (C) 2001-2013, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#ifndef AbcBranchBase_h_
#define AbcBranchBase_h_

//#############################################################################
// This file is modified from SbbBranchBase.hpp
//#############################################################################

#include <string>
#include <vector>

class OsiSolverInterface;

class AbcModel;
class AbcNode;
class AbcNodeDesc;
class AbcBranchingObject;

//#############################################################################

/** Abstract branching decision base class

    In the abstract, an AbcBranchDecision object is expected to be able to
    compare two possible branching choices.

    The #betterBranch() method is the crucial routine. It is expected to be 
    able to compare two integer variables.
*/
class AbcBranchDecision {
 public:
    /// Default Constructor 
    AbcBranchDecision ();

    /// Destructor
    virtual ~AbcBranchDecision();

    /// Clone
    virtual AbcBranchDecision * clone() const = 0;

    /// Initialize <i>e.g.</i> before starting to choose a branch at a node
    virtual void initialize(AbcModel * model) = 0;

    /** \brief Compare two branching objects (current just integer variables). 
	Return nonzero if branching using \p thisOne is better than 
	branching using \p bestSoFar.
    
	If \p bestSoFar is NULL, the routine should return a nonzero value.
	This routine is used only after strong branching.

	It is now reccommended that bestBranch is used - see below.
	This has been left for compatibility.
    */
    virtual int
	betterBranch(int thisOne,
		     int bestSoFar,
		     double changeUp, 
		     int numberInfeasibilitiesUp,
		     double changeDown, 
		     int numberInfeasibilitiesDown) = 0 ;

    /** \brief Compare N branching objects. Return index of best
	and sets way of branching in chosen object.
    
	This routine is used only after strong branching.
	This is reccommended version as it can be more sophisticated
    */
    virtual int	bestBranch ( AbcModel* model,
			     int* objects, 
			     int numberObjects, 
			     int numberUnsatisfied,
			     double * changeUp, 
			     int * numberInfeasibilitiesUp,
			     double * changeDown, 
			     int * numberInfeasibilitiesDown,
			     double objectiveValue );
    
 private:
    
    /// Assignment is illegal
    AbcBranchDecision & operator=(const AbcBranchDecision& rhs);
    
};

#endif
