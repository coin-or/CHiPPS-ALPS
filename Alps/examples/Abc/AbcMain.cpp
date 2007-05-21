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

#include "AlpsConfig.h"

#include <iostream>

#include "CoinError.hpp"
#include "CoinTime.hpp"
#include "OsiSolverInterface.hpp"
#ifdef COIN_HAS_CLP
#include "OsiClpSolverInterface.hpp"
#endif

#include "CglFlowCover.hpp"
#include "CglGomory.hpp"
#include "CglProbing.hpp"
#include "CglKnapsackCover.hpp"
#include "CglOddHole.hpp"

#ifdef COIN_HAS_MPI
#  include "AlpsKnowledgeBrokerMPI.h"
#else
#  include "AlpsKnowledgeBrokerSerial.h"
#endif

#include "AbcHeuristic.h"
#include "AbcSolution.h"
#include "AbcTreeNode.h"

//#############################################################################
//#############################################################################

int main(int argc, char* argv[])
{

    try{
	// Declare application parameter, model and knowledge broker
#ifdef  COIN_HAS_CLP
        OsiClpSolverInterface solver1;
	AbcModel model(solver1);
	solver1.getModelPtr()->setDualBound(1.0e10);
	//solver1.messageHandler()->setLogLevel(0);
#endif

#ifdef COIN_HAS_MPI
	AlpsKnowledgeBrokerMPI broker(argc, argv, model);
#else
	AlpsKnowledgeBrokerSerial broker(argc, argv, model); 
#endif
	int AbcLogLevel = model.AbcPar()->entry(AbcParams::logLevel);
	model.messageHandler()->setLogLevel(AbcLogLevel);

#if 1   // TURN ON?OFF cut generators	
	// Set up some cut generators and defaults
	// Probing first as gets tight bounds on continuous
	CglProbing generator1;
	generator1.setUsingObjective(true);
	generator1.setMaxPass(3);
	generator1.setMaxProbe(100);
	generator1.setMaxLook(50);
	generator1.setRowCuts(3);

	CglGomory generator2;	  
	generator2.setLimit(300); // try larger limit

	CglKnapsackCover generator3;

	CglOddHole generator4;
	generator4.setMinimumViolation(0.005);
	generator4.setMinimumViolationPer(0.00002);
	generator4.setMaximumEntries(200); 	// try larger limit

	CglFlowCover generator5;
	
	// Add in generators
	//model.addCutGenerator(&generator1, -1, "Probing");
	//model.addCutGenerator(&generator2, -1, "Gomory");
	model.addCutGenerator(&generator3, -1, "Knapsack");
	model.addCutGenerator(&generator4, -1, "OddHole");
	//model.addCutGenerator(&generator5, -1, "FlowCover");


	// Use rounding heuristic
	AbcRounding heuristic1(model);
	model.addHeuristic(&heuristic1);
#endif

	// Register model, solution, and tree node
	broker.registerClass(ALPS_MODEL, new AbcModel);
	broker.registerClass(ALPS_SOLUTION, new AbcSolution);
	broker.registerClass(ALPS_NODE, new AbcTreeNode(&model));

	// Formulate the root node
	// NOTE: root will be deleted by ALPS 
	AlpsTreeNode* root = new AbcTreeNode(&model);

	// Search for solutions from give root.
	broker.rootSearch(root);
    }
    catch(CoinError& er) {
	std::cerr << "ERROR:" << er.message() << std::endl
		  << " from function " << er.methodName() << std::endl
		  << " from class " << er.className() << std::endl;
    }
    catch(...) {
	std::cerr << "Something went wrong!" << std::endl;
    }
    
    return 0;
}
