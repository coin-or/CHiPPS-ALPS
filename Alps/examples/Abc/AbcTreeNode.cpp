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
 * Copyright (C) 2001-2006, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#include <cassert>
#include <iostream>
#include <utility>
#include <cmath>

#include "CoinUtility.hpp"
#include "OsiSolverInterface.hpp"
#include "OsiClpSolverInterface.hpp"
#include "OsiRowCut.hpp"
#include "OsiColCut.hpp"
#include "OsiRowCutDebugger.hpp"
#include "OsiCuts.hpp"

#include "AlpsKnowledge.h"
#include "AlpsEnumProcessT.h"

#include "AbcBranchActual.h"
#include "AbcMessage.h"
#include "AbcParams.h"
#include "AbcTreeNode.h"
#include "AbcSolution.h"

//#############################################################################

/// Infeasibility - large is 0.5
static double checkInfeasibility(AbcModel* model, const int columnNumber, 
				 int & preferredWay, double & otherWay)
{
    OsiSolverInterface * solver = model->solver();
    const double * solution = model->currentSolution();
    const double * lower = solver->getColLower();
    const double * upper = solver->getColUpper();
    double value = solution[columnNumber];
    value = max(value, lower[columnNumber]);
    value = min(value, upper[columnNumber]);
    /*printf("%d %g %g %g %g\n",columnNumber_,value,lower[columnNumber_],
    solution[columnNumber_],upper[columnNumber_]);*/
    double nearest = floor(value + 0.5);
    double integerTolerance = 
	model->getDblParam(AbcModel::AbcIntegerTolerance);
    if (nearest > value) 
	preferredWay = 1;
    else
	preferredWay = -1;
    otherWay = 1.0 - fabs(value - nearest);
    if (fabs(value - nearest) <= integerTolerance) 
	return 0.0;
    else
	return fabs(value - nearest);
}

//#############################################################################

AlpsTreeNode*
AbcTreeNode::createNewTreeNode(AlpsNodeDesc *&desc) const
{
    // Create a new tree node
    AbcNodeDesc* d = dynamic_cast<AbcNodeDesc*>(desc);
    AbcTreeNode* node = new AbcTreeNode(d);
    desc = 0;
    return(node);
}

//#############################################################################

