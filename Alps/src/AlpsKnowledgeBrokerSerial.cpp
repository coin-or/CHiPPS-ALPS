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
 * Corporation, Lehigh University, Yan Xu, Ted Ralphs, Matthew Salzman and   *
 *===========================================================================*/


#include "AlpsKnowledgeBrokerSerial.h"


//#############################################################################

/** Reading in Alps and user parameter sets, and read in model data. */
void 
AlpsKnowledgeBrokerSerial::initializeSearch(int argc, 
					    char* argv[], 
					    AlpsModel& model) {
    std::cout << std::endl;
    std::cout << "*****************************" << std::endl;
    std::cout << "* ALPS Version 0.6 (Serial) *" << std::endl;
    std::cout << "*****************************" << std::endl << std::endl;
    
    // Store a pointer to model
    model.setKnowledgeBroker(this);
    model_ = &model;

    //--------------------------------------------------
    // Read in params.
    //--------------------------------------------------

    model.readParameters(argc, argv);

    //--------------------------------------------------
    // If there two args: xecutable + instance.
    //--------------------------------------------------
    
    if (argc == 2) {
        model_->AlpsPar()->setEntry(AlpsParams::instance, argv[1]);
    }
    
    //--------------------------------------------------
    // Read in model data if NEEDED.
    //--------------------------------------------------
    
    if ( model_->AlpsPar()->entry(AlpsParams::inputFromFile) ) {
	const char* dataFile = 
	    model_->AlpsPar()->entry(AlpsParams::instance).c_str();
	messageHandler()->message(ALPS_DATAFILE, messages())
	    << dataFile << CoinMessageEol;
	model.readInstance(dataFile);
	
	if (logFileLevel_ > 0) {
	    std::string fileDir = dataFile;
	    std::string::size_type pos1 = 
		fileDir.rfind ('/', std::string::npos);
	    if (pos1 == std::string::npos) {
		// No /, put at beginning.
		pos1 = 0;
		//std::cout << "No /, pos1 = "<< pos1 << std::endl;
	    }
	    else {
		//std::cout << "Found /, pos1 = "<< pos1 << std::endl;
		++pos1;
	    }
		
	    std::string::size_type pos2 = fileDir.find (".mps", pos1);
	    
	    if (pos2 == std::string::npos) {
		// No .mps
		//std::cout << "No .mps" << std::endl;
		
		pos2 = fileDir.find (".gz", pos1); 
		if (pos2 == std::string::npos) {
		    // No .gz
		    pos2 = fileDir.length();
		    //std::cout << "No .gz, pos2 = "<< pos2 << std::endl;
		}
		else { 
		    //std::cout << "Found .gz, pos2 = "<< pos2 << std::endl;
		}
	    }
	    
	    // Sub-string from pos1 to pos2(not included)
	    int length = pos2 - pos1;
	    //std::cout << "pos1=" << pos1 <<", pos2="<< pos2
	    //        << ", lenght=" << length << std::endl;
	    
	    instanceName_ = fileDir.substr(pos1, length);    
	    logfile_ = instanceName_ + ".log";
		
            model_->AlpsPar()->setEntry(AlpsParams::logFile, 
                                        logfile_.c_str());

	    //std::ofstream logFout(logfile_.c_str(), std::ofstream::app);
	    std::ofstream logFout(logfile_.c_str());
            
	    // Times
	    logFout << "\n================================================"
		    << std::endl;
	    logFout << "Problem = " << instanceName_ << std::endl;
	    logFout << "Log file = " << logfile_ << std::endl << std::endl;
	    std::cout << "Problem = " << instanceName_ << std::endl;
	    std::cout << "Data file = " << dataFile << std::endl;
	    std::cout << "Log file = " << logfile_ << std::endl<< std::endl;
	}
    }
    
    //--------------------------------------------------
    // Do some require work to make the model usable.
    // May adjust parameters like msg level in user's code.
    //--------------------------------------------------
    
    model.setupSelf();

    //--------------------------------------------------
    // Set up messege, logfile.
    //--------------------------------------------------

    msgLevel_ = model_->AlpsPar()->entry(AlpsParams::msgLevel);
    
    // TODO. what's coin message settings.
    messageHandler()->setLogLevel(msgLevel_);
    
    messageHandler()->message(ALPS_PARAMFILE, messages())
	<< argv[1] << CoinMessageEol;
    
    logFileLevel_ = model_->AlpsPar()->entry(AlpsParams::logFileLevel);
    if (logFileLevel_ > 0) {    // Require log file
	logfile_ = model_->AlpsPar()->entry(AlpsParams::logFile).c_str();
    }
    
    //--------------------------------------------------
    // Set up solution pool and subtree pool, set comparision.
    //--------------------------------------------------
    
    setupKnowledgePools();

    
    //--------------------------------------------------
    // Register knowledge (useless for serial).
    //--------------------------------------------------

    model.registerKnowledge();    
}

