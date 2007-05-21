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
// This file is modified from SbbModel.cpp
//#############################################################################

#include <iostream>

#include "OsiRowCut.hpp"
#include "OsiColCut.hpp"
#include "OsiRowCutDebugger.hpp"

#include "AlpsTreeNode.h"

#include "AbcCutGenerator.h"
#include "AbcHeuristic.h"
#include "AbcMessage.h"
#include "AbcModel.h"
#include "AbcTreeNode.h"
#include "AbcNodeDesc.h"


//#############################################################################
//#############################################################################

void
AbcModel::initialSolve()
{
    assert (solver_);
    solver_->initialSolve();
}

//#############################################################################
//  Parameters:
//  cuts:	 (o) all cuts generated in this round of cut generation
//  numberTries: (i) the maximum number of iterations for this round of cut
//                   generation; no a priori limit if 0
//  whichGenerator: (i/o) whichGenerator[i] is loaded with the index of the
//                        generator that produced cuts[i]; reallocated as
//                        required
//  numberOldActiveCuts: (o) the number of active cuts at this node from
//                           previous rounds of cut generation
//  numberNewCuts:       (o) the number of cuts produced in this round of cut
//                           generation
//  maximumWhich:      (i/o) capacity of whichGenerator; may be updated if
//                           whichGenerator grows.
//  cutDuringRampup:    (i) Whether generating cuts during rampup
//  found: (o)  great than 0 means that heuristics found solutions;
//              otherwise not. 
bool 
AbcModel::solveWithCuts(OsiCuts & cuts, int numberTries, 
			AbcTreeNode * node, int & numberOldActiveCuts, 
			int & numberNewCuts, int & maximumWhich, 
			int *& whichGenerator, const bool cutDuringRampup,
			int & found)
{
    found = -10;
    bool feasible;
    int lastNumberCuts = 0;
    double lastObjective = -1.0e100 ;
    int violated = 0;
    int numberRowsAtStart = solver_->getNumRows();
    int numberColumns = solver_->getNumCols();

    numberOldActiveCuts = numberRowsAtStart - numberRowsAtContinuous_;
    numberNewCuts = 0;
    
    feasible = resolve(); 
    if(!feasible) {
	return false;  // If lost feasibility, bail out right now
    }
    
    reducedCostFix();
    const double *lower = solver_->getColLower();
    const double *upper = solver_->getColUpper();
    const double *solution = solver_->getColSolution();

    double minimumDrop = minimumDrop_;
    if (numberTries < 0) { 
	numberTries = -numberTries;
	minimumDrop = -1.0; 
    }

    //-------------------------------------------------------------------------
    // Is it time to scan the cuts in order to remove redundant cuts? If so, 
    // set up to do it.
# define SCANCUTS 100  
    int *countColumnCuts = NULL;
    int *countRowCuts = NULL;
    bool fullScan = false;
    if ((numberNodes_ % SCANCUTS) == 0) {
	fullScan = true;
	countColumnCuts = new int[numberCutGenerators_ + numberHeuristics_];
	countRowCuts = new int[numberCutGenerators_ + numberHeuristics_];
	memset(countColumnCuts, 0,
	       (numberCutGenerators_ + numberHeuristics_) * sizeof(int));
	memset(countRowCuts, 0,
	       (numberCutGenerators_ + numberHeuristics_) * sizeof(int));
    }

    double direction = solver_->getObjSense();
    double startObjective = solver_->getObjValue() * direction;

    int numberPasses = 0;
    double primalTolerance = 1.0e-7;

    //-------------------------------------------------------------------------
    // Start cut generation loop 
    do {
	numberPasses++;
	numberTries--;
	OsiCuts theseCuts;
    
	// First check if there are cuts violated in global cut pool 
	if (numberPasses == 1 && howOftenGlobalScan_ > 0 &&
	    (numberNodes_ % howOftenGlobalScan_) == 0) { 
	    int numberCuts = globalCuts_.sizeColCuts();
	    int i;
	    for ( i = 0; i < numberCuts; ++i) { 
		const OsiColCut *thisCut = globalCuts_.colCutPtr(i);
		if (thisCut->violated(solution) > primalTolerance) {
		    printf("Global cut added - violation %g\n",
			   thisCut->violated(solution));
		    theseCuts.insert(*thisCut);
		}
	    }
	    numberCuts = globalCuts_.sizeRowCuts();
	    for ( i = 0; i < numberCuts; ++i) {
		const OsiRowCut * thisCut = globalCuts_.rowCutPtr(i);
		if (thisCut->violated(solution) > primalTolerance) {
		    printf("Global cut added - violation %g\n",
			   thisCut->violated(solution));
		    theseCuts.insert(*thisCut);
		}
	    }
	}

	//---------------------------------------------------------------------
	// Generate new cuts (global and/or local) and/or apply heuristics
	// NOTE: Make sure CglProbing is added FIRST
	double * newSolution = new double [numberColumns];
	double heuristicValue = getCutoff();

#if defined(ABC_DEBUG_MORE)
	    std::cout << "numberCutGenerators_ = " << numberCutGenerators_
		      << "numberHeuristics_ = " << numberHeuristics_
		      << std::endl;
#endif
	for (int i = 0; i < numberCutGenerators_ + numberHeuristics_; ++i) {
	    int numberRowCutsBefore = theseCuts.sizeRowCuts();
	    int numberColumnCutsBefore = theseCuts.sizeColCuts();
	    if (i < numberCutGenerators_) {
		if (cutDuringRampup) {
		    bool mustResolve = 
			generator_[i]->generateCuts(theseCuts, fullScan);
		    if (mustResolve) {
			feasible = resolve();
			if (!feasible)
			    break;
		    }
		}
	    } 
	    else { 
		double saveValue = heuristicValue;
		int ifSol = heuristic_[i-numberCutGenerators_]->
		    solution(heuristicValue, newSolution);
		    //    solution(heuristicValue, newSolution, theseCuts);

		if (ifSol > 0) {
		    found = i;
		} 
		else if (ifSol < 0) {
		    heuristicValue = saveValue;
		}
	    }
	    int numberRowCutsAfter = theseCuts.sizeRowCuts();
	    int numberColumnCutsAfter = theseCuts.sizeColCuts();
	    int numberBefore =
		numberRowCutsBefore + numberColumnCutsBefore + lastNumberCuts;
	    int numberAfter =
		numberRowCutsAfter + numberColumnCutsAfter + lastNumberCuts;
	    if (numberAfter > maximumWhich) {
		maximumWhich = max(maximumWhich * 2 + 100, numberAfter);
		int * temp = new int[2 * maximumWhich];
		memcpy(temp, whichGenerator, numberBefore * sizeof(int));
		delete [] whichGenerator;
		whichGenerator = temp;
	    }
	    int j;
	    if (fullScan) {
		countRowCuts[i] += numberRowCutsAfter - 
		    numberRowCutsBefore;
		countColumnCuts[i] += numberColumnCutsAfter - 
		    numberColumnCutsBefore;
	    }
	    for (j = numberRowCutsBefore; j < numberRowCutsAfter; ++j) {
		whichGenerator[numberBefore++] = i;
		const OsiRowCut * thisCut = theseCuts.rowCutPtr(j);
		if (thisCut->globallyValid()) {
		    globalCuts_.insert(*thisCut);
		}
	    }
	    for (j = numberColumnCutsBefore; j < numberColumnCutsAfter; ++j) {
		whichGenerator[numberBefore++] = i;
		const OsiColCut * thisCut = theseCuts.colCutPtr(j);
		if (thisCut->globallyValid()) {
		    globalCuts_.insert(*thisCut);
		}
	    }
	}

	//---------------------------------------------------------------------
	// If found a solution, Record it before we free the vector
	if (found >= 0) {
	    bool better = 
		setBestSolution(ABC_ROUNDING, heuristicValue, newSolution);
	    //    if (!better){
	    //	found = -1;
	    //}
	    //std::cout << "better = "  << better
	    //	      << "; found = " << found << std::endl;
	}
	if(newSolution != 0) delete [] newSolution;

	int numberColumnCuts = theseCuts.sizeColCuts();
	int numberRowCuts = theseCuts.sizeRowCuts();
	violated = numberRowCuts + numberColumnCuts;
	
	//---------------------------------------------------------------------
	// Apply column cuts 
	if (numberColumnCuts) {
	    double integerTolerance = getDblParam(AbcIntegerTolerance);
	    for (int i = 0; i < numberColumnCuts; ++i) {
		const OsiColCut * thisCut = theseCuts.colCutPtr(i);
		const CoinPackedVector & lbs = thisCut->lbs();
		const CoinPackedVector & ubs = thisCut->ubs();
		int j;
		int n;
		const int * which;
		const double * values;
		n = lbs.getNumElements();
		which = lbs.getIndices();
		values = lbs.getElements();
		for (j = 0; j < n; ++j){
		    int iColumn = which[j];
		    double value = solution[iColumn];
		    solver_->setColLower(iColumn, values[j]);
		    if (value < values[j] - integerTolerance)
			violated = -1;   // violated, TODO: when happen?
		    if (values[j] > upper[iColumn] + integerTolerance) {
			violated = -2;   // infeasible
			break;
		    }
		}
		n = ubs.getNumElements();
		which = ubs.getIndices();
		values = ubs.getElements();
		for (j = 0; j < n; ++j) {
		    int iColumn = which[j];
		    double value = solution[iColumn];
		    solver_->setColUpper(iColumn, values[j]);
		    if (value > values[j] + integerTolerance)
			violated = -1;
		    if (values[j] < lower[iColumn] - integerTolerance) {
			violated = -2;   // infeasible
			break;
		    }
		}
	    }
	}

	if (violated == -2) {
	    feasible = false ;   
	    break ;    // break the cut generation loop
	}

	//---------------------------------------------------------------------
	// Now apply the row (constraint) cuts.
	int numberRowsNow = solver_->getNumRows();
	assert(numberRowsNow == numberRowsAtStart + lastNumberCuts);
	int numberToAdd = theseCuts.sizeRowCuts();
	numberNewCuts = lastNumberCuts + numberToAdd;

	// Get a basis by asking the solver for warm start information. 
	// Resize it (retaining the basis) so it can accommodate the cuts.
	delete basis_;
	basis_ = dynamic_cast<CoinWarmStartBasis*>(solver_->getWarmStart());
	assert(basis_ != NULL); // make sure not volume
	basis_->resize(numberRowsAtStart + numberNewCuts, numberColumns);

	//  Now actually add the row cuts and reoptimise.
	if (numberRowCuts > 0 || numberColumnCuts > 0) { 
	    if (numberToAdd > 0) { 
		int i;
		OsiRowCut * addCuts = new OsiRowCut [numberToAdd];
		for (i = 0; i < numberToAdd; ++i) { 
		    addCuts[i] = theseCuts.rowCut(i); 
		}
		solver_->applyRowCuts(numberToAdd, addCuts);
		// AJK this caused a memory fault on Win32
		delete [] addCuts;
		for (i = 0; i < numberToAdd; ++i) { 
		    cuts.insert(theseCuts.rowCut(i)); 
		}
		for (i = 0; i < numberToAdd; ++i) { 
		    basis_->setArtifStatus(numberRowsNow + i,
					   CoinWarmStartBasis::basic); 
		}
		if (solver_->setWarmStart(basis_) == false) {
		    throw CoinError("Fail setWarmStart() after cut install.",
				    "solveWithCuts", "SbbModel"); 
		} 
	    }
	    feasible = resolve() ;
	}
	else { 
	    numberTries = 0; 
	}

	//---------------------------------------------------------------------
	if (feasible) { 
	    int cutIterations = solver_->getIterationCount();
	    //takeOffCuts(cuts, whichGenerator, 
	    // numberOldActiveCuts, numberNewCuts, true);
	    if (solver_->isDualObjectiveLimitReached()) { 
		feasible = false;
#ifdef ABC_DEBUG
		double z = solver_->getObjValue();
		double cut = getCutoff();
		//	printf("Lost feasibility by %g in takeOffCuts; z = %g, cutoff = %g\n",
		//   z - cut, z, cut);
#endif
	    }
	    if (feasible) { 
		numberRowsAtStart = numberOldActiveCuts + 
		    numberRowsAtContinuous_;
		lastNumberCuts = numberNewCuts;
		if ((direction * solver_->getObjValue() < 
		    lastObjective + minimumDrop) &&  (numberPasses >= 3)) { 
		    numberTries = 0; 
		}
		if (numberRowCuts+numberColumnCuts == 0 || cutIterations == 0)
		{ break; }
		if (numberTries > 0) { 
		    reducedCostFix();
		    lastObjective = direction * solver_->getObjValue();
		    lower = solver_->getColLower();
		    upper = solver_->getColUpper();
		    solution = solver_->getColSolution(); 
		} 
	    } 
	}

	// We've lost feasibility 
	if (!feasible) { 
	    numberTries = 0;
	}
    } while (numberTries);
    // END OF GENERATING CUTS

    //------------------------------------------------------------------------
    // Adjust the frequency of use for any of the cut generator 
    double thisObjective = solver_->getObjValue() * direction;
    if (feasible && fullScan && numberCutGenerators_) {
	double totalCuts = 0.0;
	int i;
	for (int i = 0; i < numberCutGenerators_; ++i) 
	totalCuts += countRowCuts[i] + 5.0 * countColumnCuts[i];
	// Root node or every so often - see what to turn off
	if (!numberNodes_)
	    handler_->message(ABC_ROOT, messages_)
		<< numberNewCuts
		<< startObjective << thisObjective
		<< numberPasses
		<< CoinMessageEol;
	int * count = new int[numberCutGenerators_];
	memset(count, 0, numberCutGenerators_ * sizeof(int));
	for (i = 0; i < numberNewCuts; ++i) 
	    count[whichGenerator[i]]++;
	double small = (0.5 * totalCuts) / ((double) numberCutGenerators_);
	for (i = 0; i < numberCutGenerators_; ++i) {
	    int howOften = generator_[i]->howOften();
	    if (howOften < -99)
		continue;
	    if (howOften < 0 || howOften >= 1000000) {
		// If small number switch mostly off
		double thisCuts = countRowCuts[i] + 5.0 * countColumnCuts[i];
		if (!thisCuts || howOften == -99) {
		    if (howOften == -99)
			howOften = -100;
		    else
			howOften = 1000000 + SCANCUTS; // wait until next time
		} 
		else if (thisCuts < small) {
		    int k = (int) sqrt(small / thisCuts);
		    howOften = k + 1000000;
		} 
		else {
		    howOften = 1 + 1000000;
		}
	    }
	    generator_[i]->setHowOften(howOften);
	    int newFrequency = generator_[i]->howOften() % 1000000;
	    // if (handler_->logLevel() > 1 || !numberNodes_)
	    if (!numberNodes_)
		handler_->message(ABC_GENERATOR, messages_)
		    << i
		    << generator_[i]->cutGeneratorName()
		    << countRowCuts[i]
		    << countRowCuts[i] //<<count[i]
		    << countColumnCuts[i]
		    << newFrequency
		    << CoinMessageEol;
	}
	delete [] count;
    }

    delete [] countRowCuts;
    delete [] countColumnCuts;

#ifdef CHECK_CUT_COUNTS
    if (feasible) {
	delete basis_;
	basis_ = dynamic_cast<CoinWarmStartBasis*>(solver_->getWarmStart());
	printf("solveWithCuts: Number of rows at end (only active cuts) %d\n",
	       numberRowsAtContinuous_+numberNewCuts+numberOldActiveCuts);
	basis_->print(); 
    }
    if (numberNodes_ % 1000 == 0) {
	messageHandler()->message(ABC_CUTS, messages_)
	    << numberNodes_
	    << numberNewCuts
	    << startObjective 
	    << thisObjective
	    << numberPasses
	    << CoinMessageEol;
    }
#endif
    

    //takeOffCuts(cuts, whichGenerator, numberOldActiveCuts, 
    //		numberNewCuts, true);
    incrementNodeCount();

    return feasible;
}

