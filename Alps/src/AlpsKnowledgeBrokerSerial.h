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
 *===========================================================================*/

#ifndef AlpsKnowledgeBrokerSerial_h_
#define AlpsKnowledgeBrokerSerial_h_

#include "Alps.h"
#include "AlpsEnumProcessT.h"
#include "AlpsKnowledgeBroker.h"
#include "AlpsMessage.h"
#include "AlpsModel.h"
#include "AlpsParams.h"

//#############################################################################

class AlpsKnowledgeBrokerSerial : public AlpsKnowledgeBroker {
 private:
    AlpsKnowledgeBrokerSerial(const AlpsKnowledgeBrokerSerial&);
    AlpsKnowledgeBrokerSerial& operator=(const AlpsKnowledgeBrokerSerial&);

 public:
    AlpsKnowledgeBrokerSerial() 
	: 
	AlpsKnowledgeBroker() 
	{} 

    AlpsKnowledgeBrokerSerial(int argc, 
			      char* argv[], 
			      AlpsModel& model) 
	: 
	AlpsKnowledgeBroker() 
	{ 
	    initializeSearch(argc, argv, model); 
	}

    //-------------------------------------------------------------------------
    /** @name Report the search results.
     *  
     */
    //@{

    /** Search log. */
    virtual void searchLog();
    
    /** The process queries the quality of the incumbent that it stores.*/
    virtual double getIncumbentValue() const {
	return getBestQuality();
    }

    /** The process queries the quality of the best 
	solution that it finds. */
    virtual double getBestQuality() const {
	if (AlpsKnowledgeBroker::hasKnowledge(ALPS_SOLUTION)) {
	    return getBestKnowledge(ALPS_SOLUTION).second;
        }
	else {
	    return ALPS_INC_MAX;
        }
    }

    /** The process outputs the best solution and the quality 
	that it finds to a file or std::out. */
    virtual void printBestSolution(char* outputFile = 0) const {
	if (getNumKnowledges(ALPS_SOLUTION) <= 0) {
	    std::cout << "\nALPS did not find a solution."
		      << std::endl;
	    return;
	}
	if (outputFile != 0) {                 
            // Write to outputFile
	    std::ofstream os(outputFile);
	    os << "Quality = " << getBestQuality();
	    os << std::endl;
	    dynamic_cast<AlpsSolution* >
		(getBestKnowledge(ALPS_SOLUTION).first)->print(os);
	}
	else {                                  // Write to std::cout
	    std::cout << "Quality = " << getBestQuality();
	    std::cout << std::endl;
	    dynamic_cast<AlpsSolution* >
		(getBestKnowledge(ALPS_SOLUTION).first)->print(std::cout);
	}
    }
    //@}

    /** Reading in Alps and user parameter sets, and read in model data. */
    virtual void initializeSearch(int argc, 
                                  char* argv[], 
                                  AlpsModel& model);
    
    /** Search for best solution. */
    virtual void rootSearch(AlpsTreeNode* root);
    
};
#endif
