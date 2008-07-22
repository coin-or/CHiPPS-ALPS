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
 * Copyright (C) 2001-2008, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#include "AlpsConfig.h"

#include <iostream>

#include "CoinError.hpp"
#include "CoinTime.hpp"

#ifdef COIN_HAS_MPI
#  include "AlpsKnowledgeBrokerMPI.h"
#else
#  include "AlpsKnowledgeBrokerSerial.h"
#endif

#include "KnapSolution.h"
#include "KnapTreeNode.h"

//#############################################################################

int main(int argc, char* argv[])
{

    try{
	// 1: Declare application parameter set, model and knowledge broker
	KnapModel model;

#ifdef COIN_HAS_MPI
	AlpsKnowledgeBrokerMPI broker(argc, argv, model);
#else
	AlpsKnowledgeBrokerSerial broker(argc, argv, model);
#endif
	
	// 2: Register model, solution, and tree node
	broker.registerClass(AlpsKnowledgeTypeModel, new KnapModel);
	broker.registerClass(AlpsKnowledgeTypeSolution, 
			     new KnapSolution(&model));
	broker.registerClass(AlpsKnowledgeTypeNode, new KnapTreeNode(&model));

	// 3: Formulate the root node
	// NOTE: root will be deleted by ALPS 
	AlpsTreeNode* root = new KnapTreeNode(&model);

	// 4: Solve the problem
	broker.rootSearch(root);
    
	// 5: Report the best solution found and its ojective value
	broker.printBestSolution();
        
#ifdef NF_DEBUG
	const int numSol = broker.getNumKnowledges(AlpsKnowledgeTypeSolution);
	broker.messageHandler()->message(AlpsKnowledgeTypeSolution_COUNT,broker.messages())
	    << broker.getProcRank() << numSol << CoinMessageEol;
#endif

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

//#############################################################################