//#############################################################################

bool
AbcModel::resolve()
{
    int iRow;
    int numberRows = solver_->getNumRows();
    const double * rowLower = solver_->getRowLower();
    const double * rowUpper = solver_->getRowUpper();
    bool feasible = true;
    for (iRow = numberRowsAtContinuous_; iRow < numberRows; ++iRow) {
	if (rowLower[iRow] > rowUpper[iRow] + 1.0e-8)
	    feasible = false;
    }

    // Reoptimize. Consider the possibility that we should fathom on bounds. 
    // But be careful --- where the objective takes on integral values, we may
    // want to keep a solution where the objective is right on the cutoff.
    if (feasible) { 
	solver_->resolve();
	numberIterations_ += getIterationCount();
	feasible = (solver_->isProvenOptimal() &&
		    !solver_->isDualObjectiveLimitReached()); 
    }

    return feasible;
}

//#############################################################################

double 
AbcModel::checkSolution (double cutoff, 
			 const double *solution,
			 bool fixVariables)
{
    return 0.0;
}

//#############################################################################

bool
AbcModel::setBestSolution(ABC_Message how,
			  double & objectiveValue, 
			  const double * solution, 
			  bool fixVariables)
{
    double cutoff = getCutoff();
    // Double check the solution to catch pretenders.
    if (objectiveValue >= cutoff) {  // Bad news
	if (objectiveValue > 1.0e30)
	    handler_->message(ABC_NOTFEAS1, messages_) << CoinMessageEol;
	else
	    handler_->message(ABC_NOTFEAS2, messages_)
		<< objectiveValue << cutoff << CoinMessageEol;
	return false;
    }
    else {  // Better solution
	bestObjective_ = objectiveValue;
	int numberColumns = solver_->getNumCols();
	if (bestSolution_ == 0) {
	    bestSolution_ = new double[numberColumns];
	}
	
	memcpy(bestSolution_, solution, numberColumns*sizeof(double));
	cutoff = bestObjective_ - dblParam_[AbcCutoffIncrement];
	setCutoff(cutoff);

	if (how == ABC_ROUNDING)
	    numberHeuristicSolutions_++;
	numberSolutions_++;
//	std::cout << "cutoff = " << getCutoff() 
//		  << "; objVal = " << bestObjective_ 
//		  << "; cutoffInc = " << dblParam_[AbcCutoffIncrement] 
//		  << std::endl;
	
	handler_->message(how, messages_)
	    << bestObjective_ << numberIterations_
	    << numberNodes_
	    << CoinMessageEol;

	return true;
    }
}