int
AbcTreeNode::process(bool isRoot, bool rampUp)
{
    bool betterSolution = false;
    double bestValue = getKnowledgeBroker()->getIncumbentValue();
    double parentObjValue = getObjValue();
    double primalTolerance = 1.0e-7;
    bool cutDuringRampup;
    AlpsProcessType myType = getKnowledgeBroker()->getProcType();

    AbcModel *model=dynamic_cast<AbcModel *>(getKnowledgeBroker()->getModel());

    cutDuringRampup = true;

    if (rampUp) {
	if (myType == AlpsProcessTypeHub || myType == AlpsProcessTypeMaster){
	    cutDuringRampup=model->AbcPar()->entry(AbcParams::cutDuringRampup);
	}
    }
   

    // Check if can quit early. Becare the root.
    // REMOVE
    //if (parentObjValue < 1.0e99) bestValue = 3983;
    
    if (parentObjValue - primalTolerance > bestValue) {
	setStatus(AlpsNodeStatusFathomed);
	return false;
    }

    int i = -1;
    AbcNodeDesc *desc = dynamic_cast<AbcNodeDesc*>(desc_);
    AbcModel *m = dynamic_cast<AbcModel*>(desc->getModel());

    // Update cutoff if recv a better solution from other process
    if (bestValue < m->getCutoff()) {
	double cutoff = bestValue - 
	    m->getDblParam(AbcModel::AbcCutoffIncrement);
	m->setCutoff(cutoff);
    }

    //-------------------------------------------------------------------------
    // Report search status
    int nodeInterval = model->AbcPar()->entry(AbcParams::statusInterval);

    assert(nodeInterval > 0);

    if(m->getNodeCount() % nodeInterval == 0){
	const int nodeLeft = getKnowledgeBroker()->updateNumNodesLeft();
	m->messageHandler()->message(ABC_STATUS, m->messages())
	    << getKnowledgeBroker()->getProcRank() << (m->getNodeCount()) 
	    << nodeLeft <<  m->getCutoff() << parentObjValue << CoinMessageEol;
    }
     
    //-------------------------------------------------------------------------
    // Load and solve the lp relaxation.
    // (1) LP infeasible
    //     a. set status to be fathom
    // (2) LP feasible
    //     a. MILP feasible. Check whether need update incumbent.
    //     b. LP feasible but not MIP feasible. Check whether can be 
    //        fathomed, if not, choose a branch variable
    //-------------------------------------------------------------------------
    const int numCols = m->solver()->getNumCols();
    const double *lbs = desc->lowerBounds();
    const double *ubs = desc->upperBounds();

    for(i = 0; i < numCols; ++i) {
	m->solver()->setColBounds(i, lbs[i], ubs[i]);
    }
     
    //-------------------------------------------------------------
    OsiCuts cuts;
    int maximumCutPassesAtRoot = m->getMaximumCutPassesAtRoot();
    int numberOldActiveCuts = 0;
    int numberNewCuts = 0;
    int maximumWhich = 1000;
    int *whichGenerator = new int[maximumWhich];
    int heurFound = -1; 
 
    bool feasible = m->solveWithCuts(cuts, maximumCutPassesAtRoot,
				     NULL, numberOldActiveCuts, 
				     numberNewCuts, maximumWhich, 
				     whichGenerator, 
				     cutDuringRampup,
				     heurFound);
    
    m->setCurrentNumberCuts(m->currentNumberCuts() + numberNewCuts);
   
    // if ( (heurFound >= 0) && (m->getObjValue() < bestValue) ) {
    if (heurFound >= 0) {
	betterSolution = true;
	AbcSolution* sol = new AbcSolution(numCols, 
					   m->bestSolution(),
					   m->getObjValue());
	getKnowledgeBroker()->addKnowledge(ALPS_SOLUTION, sol, 
					   m->getObjValue());
    }
    

    //-------------------------------------------------------------
     
    if (!feasible) {
	setStatus(AlpsNodeStatusFathomed);
	setObjValue(-ALPS_OBJ_MAX);       // Remove it as soon as possilbe
    }
    else {
	double val = (m->getCurrentObjValue()) * (m->getObjSense());
	double xS = desc->getBranchedOnValue();
	int bDir = desc->getBranchedDir();
	int bInd = desc->getBranchedOn();
	
	/* Update pseudo: not for the root */
	if ((m->numberStrong() == 0) && (parent_ != 0) && (bInd >= 0)) {
	    /* FIXME: not sure which bInd < 0 */
	    // std::cout << "bInd=" << bInd << "; bVal=" << xS << std::endl;
	    (m->getPseudoList()[bInd])->update(bDir, quality_, val, xS);
	}
	
	int numberInfeasibleIntegers;
	if(m->feasibleSolution(numberInfeasibleIntegers)) { // MIP feasible
	    if (val < bestValue) {
		betterSolution = true;
		m->setBestSolution(ABC_BRANCHSOL,
				   val, m->getColSolution());
		AbcSolution* ksol = new AbcSolution(numCols, 
						    m->getColSolution(), 
						    val);
		getKnowledgeBroker()->addKnowledge(ALPS_SOLUTION, ksol, val); 
	    }
	    setStatus(AlpsNodeStatusFathomed);
	}
	else {
	    if (val < bestValue) {
		bool strongFound = false;
		int action = -1;
		while (action == -1) { 
		    if(getKnowledgeBroker()->getProcRank() == -1) {
			std::cout << "*** I AM RANK ONE: before choose:action = " << action
				  << std::endl;
		    }
		    action = chooseBranch(m, strongFound);
		    if ( (strongFound) && 
			 (m->getObjValue() < bestValue) ) {
			betterSolution = true;
			AbcSolution *sol = new AbcSolution(numCols, 
							   m->bestSolution(),
							   m->getObjValue());
			getKnowledgeBroker()->addKnowledge(ALPS_SOLUTION, sol, 
							   m->getObjValue());
			
		    }	
		    if (action == -1) { 
			feasible = m->resolve();
			//resolved = true ;
#if defined(ABC_DEBUG_MORE)
			printf("Resolve (root) as something fixed, Obj value %g %d rows\n",
			       m->solver()->getObjValue(),
			       m->solver()->getNumRows());
#endif
			if (!feasible) action = -2; 
		    }
		    if (action == -2) { 
			feasible = false; 
		    } 
		    if(getKnowledgeBroker()->getProcRank() == -1) {
			std::cout << "*** I AM RANK ONE: action = " << action
				  << std::endl;
		    }
		}
		assert(action != -1);


		if (action >= 0) {
		    const double * newLbs = m->getColLower();
		    const double * newUbs = m->getColUpper();
		    desc->setLowerBounds(newLbs, numCols);
		    desc->setUpperBounds(newUbs, numCols);
#if defined(ABC_DEBUG_MORE)
		    std::cout << "SetPregnant: branchedOn = " << branchedOn_ 
			      << "; index = " << index_ << std::endl;
#endif		    
		    setStatus(AlpsNodeStatusPregnant);
		}
		else if (action == -2) {
		    setStatus(AlpsNodeStatusFathomed);
		}
		else {
		    throw CoinError("No branch object found", "process", 
				    "AbcTreeNode");
		}
		
	    }
	    else {
		setStatus(AlpsNodeStatusFathomed);
	    }
	}
	quality_ = val;
    }

    m->takeOffCuts();

    delete [] whichGenerator;
    
    return betterSolution;
}

//#############################################################################