//#############################################################################

void 
AlpsKnowledgeBrokerSerial::search(AlpsTreeNode* root)
{
    AlpsReturnCode status = ALPS_OK;
    
    timer_.start();
    
    root->setKnowledgeBroker(this);
    root->setQuality(ALPS_OBJ_MAX);
    root->setDepth(0);
    root->setIndex(0);
    root->setExplicit(1); // True.
    
    const int mns = model_->AlpsPar()->entry(AlpsParams::maxNumSolustion);
    setMaxNumKnowledges(ALPS_SOLUTION, mns);
    
    //------------------------------------------------------
    // Construct tree.
    //------------------------------------------------------

    workingSubTree_ = new AlpsSubTree(this);
    workingSubTree_->setNodeCompare(nodeCompare_);

#ifdef NF_DEBUG_MORE
    // Useless work, just for testing subtree pool.
    subTreePool_->addKnowledge(workingSubTree_, 
			       workingSubTree_->getQuality());
    subTreePool_->popKnowledge();
#endif
    
    workingSubTree_->setNextIndex(1);  // one more than root's index
    
    messageHandler()->message(ALPS_S_SEARCH_START, messages())
	<< CoinMessageEol;
    
    //------------------------------------------------------
    // Search the best solution.
    //------------------------------------------------------

    const int nodeLimit = model_->AlpsPar()->entry(AlpsParams::nodeLimit);
    const double timeLimit = model_->AlpsPar()->entry(AlpsParams::timeLimit);
    
    status = workingSubTree_->exploreSubTree(root,
					     nodeLimit,
					     timeLimit,
					     nodeProcessedNum_, 
					     treeDepth_);
    
    getNumNodesLeft();

    timer_.stop();
    
    //------------------------------------------------------
    // Log statistics.
    //------------------------------------------------------

    if (msgLevel_ > 0) {
	if (getTermStatus() == ALPS_OPTIMAL) {
	    messageHandler()->message(ALPS_T_OPTIMAL, messages())
		<< nodeProcessedNum_ << nodeLeftNum_ << CoinMessageEol;
	}
	else if (getTermStatus() == ALPS_NODE_LIMIT) {
	    messageHandler()->message(ALPS_T_NODE_LIMIT, messages())
		<< nodeProcessedNum_ << nodeLeftNum_ << CoinMessageEol;
	}
	else if (getTermStatus() == ALPS_TIME_LIMIT) {
	    messageHandler()->message(ALPS_T_TIME_LIMIT, messages())
		<< nodeProcessedNum_ << nodeLeftNum_ << CoinMessageEol; 
	}
	else if (getTermStatus() == ALPS_FEASIBLE) {
	    messageHandler()->message(ALPS_T_FEASIBLE, messages())
		<< nodeProcessedNum_ << nodeLeftNum_ << CoinMessageEol;
	}
	else {
	    messageHandler()->message(ALPS_T_INFEASIBLE, messages())
		<< nodeProcessedNum_ << nodeLeftNum_ << CoinMessageEol;
	}
    }
    searchLog();
    model_->modelLog();
}

//#############################################################################

void 
AlpsKnowledgeBrokerSerial::searchLog() 
{

    if (msgLevel_ > 0) {
        std::cout << "Number of nodes processed = " << nodeProcessedNum_ 
                  << std::endl;
        std::cout << "Number of nodes left = " << nodeLeftNum_ << std::endl;
        std::cout << "Tree depth = " << treeDepth_ << std::endl;
        std::cout << "Search CPU time = " << timer_.getCpuTime() << " seconds"
                  << std::endl;
        std::cout << "Search wallclock = "<<timer_.getWallClock()<<" seconds"
                  << std::endl;
        std::cout << "Best solution quality = " << getBestQuality() 
                  << std::endl << std::endl;
    }
    
    if(logFileLevel_ > 0) {

	std::ofstream fout(logfile_.c_str(), std::ofstream::app);
	fout << "Number of nodes processed = "<<nodeProcessedNum_ << std::endl;
	fout << "Number of nodes left = " << nodeLeftNum_ << std::endl;
	fout << "Tree depth = " << treeDepth_ << std::endl;
	
	fout << "Search CPU time =  " << timer_.getCpuTime() << " seconds"
	     << std::endl;
	fout << "Search wallclock = " << timer_.getWallClock() <<" seconds"
	      << std::endl;
	fout << "Best solution quality = " << getBestQuality() 
             << std::endl << std::endl;
	
	
	// Print soluton.
	// if (getBestKnowledge(ALPS_SOLUTION).first != NULL) {
	//dynamic_cast<AlpsSolution* >
	//    (getBestKnowledge(ALPS_SOLUTION).first)->print(logFout);
	//}
    }
    

    
}

//#############################################################################