//#############################################################################

bool 
AbcModel::feasibleSolution(int & numberIntegerInfeasibilities)
{
    bool feasible = true;
    numberIntegerInfeasibilities = 0;
    int i = -1;
    const int numCols = getNumCols();

    if (currentSolution_ != 0) {
	delete [] currentSolution_;
	currentSolution_ = 0;
    }

    currentSolution_ = new double [numCols];
    memcpy(currentSolution_, solver_->getColSolution(),sizeof(double)*numCols);

    for (i = 0; i < numberIntegers_; ++i) {
	if ( ! checkInteger(currentSolution_[integerVariable_[i]]) ) {
	    ++numberIntegerInfeasibilities;
	    feasible = false;
	}
    }

    return feasible;
}

//#############################################################################

void 
AbcModel::findIntegers(bool startAgain)
{
    assert(solver_);

    int iColumn;    
    int numberColumns = getNumCols();
    const double *objCoeffs = getObjCoefficients();
    
    if (numberStrong_ == 0) {  // set up pseudocost list
	pseudoList_ = new AbcPseudocost* [getNumCols()];
	pseudoIndices_ = new int [getNumCols()];
        for (iColumn = 0; iColumn < numberColumns; ++iColumn) {
            pseudoList_[iColumn] = NULL;
            pseudoIndices_[iColumn] = -1;
        }
    }

    if (numberIntegers_ && !startAgain) return;
    delete [] integerVariable_;
    numberIntegers_ = 0;

    for (iColumn = 0; iColumn < numberColumns; iColumn++) {
	if (isInteger(iColumn)) numberIntegers_++;
    }

    if (numberIntegers_) {
	integerVariable_ = new int [numberIntegers_];
	numberIntegers_=0;
	for (iColumn = 0; iColumn < numberColumns; ++iColumn) {
	    if(isInteger(iColumn)) {
		integerVariable_[numberIntegers_++] = iColumn;
		if (numberStrong_ == 0) {
		    double obj = fabs(objCoeffs[iColumn]);
		    AbcPseudocost *pcost = new AbcPseudocost(iColumn,
							     obj,
							     0,
							     obj,
							     0);
		    pseudoList_[iColumn] = pcost;
		    //printf("numberIntegers_ = %d\n", numberIntegers_);
		}
		//printf("out\n");
	    }
	}
    } 
    else {
	handler_->message(ABC_NOINT, messages_) << CoinMessageEol ;
    }

    
	
}

