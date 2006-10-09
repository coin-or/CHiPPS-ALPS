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

//#############################################################################
// This file is modified from SbbCutGenerator.cpp
//#############################################################################

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif
#include <cassert>
#include <cmath>
#include <cfloat>

#include "OsiSolverInterface.hpp"
#include "CglProbing.hpp"

#include "AbcModel.h"
#include "AbcMessage.h"
#include "AbcCutGenerator.h"


// Default Constructor 
AbcCutGenerator::AbcCutGenerator ()
    : model_(NULL),
      generator_(NULL),
      whenCutGenerator_(-1),
      generatorName_(NULL),
      normal_(true),
      atSolution_(false),
      whenInfeasible_(false)
{
}
// Normal constructor
AbcCutGenerator::AbcCutGenerator(AbcModel * model,CglCutGenerator * generator,
				 int howOften, const char * name,
				 bool normal, bool atSolution, 
				 bool infeasible)
{
    model_ = model;
    generator_ = generator;
    generator_->refreshSolver(model_->solver());
    whenCutGenerator_ = howOften;
    if (name)
	generatorName_ = strdup(name);
    else
	generatorName_ = strdup("Unknown");
    normal_ = normal;
    atSolution_ = atSolution;
    whenInfeasible_ = infeasible;
}

// Copy constructor 
AbcCutGenerator::AbcCutGenerator ( const AbcCutGenerator & rhs)
{
    model_ = rhs.model_;
    generator_ = rhs.generator_;
    generator_->refreshSolver(model_->solver());
    whenCutGenerator_ = rhs.whenCutGenerator_;
    generatorName_ = strdup(rhs.generatorName_);
    normal_ = rhs.normal_;
    atSolution_ = rhs.atSolution_;
    whenInfeasible_ = rhs.whenInfeasible_;
}

// Assignment operator 
AbcCutGenerator & 
AbcCutGenerator::operator=( const AbcCutGenerator& rhs)
{
    if (this != &rhs) {
	delete generator_;
	free(generatorName_);
	model_ = rhs.model_;
	generator_ = rhs.generator_;
	generator_->refreshSolver(model_->solver());
	whenCutGenerator_ = rhs.whenCutGenerator_;
	generatorName_ = strdup(rhs.generatorName_);
	normal_ = rhs.normal_;
	atSolution_ = rhs.atSolution_;
	whenInfeasible_ = rhs.whenInfeasible_;
    }
    return *this;
}

// Destructor 
AbcCutGenerator::~AbcCutGenerator ()
{
    free(generatorName_);
}

/* This is used to refresh any inforamtion.
   It also refreshes the solver in the cut generator
   in case generator wants to do some work 
*/
void 
AbcCutGenerator::refreshModel(AbcModel * model)
{
    model_ = model;
    generator_->refreshSolver(model_->solver());
}
/* Generate cuts for the model data contained in si.
   The generated cuts are inserted into and returned in the
   collection of cuts cs.
*/
bool
AbcCutGenerator::generateCuts( OsiCuts & cs , bool fullScan)
{
    int howOften = whenCutGenerator_;
    if (howOften == -100)
	return false;
    if (howOften > 0)
	howOften = howOften % 1000000;
    else 
	howOften = 1;
    if (!howOften)
	howOften = 1;
    bool returnCode = false;
    OsiSolverInterface * solver = model_->solver();

#if defined(ABC_DEBUG_MORE)
    std::cout << "model_->getNodeCount() = " << model_->getNodeCount()
	      << std::endl;
#endif


    if (fullScan || (model_->getNodeCount() % howOften) == 0 ) {
	CglProbing* generator =
	    dynamic_cast<CglProbing*>(generator_);
	if (!generator) {
	    generator_->generateCuts(*solver,cs);
	} else {
	    // Probing - return tight column bounds
	    generator->generateCutsAndModify(*solver,cs);
	    const double * tightLower = generator->tightLower();
	    const double * lower = solver->getColLower();
	    const double * tightUpper = generator->tightUpper();
	    const double * upper = solver->getColUpper();
	    const double * solution = solver->getColSolution();
	    int j;
	    int numberColumns = solver->getNumCols();
	    double primalTolerance = 1.0e-8;
	    for (j=0; j<numberColumns; j++) {
		if (tightUpper[j] == tightLower[j] &&
		    upper[j] > lower[j]) {
		    // fix
		    solver->setColLower(j, tightLower[j]);
		    solver->setColUpper(j, tightUpper[j]);
		    if (tightLower[j] > solution[j] + primalTolerance ||
			tightUpper[j] < solution[j] - primalTolerance)
			returnCode = true;
		}
	    }
	}
    }
    return returnCode;
}

void 
AbcCutGenerator::setHowOften(int howOften) 
{
    if (howOften >= 1000000) {
	// leave Probing every 10
	howOften = howOften % 1000000;
	CglProbing* generator =
	    dynamic_cast<CglProbing*>(generator_);
	
	if (generator && howOften > 10) 
	    howOften = 10 + 1000000;
    }
    whenCutGenerator_ = howOften;
}
