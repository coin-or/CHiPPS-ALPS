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
 * Copyright (C) 2001-2007, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

//#############################################################################
// This file is modified from SbbBranchBase.cpp
//#############################################################################

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif
#include <cassert>
#include <cmath>
#include <cfloat>

#include "AbcBranchBase.h"

//#############################################################################
//#############################################################################

// Default Constructor 
AbcBranchDecision::AbcBranchDecision ()
{
}

AbcBranchDecision::~AbcBranchDecision()
{
}

// Compare N branching objects. Return index of best and sets way of 
// branching in chosen object. This routine is used only after strong 
// branching. This is reccommended version as it can be more sophisticated
int
AbcBranchDecision::bestBranch ( AbcModel* model,
				int* objects, 
				int numberObjects,
				int numberUnsatisfied,
				double * changeUp, 
				int * numberInfeasibilitiesUp,
				double * changeDown, 
				int * numberInfeasibilitiesDown,
				double objectiveValue ) 
{
    int bestWay = 0;
    int whichObject = -1;
    int i;
    
    if (numberObjects) {
	initialize(model); 
	int bestObject = -1;
	for (i = 0; i < numberObjects; ++i) {
	    int betterWay = betterBranch(objects[i],
					 bestObject,
					 changeUp[i],
					 numberInfeasibilitiesUp [i],
					 changeDown[i],
					 numberInfeasibilitiesDown[i]);
	    if (betterWay) {
		bestObject = objects[i];
		bestWay = betterWay;
		whichObject = i;
	    }
	}
	// set way in best
	//bestObject->way(bestWay);
    }
    return whichObject;
}