//#############################################################################
// Add one generator
void 
AbcModel::addCutGenerator(CglCutGenerator * generator,
			  int howOften, const char * name,
			  bool normal, bool atSolution,
			  bool whenInfeasible)
{
    AbcCutGenerator ** temp = generator_;
    generator_ = new AbcCutGenerator * [numberCutGenerators_ + 1];
    memcpy(generator_, temp, numberCutGenerators_*sizeof(AbcCutGenerator *));
    delete[] temp;
    generator_[numberCutGenerators_++]= 
	new AbcCutGenerator(this, generator, howOften, name,
			    normal, atSolution, whenInfeasible);
							  
}

//#############################################################################
// Add one heuristic
void 
AbcModel::addHeuristic(AbcHeuristic* generator)
{
  AbcHeuristic ** temp = heuristic_;
  heuristic_ = new AbcHeuristic* [numberHeuristics_ + 1];
  memcpy(heuristic_, temp, numberHeuristics_ * sizeof(AbcHeuristic *));
  delete [] temp;
  heuristic_[numberHeuristics_++] = generator;
}

//#############################################################################
// Perform reduced cost fixing on integer variables. The variables in 
// question are already nonbasic at bound. We're just nailing down the 
// current situation.
void AbcModel::reducedCostFix ()
{ 
    double cutoff = getCutoff();
    double direction = solver_->getObjSense();
    double gap = cutoff - solver_->getObjValue()*direction;
    double integerTolerance = getDblParam(AbcIntegerTolerance);

    const double* lower = solver_->getColLower();
    const double* upper = solver_->getColUpper();
    const double* solution = solver_->getColSolution();
    const double* reducedCost = solver_->getReducedCost();

    int numberFixed = 0 ;
    for (int i = 0; i < numberIntegers_; i++) { 
	int iColumn = integerVariable_[i];
	double djValue = direction * reducedCost[iColumn];
	if (upper[iColumn] - lower[iColumn] > integerTolerance) { 
	    if (solution[iColumn] < lower[iColumn] + integerTolerance && 
		djValue > gap) { 
		solver_->setColUpper(iColumn, lower[iColumn]);
		numberFixed++; 
	    }
	    else if (solution[iColumn] > upper[iColumn] - integerTolerance && 
		     -djValue > gap) { 
		solver_->setColLower(iColumn, upper[iColumn]);
		numberFixed++; 
	    } 
	} 
    }  
}