std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> >
AbcTreeNode::branch()
{
    AbcNodeDesc* desc = 
	dynamic_cast<AbcNodeDesc*>(desc_);
    AbcModel* m = dynamic_cast<AbcModel*>(desc->getModel());

    double* oldLbs = desc->lowerBounds();
    double* oldUbs = desc->upperBounds();
    const int numCols = m->getNumCols();
    assert(oldLbs && oldUbs && numCols);

    if ( (branchedOn_ < 0) || (branchedOn_ >= numCols) ) {
	std::cout << "AbcError: branchedOn_ = "<< branchedOn_ << "; numCols = "
		  << numCols << "; index_ = " << index_ << std::endl;
	throw CoinError("branch index is out of range", 
			"branch", "AbcTreeNode");
    }

    double* newLbs = new double[numCols];
    double* newUbs = new double[numCols];
    std::copy(oldLbs, oldLbs + numCols, newLbs);
    std::copy(oldUbs, oldUbs + numCols, newUbs);
    //memcpy(newLbs, oldLbs, sizeof(double) * numCols);
    //memcpy(newUbs, oldUbs, sizeof(double) * numCols);

    std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> > newNodes;

    double objVal;
    // if (getParent() == 0) { //ROOT
	objVal = getObjValue();
	//}
	//else {
	//objVal = std::min(getParent()->getObjValue(), getObjValue());
	//}
    
    // Branch down
    newLbs[branchedOn_] = oldLbs[branchedOn_];
    newUbs[branchedOn_] = floor(branchedOnVal_);//floor(branchedOnVal_+1.0e-5);
    AbcNodeDesc* child;
    assert(branchedOn_ >= 0);
    child = new AbcNodeDesc(m, newLbs, newUbs);
    child->setBranchedOn(branchedOn_);
    child->setBranchedOnValue(branchedOnVal_);
    child->setBranchedDir(-1);
    newNodes.push_back(CoinMakeTriple(static_cast<AlpsNodeDesc *>(child),
				      AlpsNodeStatusCandidate,
				      objVal));
    
    // Branch up
    newUbs[branchedOn_] = oldUbs[branchedOn_];
    newLbs[branchedOn_] = ceil(branchedOnVal_);//ceil(branchedOnVal_ - 1.0e-5);
    child = 0;
    child = new AbcNodeDesc(m, newLbs, newUbs);
    child->setBranchedOn(branchedOn_);
    child->setBranchedOnValue(branchedOnVal_);
    child->setBranchedDir(1);
    newNodes.push_back(CoinMakeTriple(static_cast<AlpsNodeDesc *>(child),
				      AlpsNodeStatusCandidate,
				      objVal));
    if (newLbs != 0) {
	delete [] newLbs;
	newLbs = 0;
    }
    if (newUbs != 0) {
	delete [] newUbs;
	newUbs = 0;
    }
    return newNodes;
}

//#############################################################################
#if 0
// Right now just choose the variable farest to be integral
int 
AbcTreeNode::chooseBranch(AbcModel* model, bool& strongFound) 
{
    strongFound = false;
    int i = -1;
    const int numInts = model->numberIntegers();
    const int* intVar = model->integerVariable();
    const double* sol = model->solver()->getColSolution();
    double maxDist = model->getDblParam(AbcModel::AbcIntegerTolerance);
    double value = 0.0;
    double nearest = 0.0;
    double distance = 0.0;

    branchedOn_ = -1;

    for (i = 0; i < numInts; ++i) {
	value = sol[intVar[i]];	
	double nearest = floor(value + 0.5);
	double distance = fabs(value - nearest);
	
	if (distance > maxDist) {
	    maxDist = distance;
	    branchedOn_ = intVar[i];
	    branchedOnVal_ = value;
	}
    }

    if (branchedOn_ == -1) {
	return -2;  // Should not happend
    }
    else {
	return 2;   // Find a branch variable
    }
}
#endif

//#############################################################################
//#############################################################################

AlpsEncoded*
AbcTreeNode::encode() const 
{
#if defined(ABC_DEBUG_MORE)
    std::cout << "AbcTreeNode::encode()--start to encode" << std::endl;
#endif
    AlpsEncoded* encoded = new AlpsEncoded("ALPS_NODE");
    AbcNodeDesc* desc = dynamic_cast<AbcNodeDesc*>(desc_);
    AbcModel* model = dynamic_cast<AbcModel*>(desc->getModel());

    int numCols = model->getNumCols();
    assert(numCols);
    
    const double * lb = desc->lowerBounds();
    const double * ub = desc->upperBounds();

    encoded->writeRep(explicit_);
    encoded->writeRep(numCols);
    encoded->writeRep(lb, numCols);
    encoded->writeRep(ub, numCols);
    encoded->writeRep(index_);
    encoded->writeRep(depth_);
    encoded->writeRep(quality_);
    encoded->writeRep(parentIndex_);
    encoded->writeRep(numChildren_);
    encoded->writeRep(status_);
    encoded->writeRep(sentMark_);
    encoded->writeRep(branchedOn_);
    encoded->writeRep(branchedOnVal_);

#if defined(ABC_DEBUG_MORE)
    std::cout << "numCols = " << numCols << "; ";
    for (int i = 0; i < numCols; ++i) {
	std::cout << "[" << lb[i] << "," << ub[i] <<"], ";
    }
    std::cout << std::endl;
    std::cout << "index_ = " << index_ << "; ";
    std::cout << "depth_ = " << depth_ << "; ";
    std::cout << "quality_ = " << quality_ << "; ";
    std::cout << "parentIndex_ = " << parentIndex_ << "; ";
    std::cout << "numChildren_ = " << numChildren_ << std::endl;
#endif

    return encoded;
}

