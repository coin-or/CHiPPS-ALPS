/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Eclipse Public License as part of the       *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors:                                                                  *
 *                                                                           *
 *          Yan Xu, Lehigh University                                        *
 *          Aykut Bulut, Lehigh University                                   *
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
 * Copyright (C) 2001-2019, Lehigh University, Yan Xu, Aykut Bulut, and      *
 *                          Ted Ralphs.                                      *
 * All Rights Reserved.                                                      *
 *===========================================================================*/


//#############################################################################
// This file is modified from SbbHeuristic.cpp
//#############################################################################

#include <cassert>
#include <cmath>
#include <cfloat>

#include "OsiSolverInterface.hpp"
#include "AbcModel.h"
//#include "AbcMessage.hpp"
#include "AbcHeuristic.h"

// Default Constructor
AbcHeuristic::AbcHeuristic()
    :model_(NULL)
{
}

// Constructor from model
AbcHeuristic::AbcHeuristic(AbcModel & model)
{
    model_ = &model;
}

// Destructor
AbcHeuristic::~AbcHeuristic ()
{
}

// update model
void AbcHeuristic::setModel(AbcModel * model)
{
    model_ = model;
}

// Default Constructor
AbcRounding::AbcRounding()
    :AbcHeuristic()
{
    // matrix and row copy will automatically be empty
}

// Constructor from model
AbcRounding::AbcRounding(AbcModel & model)
    :AbcHeuristic(model)
{
    assert(model.solver());
}

// Destructor
AbcRounding::~AbcRounding ()
{
}

// Clone
AbcHeuristic *
AbcRounding::clone() const
{
    return new AbcRounding(*this);
}