//#############################################################################
#if 0
void
AbcModel::takeOffCuts(OsiCuts &newCuts, int *whichGenerator,
		      int &numberOldActiveCuts, int &numberNewCuts,
		      bool allowResolve) 
{
    int firstOldCut = numberRowsAtContinuous_;
    int totalNumberCuts = numberNewCuts + numberOldActiveCuts;
    int *solverCutIndices = new int[totalNumberCuts];
    int *newCutIndices = new int[numberNewCuts];
    const CoinWarmStartBasis* ws;
    CoinWarmStartBasis::Status status;
    bool needPurge = true;
    
    // The outer loop allows repetition of purge in the event that 
    // reoptimisation changes the basis. To start an iteration, clear the 
    // deletion counts and grab the current basis.
    
    while (needPurge) { 
	int numberNewToDelete = 0;
	int numberOldToDelete = 0;
	int i;
	ws = dynamic_cast<const CoinWarmStartBasis*>(solver_->getWarmStart());

	// Scan the basis entries of the old cuts generated prior to this 
	// round of cut generation. Loose cuts are `removed'.
	for (i = 0; i < numberOldActiveCuts; ++i) { 
	    status = ws->getArtifStatus(i + firstOldCut);
	    if (status == CoinWarmStartBasis::basic) { 
		solverCutIndices[numberOldToDelete++] = i + firstOldCut;
	    }
	}

	// Scan the basis entries of the new cuts generated with this round 
	// of cut generation.  At this point, newCuts is the only record of 
	// the new cuts, so when we delete loose cuts from newCuts, they're 
	// really gone. newCuts is a vector, so it's most efficient to 
	// compress it (eraseRowCut) from back to front.
	int firstNewCut = firstOldCut + numberOldActiveCuts;
	int k = 0;
	for (i = 0; i < numberNewCuts; ++i) { 
	    status = ws->getArtifStatus(i + firstNewCut);
	    if (status == CoinWarmStartBasis::basic) { 
		solverCutIndices[numberNewToDelete + numberOldToDelete] = 
		    i + firstNewCut ;
		newCutIndices[numberNewToDelete++] = i;
	    }
	    else { // save which generator did it
		whichGenerator[k++] = whichGenerator[i]; 
	    } 
	}
	for (i = numberNewToDelete - 1 ; i >= 0 ; i--) { 
	    int iCut = newCutIndices[i];
	    newCuts.eraseRowCut(iCut); 
	}

	// Did we delete anything? If so, delete the cuts from the constraint
	// system held in the solver and reoptimise unless we're forbidden 
	// to do so. If the call to resolve() results in pivots, there's the 
	// possibility we again have basic slacks. Repeat the purging loop.

	if (numberNewToDelete  + numberOldToDelete > 0) { 
	    solver_->deleteRows(numberNewToDelete + numberOldToDelete,
				solverCutIndices);
	    numberNewCuts -= numberNewToDelete;
	    numberOldActiveCuts -= numberOldToDelete;
#           ifdef ABC_DEBUG
	    std::cout << "takeOffCuts: purged " << numberOldToDelete << "+"
		      << numberNewToDelete << " cuts." << std::endl;
#           endif
	    if (allowResolve) { 
		solver_->resolve();
		if (solver_->getIterationCount() == 0) { 
		    needPurge = false; 
		}
#	    ifdef ABC_DEBUG
		else { 
		    std::cout << "Repeating purging loop. "
			      << solver_->getIterationCount() << " iters."
			      << std::endl; 
		}
#	    endif
	    }
	    else { 
		needPurge = false; 
	    } 
	} 
	else { 
	    needPurge = false; 
	} 
    }

    delete ws;
    delete [] solverCutIndices;
    delete [] newCutIndices;
}
#endif