AlpsKnowledge* 
AbcTreeNode::decode(AlpsEncoded& encoded) const 
{
    int expli;
    int numCols;
    double* lb;
    double* ub;
    int index;
    int depth;
    double objValue;
    int parentIndex;
    int numChildren;
    AlpsNodeStatus     nodeStatus;
    int sentMark;
    int branchedOn;
    double branchedOnVal;
    
    encoded.readRep(expli);  // Check whether has full or diff
    encoded.readRep(numCols);
    encoded.readRep(lb, numCols);
    encoded.readRep(ub, numCols);
    encoded.readRep(index);
    encoded.readRep(depth);
    encoded.readRep(objValue);
    encoded.readRep(parentIndex);
    encoded.readRep(numChildren);
    encoded.readRep(nodeStatus);
    encoded.readRep(sentMark);
    encoded.readRep(branchedOn);
    encoded.readRep(branchedOnVal);
    

    if (nodeStatus == AlpsNodeStatusPregnant) {
	assert(branchedOn >= 0 && branchedOn < numCols);
    }
	
    AbcNodeDesc* nodeDesc = new AbcNodeDesc(dynamic_cast<AbcModel*>
					    (desc_->getModel()), lb, ub);
 
    //  nodeDesc->setModel(getKnowledgeBroker()->getDataPool()->getModel());
    AbcTreeNode* treeNode = new AbcTreeNode(nodeDesc);
    treeNode->setIndex(index);
    treeNode->setDepth(depth);
    treeNode->setObjValue(objValue);
    treeNode->setParentIndex(parentIndex);
    treeNode->setParent(0);
    treeNode->setNumChildren(numChildren);
    treeNode->setStatus(nodeStatus);
    treeNode->setSentMark(sentMark);
    treeNode->setBranchedOn(branchedOn);
    treeNode->setBranchedOnValue(branchedOnVal);

    if(!lb) {
	delete [] lb;
	lb = 0;
    }
    if(!ub) {
	delete [] ub;
	ub = 0;
    }
    
#if defined(ABC_DEBUG_MORE)
    std::cout << "numCols = " << numCols << "; ";
    std::cout << "index = " << index << "; ";
    std::cout << "depth = " << depth << "; ";
    std::cout << "objValue = " << objValue<<"; ";
    std::cout << "parentIndex = " << parentIndex << "; ";
    std::cout << "status = " << nodeStatus << "; ";
    std::cout << "branchedOn = " << branchedOn << "; ";
    std::cout << "branchedOnVal = " << branchedOnVal << "; ";
    std::cout << "numChildren = " << numChildren << std::endl;
#endif

    return treeNode;
}