// Copy constructor
AbcRounding::AbcRounding(const AbcRounding & rhs)
    :
    AbcHeuristic(rhs),
    matrix_(rhs.matrix_),
    matrixByRow_(rhs.matrixByRow_),
    seed_(rhs.seed_)
{
}
// See if rounding will give solution
// Sets value of solution
// Assumes rhs for original matrix still okay
// At present only works with integers
// Fix values if asked for
// Returns 1 if solution, 0 if not
int
AbcRounding::solution(double & solutionValue,
		      double * betterSolution)
{
    // Get a copy of original matrix (and by row for rounding);
    matrix_ = *(model_->solver()->getMatrixByCol());
    matrixByRow_ = *(model_->solver()->getMatrixByRow());
    seed_=1;

    OsiSolverInterface * solver = model_->solver();
    const double * lower = solver->getColLower();
    const double * upper = solver->getColUpper();
    const double * rowLower = solver->getRowLower();
    const double * rowUpper = solver->getRowUpper();
    const double * solution = solver->getColSolution();
    const double * objective = solver->getObjCoefficients();
    double integerTolerance = 1.0e-5;
    //model_->getDblParam(AbcModel::AbcIntegerTolerance);
    double primalTolerance;
    solver->getDblParam(OsiPrimalTolerance, primalTolerance);

    int numberRows = matrix_.getNumRows();

    int numberIntegers = model_->numberIntegers();
    const int * integerVariable = model_->integerVariable();
    int i;
    double direction = solver->getObjSense();
    double newSolutionValue = direction * solver->getObjValue();
    int returnCode = 0;

    // Column copy
    const double * element = matrix_.getElements();
    const int * row = matrix_.getIndices();
    const int * columnStart = matrix_.getVectorStarts();
    const int * columnLength = matrix_.getVectorLengths();
    // Row copy
    const double * elementByRow = matrixByRow_.getElements();
    const int * column = matrixByRow_.getIndices();
    const int * rowStart = matrixByRow_.getVectorStarts();
    const int * rowLength = matrixByRow_.getVectorLengths();

    // Get solution array for heuristic solution
    int numberColumns = solver->getNumCols();
    double * newSolution = new double [numberColumns];
    memcpy(newSolution, solution, numberColumns * sizeof(double));

    double * rowActivity = new double[numberRows];
    memset(rowActivity, 0, numberRows*sizeof(double));
    for (i = 0; i < numberColumns; i++) {
	int j;
	double value = newSolution[i];
	if (value) {
	    for (j = columnStart[i];
		 j < columnStart[i] + columnLength[i]; j++) {
		int iRow = row[j];
		rowActivity[iRow] += value*element[j];
	    }
	}
    }
    // check was feasible - if not adjust (cleaning may move)
    for (i = 0; i < numberRows; i++) {
	if(rowActivity[i] < rowLower[i]) {
	    //assert (rowActivity[i]>rowLower[i]-1000.0*primalTolerance);
	    rowActivity[i] = rowLower[i];
	} else if(rowActivity[i] > rowUpper[i]) {
	    //assert (rowActivity[i]<rowUpper[i]+1000.0*primalTolerance);
	    rowActivity[i] = rowUpper[i];
	}
    }
    for (i = 0; i < numberIntegers; i++) {
	int iColumn = integerVariable[i];
	double value = newSolution[iColumn];
	if (fabs(floor(value + 0.5) - value) > integerTolerance) {
	    double below = floor(value);
	    double newValue = newSolution[iColumn];
	    double cost = direction * objective[iColumn];
	    double move;
	    if (cost > 0.0) {
		// try up
		move = 1.0 - (value - below);
	    } else if (cost < 0.0) {
		// try down
		move = below - value;
	    } else {
		// won't be able to move unless we can grab another variable
		// just for now go down
		move = below-value;
	    }
	    newValue += move;
	    newSolution[iColumn] = newValue;
	    newSolutionValue += move * cost;
	    int j;
	    for (j = columnStart[iColumn];
		 j < columnStart[iColumn] + columnLength[iColumn]; j++) {
		int iRow = row[j];
		rowActivity[iRow] += move * element[j];
	    }
	}
    }

    double penalty = 0.0;

    // see if feasible
    for (i = 0; i < numberRows; i++) {
	double value = rowActivity[i];
	double thisInfeasibility = 0.0;
	if (value < rowLower[i] - primalTolerance)
	    thisInfeasibility = value - rowLower[i];
	else if (value > rowUpper[i] + primalTolerance)
	    thisInfeasibility = value - rowUpper[i];
	if (thisInfeasibility) {
	    // See if there are any slacks I can use to fix up
	    // maybe put in coding for multiple slacks?
	    double bestCost = 1.0e50;
	    int k;
	    int iBest = -1;
	    double addCost = 0.0;
	    double newValue = 0.0;
	    double changeRowActivity = 0.0;
	    double absInfeasibility = fabs(thisInfeasibility);
	    for (k = rowStart[i]; k < rowStart[i] + rowLength[i]; k++) {
		int iColumn = column[k];
		if (columnLength[iColumn] == 1) {
		    double currentValue = newSolution[iColumn];
		    double elementValue = elementByRow[k];
		    double lowerValue = lower[iColumn];
		    double upperValue = upper[iColumn];
		    double gap = rowUpper[i] - rowLower[i];
		    double absElement = fabs(elementValue);
		    if (thisInfeasibility * elementValue > 0.0) {
			// we want to reduce
			if ((currentValue - lowerValue) * absElement >=
			    absInfeasibility) {

			    // possible - check if integer
			    double distance = absInfeasibility / absElement;
			    double thisCost =
				-direction * objective[iColumn] * distance;
			    if (solver->isInteger(iColumn)) {
				distance = ceil(distance - primalTolerance);
				assert (currentValue - distance >=
					lowerValue - primalTolerance);
				if (absInfeasibility - distance * absElement
				    < -gap - primalTolerance)
				    thisCost = 1.0e100; // no good
				else
				    thisCost =
					-direction*objective[iColumn]*distance;
			    }
			    if (thisCost < bestCost) {
				bestCost = thisCost;
				iBest = iColumn;
				addCost = thisCost;
				newValue = currentValue - distance;
				changeRowActivity = -distance * elementValue;
			    }
			}
		    } else {
			// we want to increase
			if ((upperValue - currentValue) * absElement >=
			    absInfeasibility) {
			    // possible - check if integer
			    double distance = absInfeasibility / absElement;
			    double thisCost =
				direction * objective[iColumn] * distance;
			    if (solver->isInteger(iColumn)) {
				distance = ceil(distance - 1.0e-7);
				assert (currentValue - distance <=
					upperValue + primalTolerance);
				if (absInfeasibility - distance * absElement
				    < -gap - primalTolerance)
				    thisCost = 1.0e100; // no good
				else
				    thisCost =
					direction*objective[iColumn]*distance;
			    }
			    if (thisCost < bestCost) {
				bestCost = thisCost;
				iBest = iColumn;
				addCost = thisCost;
				newValue = currentValue + distance;
				changeRowActivity = distance * elementValue;
			    }
			}
		    }
		}
	    }
	    if (iBest >= 0) {
		/*printf("Infeasibility of %g on row %d cost %g\n",
		  thisInfeasibility,i,addCost);*/
		newSolution[iBest] = newValue;
		thisInfeasibility = 0.0;
		newSolutionValue += addCost;
		rowActivity[i] += changeRowActivity;
	    }
	    penalty += fabs(thisInfeasibility);
	}
    }

    // Could also set SOS (using random) and repeat
    if (!penalty) {
	// See if we can do better
	//seed_++;
	//CoinSeedRandom(seed_);
	// Random number between 0 and 1.
	double randomNumber = CoinDrand48();
	int iPass;
	int start[2];
	int end[2];
	int iRandom = (int) (randomNumber * ((double) numberIntegers));
	start[0] = iRandom;
	end[0] = numberIntegers;
	start[1] = 0;
	end[1] = iRandom;
	for (iPass = 0; iPass < 2; iPass++) {
	    int i;
	    for (i = start[iPass]; i < end[iPass]; i++) {
		int iColumn = integerVariable[i];
		double value = newSolution[iColumn];
		assert(fabs(floor(value + 0.5) - value) < integerTolerance);
		double cost = direction * objective[iColumn];
		double move = 0.0;
		if (cost > 0.0)
		    move = -1.0;
		else if (cost < 0.0)
		    move = 1.0;
		while (move) {
		    bool good = true;
		    double newValue = newSolution[iColumn] + move;
		    if (newValue < lower[iColumn] - primalTolerance||
			newValue > upper[iColumn] + primalTolerance) {
			move = 0.0;
		    } else {
			// see if we can move
			int j;
			for (j = columnStart[iColumn];
			     j < columnStart[iColumn] + columnLength[iColumn];
			     j++) {
			    int iRow = row[j];
			    double newActivity =
				rowActivity[iRow] + move*element[j];
			    if (newActivity < rowLower[iRow] - primalTolerance
				||
				newActivity > rowUpper[iRow]+primalTolerance) {
				good = false;
				break;
			    }
			}
			if (good) {
			    newSolution[iColumn] = newValue;
			    newSolutionValue += move * cost;
			    int j;
			    for (j = columnStart[iColumn];
				 j < columnStart[iColumn] +
				     columnLength[iColumn]; j++) {
				int iRow = row[j];
				rowActivity[iRow] += move*element[j];
			    }
			} else {
			    move=0.0;
			}
		    }
		}
	    }
	}
	if (newSolutionValue < solutionValue) {
	    // paranoid check
	    memset(rowActivity, 0, numberRows * sizeof(double));
	    for (i = 0; i < numberColumns; i++) {
		int j;
		double value = newSolution[i];
		if (value) {
		    for (j = columnStart[i];
			 j < columnStart[i] + columnLength[i]; j++) {
			int iRow = row[j];
			rowActivity[iRow] += value * element[j];
		    }
		}
	    }
	    // check was approximately feasible
	    bool feasible = true;
	    for (i = 0; i < numberRows; i++) {
		if(rowActivity[i] < rowLower[i]) {
		    if (rowActivity[i] < rowLower[i] - 1000.0*primalTolerance)
			feasible = false;
		} else if(rowActivity[i] > rowUpper[i]) {
		    if (rowActivity[i] > rowUpper[i] + 1000.0*primalTolerance)
			feasible = false;
		}
	    }
	    if (feasible) {
		// new solution
		memcpy(betterSolution, newSolution,
		       numberColumns * sizeof(double));
		solutionValue = newSolutionValue;
		//printf("** Solution of %g found by rounding\n",newSolutionValue);
		returnCode=1;
	    } else {
		// Can easily happen
		//printf("Debug AbcRounding giving bad solution\n");
	    }
	}
    }
    delete [] newSolution;
    delete [] rowActivity;
    return returnCode;
}
// update model
void AbcRounding::setModel(AbcModel * model)
{
    model_ = model;
    // Get a copy of original matrix (and by row for rounding);
    assert(model_->solver());
    matrix_ = *model_->solver()->getMatrixByCol();
    matrixByRow_ = *model_->solver()->getMatrixByRow();
}