//#############################################################################
#if 1
//void
//AbcModel::takeOffCuts(OsiCuts &newCuts, int *whichGenerator,
//		      int &numberOldActiveCuts, int &numberNewCuts,
//		      bool allowResolve) 
void
AbcModel::takeOffCuts()
{
//    assert(!numberOldActiveCuts);
    int totalNumberCuts = solver()->getNumRows() - numberRowsAtContinuous_;
    int *solverCutIndices = new int[totalNumberCuts];
    //  const CoinWarmStartBasis* ws;
    
    for (int i = 0; i < totalNumberCuts; ++i) {
	solverCutIndices[i] = i + numberRowsAtContinuous_;
    }
    
    // Delete all new cuts
    solver_->deleteRows(totalNumberCuts, solverCutIndices);
    solver_->setWarmStart(sharedBasis_);
    //numberOldActiveCuts = numberNewCuts = 0;
    delete []  solverCutIndices;
}
#endif

//#############################################################################
/**
  This routine sets the objective cutoff value used for fathoming and
  determining monotonic variables.

  If the fathoming discipline is strict, a small tolerance is added to the
  new cutoff. This avoids problems due to roundoff when the target value
  is exact. The common example would be an IP with only integer variables in
  the objective. If the target is set to the exact value z of the optimum,
  it's possible to end up fathoming an ancestor of the solution because the
  solver returns z+epsilon.

  Determining if strict fathoming is needed is best done by analysis.
  In sbb, that's analyseObjective. The default is false.

  In sbb we always minimize so add epsilon
*/
void AbcModel::setCutoff (double value)
{ 
    double tol = 0;
    int fathomStrict = getIntParam(AbcFathomDiscipline);
    double direction = solver_->getObjSense();
    if (fathomStrict == 1) { 
	solver_->getDblParam(OsiDualTolerance, tol);
	tol = tol * (1 + fabs(value));
	value += tol; 
    }
    
    // Solvers know about direction
    solver_->setDblParam(OsiDualObjectiveLimit, value * direction); 
}