//#############################################################################
// This function tests each integer using strong branching and selects 
// the one with the least objective degradation. If strong branching is 
// disabled, the most infeasible integer is choosen
// Returns:     2  Find a branching integer
//             -1  Monotone
//	       -2  Infeasible
int AbcTreeNode::chooseBranch(AbcModel *model, bool& strongFound)
{ 
    int numberIntegerInfeasibilities;
    int anyAction = 0;
        
    strongFound = false;

    // Make sure we are talking about the same solver
    bool feasible = model->resolve(); 
    if (!feasible) {
	anyAction = -2;
	return anyAction;
    }

    OsiSolverInterface * solver = model->solver();	
    model->feasibleSolution(numberIntegerInfeasibilities);
    if (numberIntegerInfeasibilities <= 0) {
	double objectiveValue = 
	    solver->getObjSense() * solver->getObjValue();
	strongFound = model->setBestSolution(ABC_STRONGSOL,
					     objectiveValue,
					     solver->getColSolution());
	anyAction = -2;
	return anyAction;
    }
    
    double saveObjectiveValue = solver->getObjValue();
    double objectiveValue = solver->getObjSense() * saveObjectiveValue;
    
    if(getKnowledgeBroker()->getProcRank() == -1) {
	std::cout << "*** I AM RANK FIVE: obj = "<< objectiveValue
		  << ", cutoff = "<< model->getCutoff() << std::endl;
    }

    if (objectiveValue >=  model->getCutoff()) {
	anyAction = -2;
	return anyAction;
    }

    const double * lower = solver->getColLower();
    const double * upper = solver->getColUpper();

    double integerTolerance = 
	model->getDblParam(AbcModel::AbcIntegerTolerance);
    int i, j;
    bool beforeSolution = model->getSolutionCount()==0;
    int numberStrong = model->numberStrong();
    int numberObjects = model->numberIntegers();
    int maximumStrong = max(min(model->numberStrong(), numberObjects), 1);
    int numberColumns = model->getNumCols();
    double * saveUpper = new double[numberColumns];
    double * saveLower = new double[numberColumns];

    // Save solution in case heuristics need good solution later
    double * saveSolution = new double[numberColumns];
    memcpy(saveSolution,solver->getColSolution(),numberColumns*sizeof(double));

    // Get a branching decision object.
    AbcBranchDecision * decision = model->branchingMethod();
    if (!decision) {
	decision = new AbcBranchDefaultDecision();
    }
    
    typedef struct {
	int possibleBranch;   // branching integer variable
	double upMovement;    // cost going up (and initial away from feasible)
	double downMovement;  // cost going down
	int numIntInfeasUp;   // without odd ones
	int numObjInfeasUp;   // just odd ones
	bool finishedUp;      // true if solver finished
	int numItersUp;       // number of iterations in solver
	int numIntInfeasDown; // without odd ones
	int numObjInfeasDown; // just odd ones
	bool finishedDown;    // true if solver finished
	int numItersDown;     // number of iterations in solver
	int objectNumber;     // Which object it is
    } Strong;

    Strong * choice = new Strong[maximumStrong];
    for (i = 0; i < numberColumns; ++i) {
	saveLower[i] = lower[i];
	saveUpper[i] = upper[i];
    }

    //assert(model->getForcePriority() < 0);     // For hot start

    double saveOtherWay = 0.0;        // just for non-strong branching
    numberUnsatisfied_ = 0;
    int bestPriority = INT_MAX;

    // Scan for branching objects that indicate infeasibility. 
    // Choose the best maximumStrong candidates, using priority as the 
    // first criteria, then integer infeasibility.
    // The algorithm is to fill the choice array with a set of good 
    // candidates (by infeasibility) with priority bestPriority.  Finding 
    // a candidate with priority better (less) than bestPriority flushes 
    // the choice array. (This serves as initialization when the first 
    // candidate is found.)  When a candidate is added, 
    // it replaces the candidate with the smallest infeasibility (tracked by
    // iSmallest)
    int iSmallest;
    double mostAway;
    for (i = 0; i < maximumStrong; ++i) choice[i].possibleBranch = -1;
    numberStrong = 0;
    for (i = 0; i < numberObjects; ++i) {
	const int object = model->integerVariable()[i];
	int preferredWay;
	double otherWay;
	double infeasibility = checkInfeasibility(model, object, 
						  preferredWay, otherWay);
	if (infeasibility > integerTolerance) {
	    if (model->numberStrong() == 0) {  /* pseudo cost branching */
		model->getPseudoIndices()[numberUnsatisfied_] = object;
	    }
	    ++numberUnsatisfied_;
	    int priorityLevel = model->priority(i);
	    if (priorityLevel < bestPriority) {
		int j;
		iSmallest = 0;
		for (j = 0; j < maximumStrong; ++j) {
		    choice[j].upMovement = 0.0;
		    choice[j].possibleBranch = -1;
		}
		bestPriority = priorityLevel;
		mostAway = integerTolerance;
		numberStrong = 0;
	    } 
	    else if (priorityLevel > bestPriority) {
		continue;
	    }
      
	    // Check for suitability based on infeasibility.
	    if (infeasibility > mostAway) {
		choice[iSmallest].upMovement = infeasibility;
		choice[iSmallest].downMovement = otherWay;
		saveOtherWay = otherWay;
		choice[iSmallest].possibleBranch = object;
		numberStrong = max(numberStrong, iSmallest + 1);
		choice[iSmallest].objectNumber = i;
		int j;
		iSmallest = -1;
		mostAway = 1.0e50;
		for (j = 0; j < maximumStrong; ++j) {
		    if (choice[j].upMovement < mostAway) {
			mostAway = choice[j].upMovement;
			iSmallest = j;
		    }
		}
	    }
	}
    }

#if 0
    if (numberStrong == 0) {  // FIXME: Should not happen
	bool fea = model->feasibleSolution(numberIntegerInfeasibilities);
	if (fea) {
	    double objectiveValue = 
		solver->getObjSense() * solver->getObjValue();
	    strongFound = model->setBestSolution(ABC_STRONGSOL,
						 objectiveValue,
						 solver->getColSolution());
	}
	//abort();
	if (!model->branchingMethod()) delete decision;
	delete [] choice;
	delete [] saveLower;
	delete [] saveUpper;
	// restore solution
	solver->setColSolution(saveSolution);
	delete [] saveSolution;
	anyAction = -2;
	return anyAction;
    }
#endif    

    // Some solvers can do the strong branching calculations faster if
    // they do them all at once.  At present only Clp does for ordinary
    // integers but I think this coding would be easy to modify
    bool allNormal = true; // to say if we can do fast strong branching
    for (i = 0; i < numberStrong; ++i) {
	choice[i].numIntInfeasUp = numberUnsatisfied_;
	choice[i].numIntInfeasDown = numberUnsatisfied_;
    }

    // Setup for strong branching involves saving the current basis (for 
    // restoration afterwards) and setting up for hot starts.
    if (model->numberStrong() >0) {
	// set true to say look at all even if some fixed (experiment)
	bool solveAll = false; 
	// Save basis
	CoinWarmStart * ws = solver->getWarmStart();
	// save limit
	int saveLimit;
	solver->getIntParam(OsiMaxNumIterationHotStart, saveLimit);
	if (beforeSolution)  // go to end
	    solver->setIntParam(OsiMaxNumIterationHotStart, 10000); 

	// If we are doing all strong branching in one go then we create 
	// new arrays to store information. If clp NULL then doing old way.
	// Going down -
	//  outputSolution[2*i] is final solution.
	//  outputStuff[2*i] is status (0 - finished, 1 infeas, other unknown)
	//  outputStuff[2*i+numberStrong] is number iterations
	//  On entry newUpper[i] is new upper bound, on exit obj change
	// Going up -
	//  outputSolution[2*i+1] is final solution.
	//  outputStuff[2*i+1] is status (0 - finished, 1 infeas, other unknown
	//  outputStuff[2*i+1+numberStrong] is number iterations
	//  On entry newLower[i] is new lower bound, on exit obj change
	OsiClpSolverInterface * osiclp = 
	    dynamic_cast< OsiClpSolverInterface*>(solver);
	ClpSimplex * clp=NULL;
	double * newLower = NULL;
	double * newUpper = NULL;
	double ** outputSolution = NULL;
	int * outputStuff = NULL;
	int saveLogLevel;

	//allNormal=false;
	if (osiclp && allNormal) {
	    clp = osiclp->getModelPtr();
	    saveLogLevel = clp->logLevel();
	    int saveMaxIts = clp->maximumIterations();
	    clp->setMaximumIterations(saveLimit);
	    clp->setLogLevel(0);
	    newLower = new double[numberStrong];
	    newUpper = new double[numberStrong];
	    outputSolution = new double * [2 * numberStrong];
	    outputStuff = new int [4 * numberStrong];
	    int * which = new int [numberStrong];
	    for (i = 0; i < numberStrong; ++i) {
		int iObject = choice[i].objectNumber;
		int iSequence = model->integerVariable()[iObject];
		newLower[i] = ceil(saveSolution[iSequence]);
		newUpper[i] = floor(saveSolution[iSequence]);
		which[i] = iSequence;
		outputSolution[2*i] = new double [numberColumns];
		outputSolution[2*i+1] = new double [numberColumns];
	    }
	    clp->strongBranching(numberStrong, which,
				 newLower, newUpper, 
				 outputSolution, outputStuff,
				 outputStuff + 2 * numberStrong,
				 !solveAll, false);
	    clp->setMaximumIterations(saveMaxIts);
	    delete [] which;
	} 
	else { // Doing normal way
	    solver->markHotStart();
	}

	//---------------------------------------------------------------------
	// Open a loop to do the strong branching LPs. For each candidate
	// variable, solve an LP with the variable forced down, then up. 
	// If a direction turns out to be infeasible or monotonic (i.e., 
	// over the dual objective cutoff), force the objective change to
	// be big (1.0e100). If we determine the problem is infeasible, 
	// or find a monotone variable, escape the loop.

	for (i = 0; i < numberStrong; ++i) { 
	    double objectiveChange ;
	    double newObjectiveValue = 1.0e100;
	    // status is 0 finished, 1 infeasible and other
	    int iStatus;
	    int colInd = choice[i].possibleBranch;

	    // Try the down direction first.
	    if (!clp) {
		solver->setColUpper(colInd, floor(saveSolution[colInd]));
		solver->solveFromHotStart();
		// restore bounds
		for (int j = 0; j < numberColumns; ++j) {
		    if (saveLower[j] != lower[j])
			solver->setColLower(j, saveLower[j]);
		    if (saveUpper[j] != upper[j])
			solver->setColUpper(j, saveUpper[j]);
		}

		if (solver->isProvenOptimal())
		    iStatus = 0; // optimal
		else if (solver->isIterationLimitReached()
			 &&!solver->isDualObjectiveLimitReached())
		    iStatus = 2; // unknown 
		else
		    iStatus = 1; // infeasible
		newObjectiveValue = 
		    solver->getObjSense() * solver->getObjValue();
		choice[i].numItersDown = solver->getIterationCount();
		//objectiveChange = newObjectiveValue - objectiveValue ;
	    } 
	    else {
		iStatus = outputStuff[2*i];
		choice[i].numItersDown = outputStuff[2*numberStrong + 2*i];
		newObjectiveValue = objectiveValue + newUpper[i];
		solver->setColSolution(outputSolution[2*i]);
	    }
	    objectiveChange = newObjectiveValue - objectiveValue;
	    if (!iStatus) {
		choice[i].finishedDown = true;
		if (newObjectiveValue > model->getCutoff()) {
		    objectiveChange = 1.0e100;          // discard it
		} 
		else {
		    // See if integer solution
		    if (model->feasibleSolution(choice[i].numIntInfeasDown)){
			strongFound = 
			    model->setBestSolution(ABC_STRONGSOL,
						   newObjectiveValue,
						   solver->getColSolution());
			if (newObjectiveValue > model->getCutoff())
			    objectiveChange = 1.0e100;  // discard it
		    }
		}
	    } 
	    else if (iStatus == 1) {
		objectiveChange = 1.0e100;
	    } 
	    else {   		// Can't say much as we did not finish
		choice[i].finishedDown = false ;
	    }
	    choice[i].downMovement = objectiveChange ;
	    
	    // repeat the whole exercise, forcing the variable up
	    if (!clp) {
		solver->setColLower(colInd, ceil(saveSolution[colInd]));
		solver->solveFromHotStart();
		// restore bounds
		for (int j = 0; j < numberColumns; j++) {
		    if (saveLower[j] != lower[j])
			solver->setColLower(j, saveLower[j]);
		    if (saveUpper[j] != upper[j])
			solver->setColUpper(j, saveUpper[j]);
		}
		if (solver->isProvenOptimal())
		    iStatus = 0; // optimal
		else if (solver->isIterationLimitReached()
			 &&!solver->isDualObjectiveLimitReached())
		    iStatus = 2; // unknown 
		else
		    iStatus = 1; // infeasible
		newObjectiveValue = 
		    solver->getObjSense() * solver->getObjValue();
		choice[i].numItersUp = solver->getIterationCount();
		objectiveChange = newObjectiveValue - objectiveValue ;
	    } 
	    else {
		iStatus = outputStuff[2 * i + 1];
		choice[i].numItersUp = outputStuff[2*numberStrong + 2*i + 1];
		newObjectiveValue = objectiveValue + newLower[i];
		solver->setColSolution(outputSolution[2*i + 1]);
	    }
	    objectiveChange = newObjectiveValue - objectiveValue;
	    if (!iStatus) {
		choice[i].finishedUp = true;
		if (newObjectiveValue > model->getCutoff()) {
		    objectiveChange = 1.0e100;
		} 
		else {
		    // See if integer solution
		    if (model->feasibleSolution(choice[i].numIntInfeasUp)){
			strongFound = 
			    model->setBestSolution(ABC_STRONGSOL,
						   newObjectiveValue,
						   solver->getColSolution());
			if (newObjectiveValue > model->getCutoff())
			    objectiveChange = 1.0e100;
		    }
		}
	    } 
	    else if (iStatus == 1) {
		objectiveChange = 1.0e100;
	    } 
	    else {
		// Can't say much as we did not finish
		choice[i].finishedUp = false;
	    }
	    choice[i].upMovement = objectiveChange;

	    // End of evaluation for this candidate variable. Possibilities 
	    // are: 
	    // * Both sides below cutoff; this variable is a candidate 
	    //   for branching.
	    // * Both sides infeasible or above the objective cutoff: 
	    //   no further action here. Break from the evaluation loop and 
	    //   assume the node will be purged by the caller.
	    // * One side below cutoff: Install the branch (i.e., fix the 
	    //   variable). Break from the evaluation loop and assume 
	    //   the node will be reoptimised by the caller.

	    AbcNodeDesc* desc = dynamic_cast<AbcNodeDesc*>(desc_);
	    int monoIndex;	    

	    if (choice[i].upMovement < 1.0e100) {
		if(choice[i].downMovement < 1.0e100) {
		    anyAction = 2;   // Both
		} 
		else {               // Only up 
		    anyAction = -1;  
		    monoIndex = choice[i].possibleBranch;
		    solver->setColLower(monoIndex,
					ceil(saveSolution[monoIndex]));
		    desc->setLowerBound(monoIndex, 
					ceil(saveSolution[monoIndex]));
		    break;
		}
	    } 
	    else {
		if(choice[i].downMovement < 1.0e100) {
		    anyAction = -1;  // Only down
		    monoIndex = choice[i].possibleBranch;
		    solver->setColUpper(monoIndex,
					floor(saveSolution[monoIndex]));
		    desc->setUpperBound(monoIndex, 
					floor(saveSolution[monoIndex]));
		    break;
		} 
		else {                // neither
		    anyAction = -2;
		    break;
		}
	    }
	}
	
	if (!clp) {
	    solver->unmarkHotStart();
	} else {
	    clp->setLogLevel(saveLogLevel);
	    delete [] newLower;
	    delete [] newUpper;
	    delete [] outputStuff;
	    for (int i = 0; i < 2*numberStrong; ++i)
		delete [] outputSolution[i];
	    delete [] outputSolution;
	}

	solver->setIntParam(OsiMaxNumIterationHotStart,saveLimit);
	solver->setWarmStart(ws);	// restore basis
	delete ws;

	if(getKnowledgeBroker()->getProcRank() == -1) {
	    std::cout << "*** I AM RANK ONE: chooseBranch():anyAction = " 
		      << anyAction << "numberStrong = "<< numberStrong
		      << std::endl;
	}

	// Sift through the candidates for the best one.
	// QUERY: Setting numberNodes looks to be a distributed noop. 
	// numberNodes is local to this code block. Perhaps should be 
	// numberNodes_ from model?
	// Unclear what this calculation is doing.
	if (anyAction > 0) {
	    int numberNodes = model->getNodeCount();
	    // get average cost per iteration and assume stopped ones
	    // would stop after 50% more iterations at average cost??? !!! ???
	    double averageCostPerIteration = 0.0;
	    double totalNumberIterations = 1.0;
	    int smallestNumberInfeasibilities = INT_MAX;
	    for (i = 0; i < numberStrong; ++i) {
		totalNumberIterations += choice[i].numItersDown +
		    choice[i].numItersUp;
		averageCostPerIteration += choice[i].downMovement +
		    choice[i].upMovement;
		smallestNumberInfeasibilities = 
		    min(min(choice[i].numIntInfeasDown,
			    choice[i].numIntInfeasUp),
			smallestNumberInfeasibilities);
	    }
	    if (smallestNumberInfeasibilities >= numberIntegerInfeasibilities)
		numberNodes = 1000000; // switch off search for better solution
	    numberNodes = 1000000;     // switch off anyway
	    averageCostPerIteration /= totalNumberIterations;
	    // all feasible - choose best bet

	    // New method does all at once so it can be more sophisticated
	    // in deciding how to balance actions.
	    // But it does need arrays
	    double * changeUp = new double [numberStrong];
	    int * numberInfeasibilitiesUp = new int [numberStrong];
	    double * changeDown = new double [numberStrong];
	    int * numberInfeasibilitiesDown = new int [numberStrong];
	    int * objects = new int [numberStrong];

	    for (i = 0; i < numberStrong; ++i) {

		int iColumn = choice[i].possibleBranch;

		model->messageHandler()->message(ABC_STRONG,model->messages())
		    << i << iColumn
		    << choice[i].downMovement << choice[i].numIntInfeasDown 
		    << choice[i].upMovement << choice[i].numIntInfeasUp 
		    << choice[i].possibleBranch
		    << CoinMessageEol;

		changeUp[i] = choice[i].upMovement;
		numberInfeasibilitiesUp[i] = choice[i].numIntInfeasUp;
		changeDown[i] = choice[i].downMovement;
		numberInfeasibilitiesDown[i] = choice[i].numIntInfeasDown;
		objects[i] = choice[i].possibleBranch;
	    }
	    int whichObject = 
		decision->bestBranch(model,
				     objects, numberStrong,
				     numberUnsatisfied_, changeUp,
				     numberInfeasibilitiesUp, changeDown,
				     numberInfeasibilitiesDown,objectiveValue);

	    // move branching object and make sure it will not be deleted
	    if (whichObject >= 0) {
		branchedOn_ = objects[whichObject];
		branchedOnVal_= saveSolution[branchedOn_];
		assert(branchedOn_ >= 0 && branchedOn_ < numberColumns);

#if 0		// FIXME
		std::cout << "AbcStrongBranch: col index : ";
		for (i = 0; i < numberStrong; ++i) {
		    std::cout<< objects[i] << " ";
		}
		std::cout <<"\nAbcStrongBranch: branchedOn_ = " << branchedOn_ 
			  << "; branchedOnVal_ = " << branchedOnVal_ 
			  << std::endl;
#endif
	    }
	    else {
		std::cout << "AbcError: whichObject = " << whichObject 
			  << "; numberStrong = " << numberStrong << std::endl;
		throw CoinError("bestBranch find nothing", 
				"chooseBranch", 
				"AbcTreeNode");
	    }

	    delete [] changeUp;
	    delete [] numberInfeasibilitiesUp;
	    delete [] changeDown;
	    delete [] numberInfeasibilitiesDown;
	    delete [] objects;
	}
	
	if (anyAction == 0){
	    throw CoinError("Can't find candidates", 
			    "chooseBranch", 
			    "AbcTreeNode");
	}
    }
    else {  // pseudocost branching
#if 1
	int object;
	int mostInd = -1;
	double mostDeg = -1.0;
	double deg, upCost, downCost, fraction;
	AbcPseudocost *pseudoC;
	
	for(j = 0; j < numberUnsatisfied_; ++j) {
	    //for(j = 0; j < numberStrong; ++j) {
	    object = model->getPseudoIndices()[j];
	    //object = choice[j].possibleBranch;
	    fraction = saveSolution[object] - floor(saveSolution[object]);
	    pseudoC = model->getPseudoList()[object];
	    upCost = pseudoC->upCost_ * (1.0 - fraction);
	    downCost = pseudoC->downCost_ * fraction;
	    deg = 4.0 * std::min(upCost, downCost) +
		1.0 * std::max(upCost, downCost);
	    if (deg > mostDeg) {
		mostDeg = deg;
		mostInd = object;
	    }
	}
	
	branchedOn_ = mostInd;
#else
	branchedOn_ = choice[0].possibleBranch;
#endif	
	branchedOnVal_= saveSolution[branchedOn_];
	assert( (branchedOn_ >= 0) && (branchedOn_ < numberColumns));
    }

    if (!model->branchingMethod()) delete decision;

    delete [] choice;
    delete [] saveLower;
    delete [] saveUpper;

    // restore solution
    solver->setColSolution(saveSolution);
    delete [] saveSolution;

    return anyAction;
}