//#############################################################################
// Initial solve and find integers
bool
AbcModel::setupSelf()
{   
    bool feasible = true;
    solver_->messageHandler()->setLogLevel(0);
    initialSolve();
    sharedBasis_ = dynamic_cast<CoinWarmStartBasis*>
	(solver_->getWarmStart());
    
# ifdef ABC_DEBUG_MORE
    std::string problemName;
    solver_->getStrParam(OsiProbName, problemName);
    printf("Problem name - %s\n", problemName.c_str());
    solver_->setHintParam(OsiDoReducePrint, false, OsiHintDo, 0);
# endif

    status_ = 0;

    findIntegers(true);

    bestObjective_ = 1.0e50;
    double direction = solver_->getObjSense();

    int numberColumns = getNumCols();
    if (!currentSolution_)
	currentSolution_ = new double[numberColumns];

    //continuousSolver_ = solver_->clone();
    numberRowsAtContinuous_ = getNumRows();

    maximumNumberCuts_ = 0;
    currentNumberCuts_ = 0;

    // FIXME:
    
    return feasible;
}

//#############################################################################
// Send model and root so that initial solve
AlpsEncoded* 
AbcModel::encode() const 
{ 
    AlpsReturnCode status = ALPS_OK;

    AlpsEncoded* encoded = new AlpsEncoded(ALPS_MODEL);

    //------------------------------------------------------
    // Encode Alps part. 
    //------------------------------------------------------

    status = encodeAlps(encoded);
    
    //------------------------------------------------------
    // Encode Abc part. 
    //------------------------------------------------------

    // Write the model data into representation_
    const CoinPackedMatrix* matrixByCol = solver_->getMatrixByCol();
    int numRows = getNumRows();
    encoded->writeRep(numRows);
    int numCols = getNumCols();
    encoded->writeRep(numCols);
#if defined(ABC_DEBUG_MORE)
    std::cout << "AbcModel::encode()-- numRows="<< numRows << "; numCols=" 
	      << numCols << std::endl;
#endif

    const double* collb = solver_->getColLower();
    encoded->writeRep(collb, numCols);
    const double* colub = solver_->getColUpper();
    encoded->writeRep(colub, numCols);
    const double* obj = solver_->getObjCoefficients();
    encoded->writeRep(obj, numCols);
    const double objSense = solver_->getObjSense();
    encoded->writeRep(objSense);
    const double* rowlb = solver_->getRowLower();
    encoded->writeRep(rowlb, numRows);
    const double* rowub = solver_->getRowUpper();
    encoded->writeRep(rowub, numRows);
    int numElements = solver_->getNumElements();
    encoded->writeRep(numElements);
    const double* elementValue = matrixByCol->getElements();
    encoded->writeRep(elementValue, numElements);
    const CoinBigIndex* colStart = matrixByCol->getVectorStarts();
    int numStart = numCols + 1;
    encoded->writeRep(colStart, numStart);
    const int* index = matrixByCol->getIndices();
    encoded->writeRep(index, numElements);
    encoded->writeRep(numberIntegers_);
    encoded->writeRep(integerVariable_, numberIntegers_);
#if defined(ABC_DEBUG_MORE)
    std::cout << "AbcModel::encode()-- objSense="<< objSense
	      << "; numElements="<< numElements 
	      << "; numberIntegers_=" << numberIntegers_ 
	      << "; numStart = " << numStart <<std::endl;
#endif
#if defined(ABC_DEBUG_MORE)
    std::cout << "rowub=";
    for (int i = 0; i < numRows; ++i){
	std::cout <<rowub[i]<<" ";
    }
    std::cout << std::endl;
    std::cout << "elementValue=";
    for (int j = 0; j < numElements; ++j) {
	std::cout << elementValue[j] << " ";
    }
    std::cout << std::endl;    
#endif

    return encoded;
}

//#############################################################################
// Decode and load model data to LP solver. 
void
AbcModel::decodeToSelf(AlpsEncoded& encoded) 
{
    AlpsReturnCode status = ALPS_OK;

    //------------------------------------------------------
    // Decode Alps part. 
    //------------------------------------------------------

    status = decodeAlps(encoded);

    //------------------------------------------------------
    // Decode Abc part. 
    //------------------------------------------------------

    int numRows;
    encoded.readRep(numRows);
    int numCols;
    encoded.readRep(numCols);    
#if defined(ABC_DEBUG_MORE)
    std::cout << "AbcModel::decode()-- numRows="<< numRows << "; numCols=" 
	      << numCols << std::endl;
#endif
    double* collb;
    encoded.readRep(collb, numCols);
    double* colub;
    encoded.readRep(colub, numCols);
    double* obj;
    encoded.readRep(obj, numCols);
    double objSense;
    encoded.readRep(objSense);
    double* rowlb;
    encoded.readRep(rowlb, numRows);
    double* rowub;
    encoded.readRep(rowub, numRows);
    int numElements;
    encoded.readRep(numElements);
    double* elementValue;
    encoded.readRep(elementValue, numElements);
    CoinBigIndex* colStart;
    int numStart = numCols + 1;
    encoded.readRep(colStart, numStart);
    int* index;
    encoded.readRep(index, numElements);
    encoded.readRep(numberIntegers_);
    encoded.readRep(integerVariable_, numberIntegers_);
#if defined(ABC_DEBUG_MORE)
    std::cout << "AbcModel::decode()-- objSense="<< objSense
	      <<  "; numElements="<< numElements 
	      << "; numberIntegers_=" << numberIntegers_ 
	      << "; numStart = " << numStart <<std::endl;
#endif
#if defined(ABC_DEBUG_MORE)
    std::cout << "rowub=";
    for (int i = 0; i < numRows; ++i){
	std::cout <<rowub[i]<<" ";
    }
    std::cout << std::endl;
    std::cout << "elementValue=";
    for (int j = 0; j < numElements; ++j) {
	std::cout << elementValue[j] << " ";
    }
    std::cout << std::endl;  
    std::cout << "index=";
    for (int j = 0; j < numElements; ++j) {
	std::cout << index[j] << " ";
    }
    std::cout << std::endl;  
    std::cout << "colStart=";
    for (int j = 0; j < numElements+1; ++j) {
	std::cout << colStart[j] << " ";
    }
    std::cout << std::endl;   
#endif

    // Check if solver_ is declared in main
    assert(solver_);

    //-------------------------------------------------------------------------
    // load the standardized problem into stdSi    
#if 0  // load matrix doesn't work. Don't know why.
    CoinPackedMatrix * matrixByCol = 
	new CoinPackedMatrix(true, numCols, numRows, numElements,
			     elementValue, index, colStart, 0);
    solver_->loadProblem(*matrixByCol,
			 collb, colub,   
			 obj,
			 rowlb, rowub);
#endif
    //-------------------------------------------------------------------------
    solver_->loadProblem(numCols, numRows,
			 colStart, index, elementValue,
			 collb, colub, 
			 obj,
			 rowlb, rowub);

    solver_->setObjSense(objSense);
    solver_->setInteger(integerVariable_, numberIntegers_);

    delete [] collb;
    collb = NULL;
    delete [] colub;
    colub = NULL;
    delete [] obj;
    obj = NULL;
    delete [] rowlb;
    rowlb = NULL;
    delete [] rowub;
    rowub = NULL;
    delete [] elementValue;
    elementValue = NULL;
    delete [] colStart;
    colStart = NULL;
    delete [] index;
    index = NULL;
}
